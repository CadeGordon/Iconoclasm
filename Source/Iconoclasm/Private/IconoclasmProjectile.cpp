// Copyright Epic Games, Inc. All Rights Reserved.

#include "IconoclasmProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "IconoclasmCharacter.h"
#include "EngineUtils.h"

AIconoclasmProjectile::AIconoclasmProjectile() 
{
    // Use a sphere as a simple collision representation
    CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    CollisionComp->InitSphereRadius(5.0f);
    CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
    CollisionComp->OnComponentHit.AddDynamic(this, &AIconoclasmProjectile::OnHit);

    // Set collision responses - block world static (walls) and pawns
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionComp->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    CollisionComp->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    CollisionComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
    CollisionComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);

    // NEW: allow grapple traces (your grapple uses Visibility traces)
    CollisionComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

    // Players can't walk on it
    CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
    CollisionComp->CanCharacterStepUpOn = ECB_No;

    // Set as root component
    RootComponent = CollisionComp;

    // Use a ProjectileMovementComponent to govern this projectile's movement
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
    ProjectileMovement->UpdatedComponent = CollisionComp;
    ProjectileMovement->InitialSpeed = 1900.f;
    ProjectileMovement->MaxSpeed = 1900.f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;

    // DISABLE GRAVITY
    ProjectileMovement->ProjectileGravityScale = 0.0f;

    // Disable built-in homing (we'll handle it manually)
    ProjectileMovement->bIsHomingProjectile = false;

    // Bullet hell properties
    TrackingStrength = 4500.0f;
    MaxTrackingDistance = 5500.0f;
    bCanTrackPlayer = true;
    TargetPlayer = nullptr;

    // Reflection properties
    bIsReflected = false;
    ReflectedExplosionRadius = 400.0f;
    ReflectedExplosionDamage = 150.0f;
    OriginalInstigator = nullptr;

    // NEW: grapple state
    bIsBeingGrappled = false;
    bIsHeldByPlayer = false;
    GrapplePullSpeed = 4200.0f;
    HoldingCharacter = nullptr;

    // Set tick to update tracking
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.02f;

    // Die after 8 seconds
    InitialLifeSpan = 8.0f;
}

void AIconoclasmProjectile::BeginPlay()
{
    Super::BeginPlay();

    // Store original instigator
    OriginalInstigator = GetInstigator();

    // Find the player character
    FindTargetPlayer();
}

void AIconoclasmProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // NEW: if grapple is pulling us, move toward holder
    if (bIsBeingGrappled && !bIsHeldByPlayer)
    {
        ACharacter* Holder = HoldingCharacter.Get();
        if (!Holder)
        {
            StopGrapplePull();
        }
        else
        {
            // Pull toward a point in front of the player (not necessarily a socket yet)
            FVector TargetPoint = Holder->GetActorLocation() + Holder->GetActorForwardVector() * 120.0f + FVector(0, 0, 60.0f);
            FVector Current = GetActorLocation();
            FVector ToTarget = (TargetPoint - Current);
            float Dist = ToTarget.Size();

            if (Dist <= 120.0f)
            {
                // Close enough - actual attach is done by grapple component
                // (We stay stable here)
                SetActorLocation(TargetPoint);
            }
            else
            {
                FVector Step = ToTarget.GetSafeNormal() * GrapplePullSpeed * DeltaTime;
                SetActorLocation(Current + Step);
            }
        }

        return; // don't do tracking while being grappled
    }

    // Only track player if NOT reflected and not held
    if (!bIsHeldByPlayer && !bIsReflected && bCanTrackPlayer && TargetPlayer)
    {
        UpdatePlayerTracking(DeltaTime);
    }
}

void AIconoclasmProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    // NEW: if held, ignore any hit logic (shouldn't happen if collision is disabled)
    if (bIsHeldByPlayer)
    {
        return;
    }

    // If reflected (OR thrown by grapple), explode on ANY hit
    if (bIsReflected)
    {
        FVector ImpactLocation = GetActorLocation();

        // Draw debug sphere
        DrawDebugSphere(GetWorld(), ImpactLocation, ReflectedExplosionRadius, 32, FColor::Orange, false, 2.0f);

        // Create array to ignore the player who reflected/threw it
        TArray<AActor*> IgnoreActors;
        if (GetInstigator())
        {
            IgnoreActors.Add(GetInstigator());
        }

        UGameplayStatics::ApplyRadialDamage(
            GetWorld(),
            ReflectedExplosionDamage,
            ImpactLocation,
            ReflectedExplosionRadius,
            UDamageType::StaticClass(),
            IgnoreActors,
            this,
            GetInstigatorController(),
            true,
            ECC_Visibility
        );

        UE_LOG(LogTemp, Warning, TEXT("Reflected/thrown projectile exploded at: %s"), *ImpactLocation.ToString());

        Destroy();
        return;
    }

    // Original non-reflected behavior
    // Check if we hit a wall/static object
    if (OtherComp && OtherComp->GetCollisionObjectType() == ECollisionChannel::ECC_WorldStatic)
    {
        Destroy();
        UE_LOG(LogTemp, Warning, TEXT("Projectile hit wall and was destroyed"));
        return;
    }

    // Only react to hitting the PLAYER character specifically
    if (OtherActor && OtherActor != this && OtherActor == TargetPlayer)
    {
        Destroy();

        FVector ImpactLocation = GetActorLocation();
        float ExplosionRadius = 300.0f;
        float BaseDamage = 100.0f;

        DrawDebugSphere(GetWorld(), ImpactLocation, ExplosionRadius, 32, FColor::Red, false, 2.0f);

        TArray<AActor*> IgnoreActors;
        UWorld* World = GetWorld();
        if (World)
        {
            for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
            {
                AActor* CurrentActor = *ActorIterator;
                if (CurrentActor && CurrentActor != TargetPlayer && CurrentActor->IsA(AIconoclasmCharacter::StaticClass()))
                {
                    IgnoreActors.Add(CurrentActor);
                }
            }
        }

        UGameplayStatics::ApplyRadialDamage(
            GetWorld(),
            BaseDamage,
            ImpactLocation,
            ExplosionRadius,
            UDamageType::StaticClass(),
            IgnoreActors,
            this,
            GetInstigatorController(),
            true,
            ECC_Visibility
        );

        UE_LOG(LogTemp, Warning, TEXT("Explosion at location: %s"), *ImpactLocation.ToString());
    }
}

void AIconoclasmProjectile::ReflectProjectile(const FVector& ReflectionDirection, AActor* NewInstigator)
{
    // Mark as reflected
    bIsReflected = true;

    // Disable player tracking
    bCanTrackPlayer = false;
    TargetPlayer = nullptr;

    // Change instigator to the player
    SetInstigator(Cast<APawn>(NewInstigator));
    SetOwner(NewInstigator);

    // Increase speed for reflected projectiles
    float ReflectedSpeed = 2500.0f;
    ProjectileMovement->MaxSpeed = ReflectedSpeed;
    ProjectileMovement->Velocity = ReflectionDirection.GetSafeNormal() * ReflectedSpeed;

    // Optional: Change collision to hit enemies
    CollisionComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);

    UE_LOG(LogTemp, Warning, TEXT("Projectile reflected! New direction: %s"), *ReflectionDirection.ToString());
}

void AIconoclasmProjectile::AltOnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (OtherActor && OtherActor != this && OtherActor->IsA(AIconoclasmCharacter::StaticClass()))
    {
        // Apply damage to the hit actor
        float DamageAmount = 100.0f; // Adjust damage as needed
        UGameplayStatics::ApplyDamage(
            OtherActor,                // The target actor (the actor that was hit)
            DamageAmount,              // The damage amount
            GetInstigatorController(), // The instigator (the actor who fired the projectile)
            this,                      // The damage causer (the projectile itself)
            UDamageType::StaticClass() // The type of damage (you can define a custom damage type if needed)
        );

        // Destroy the projectile
        Destroy();

        // Create the healing area
        FVector ImpactLocation = GetActorLocation();
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.Instigator = GetInstigator();

        // Draw debug sphere to visualize the healing area
        DrawDebugSphere(GetWorld(), ImpactLocation, 300.0f, 32, FColor::Green, false, 10.0f);

        // Log a debug message to see if it hit something
        UE_LOG(LogTemp, Warning, TEXT("Projectile hit: %s"), *OtherActor->GetName());
    }
}

void AIconoclasmProjectile::FireInDirection(const FVector& ShootDirection)
{
    // Set the velocity of the projectile
    ProjectileMovement->Velocity = ShootDirection * ProjectileMovement->InitialSpeed;

    // Find and set tracking target
    FindTargetPlayer();
}

void AIconoclasmProjectile::FindTargetPlayer()
{
	// Find the player character in the world
	if (UWorld* World = GetWorld())
	{
		TargetPlayer = UGameplayStatics::GetPlayerCharacter(World, 0);
	}
}

void AIconoclasmProjectile::UpdatePlayerTracking(float DeltaTime)
{
	if (!TargetPlayer || !ProjectileMovement)
		return;

	FVector PlayerLocation = TargetPlayer->GetActorLocation();
	FVector ProjectileLocation = GetActorLocation();
	float DistanceToPlayer = FVector::Dist(PlayerLocation, ProjectileLocation);

	// Only track if within max distance
	if (DistanceToPlayer <= MaxTrackingDistance)
	{
		// Calculate direction to player
		FVector DirectionToPlayer = (PlayerLocation - ProjectileLocation).GetSafeNormal();

		// Get current velocity direction
		FVector CurrentVelocity = ProjectileMovement->Velocity;
		FVector CurrentDirection = CurrentVelocity.GetSafeNormal();

		// Much more aggressive interpolation for bullet hell feel
		float InterpolationRate = TrackingStrength / 100.0f; // Increased rate
		FVector NewDirection = FMath::VInterpTo(CurrentDirection, DirectionToPlayer, DeltaTime, InterpolationRate);

		// Maintain speed while changing direction
		ProjectileMovement->Velocity = NewDirection * ProjectileMovement->MaxSpeed;

		// Optional: Draw debug line to show tracking
		if (GEngine && GEngine->GetNetMode(GetWorld()) != NM_DedicatedServer)
		{
			DrawDebugLine(GetWorld(), ProjectileLocation, PlayerLocation, FColor::Yellow, false, 0.1f, 0, 1.0f);
		}
	}
}

void AIconoclasmProjectile::SetTrackingEnabled(bool bEnabled)
{
	bCanTrackPlayer = bEnabled;
}

void AIconoclasmProjectile::SetTrackingStrength(float NewStrength)
{
	TrackingStrength = NewStrength;

	
}

bool AIconoclasmProjectile::IsGrapplable() const
{
    // Can grapple it if it's not already held
    return !bIsHeldByPlayer;
}

void AIconoclasmProjectile::StartGrapplePull(ACharacter* PullingCharacter)
{
    if (!PullingCharacter || bIsHeldByPlayer)
        return;

    bIsBeingGrappled = true;
    HoldingCharacter = PullingCharacter;

    // Stop tracking while being pulled
    bCanTrackPlayer = false;
    TargetPlayer = nullptr;

    // Stop movement component influence while we manually move it
    if (ProjectileMovement)
    {
        ProjectileMovement->StopMovementImmediately();
        ProjectileMovement->Deactivate();
    }

    // Prevent it from hitting stuff while being pulled
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AIconoclasmProjectile::StopGrapplePull()
{
    bIsBeingGrappled = false;
    HoldingCharacter = nullptr;

    // If we are not held, restore collision + movement (but still no tracking by default)
    if (!bIsHeldByPlayer)
    {
        CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

        if (ProjectileMovement)
        {
            ProjectileMovement->Activate();
        }
    }
}

void AIconoclasmProjectile::AttachToPlayer(ACharacter* NewHolder, const FName& SocketName, const FVector& RelativeOffset)
{
    if (!NewHolder)
        return;

    bIsBeingGrappled = false;
    bIsHeldByPlayer = true;
    HoldingCharacter = NewHolder;

    // We are now "owned" by the player
    SetOwner(NewHolder);
    SetInstigator(Cast<APawn>(NewHolder));

    // Disable tracking and reflection until thrown
    bCanTrackPlayer = false;
    TargetPlayer = nullptr;

    // Freeze movement
    if (ProjectileMovement)
    {
        ProjectileMovement->StopMovementImmediately();
        ProjectileMovement->Deactivate();
    }

    // Collision off while held
    SetHeldCollision(true);

    // Attach to mesh if possible, otherwise attach to root
    if (USkeletalMeshComponent* Mesh = NewHolder->GetMesh())
    {
        FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, true);
        AttachToComponent(Mesh, Rules, SocketName);
        SetActorRelativeLocation(RelativeOffset);
        SetActorRelativeRotation(FRotator::ZeroRotator);
    }
    else
    {
        FAttachmentTransformRules Rules(EAttachmentRule::KeepWorld, true);
        AttachToComponent(NewHolder->GetRootComponent(), Rules);
    }
}

void AIconoclasmProjectile::ThrowFromPlayer(const FVector& ThrowDirection, float ThrowSpeed, AActor* NewInstigator)
{
    if (!bIsHeldByPlayer)
        return;

    ACharacter* Holder = HoldingCharacter.Get();

    // Detach from player
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    bIsHeldByPlayer = false;
    bIsBeingGrappled = false;
    HoldingCharacter = nullptr;

    // Now treat as "reflected" so it explodes on any impact (your existing behavior)
    bIsReflected = true;

    // Set instigator/owner to player
    SetOwner(NewInstigator);
    SetInstigator(Cast<APawn>(NewInstigator));

    // Restore collision and movement
    SetHeldCollision(false);

    if (ProjectileMovement)
    {
        ProjectileMovement->Activate(true);
        ProjectileMovement->Velocity = ThrowDirection.GetSafeNormal() * ThrowSpeed;
        ProjectileMovement->MaxSpeed = ThrowSpeed;
        ProjectileMovement->bRotationFollowsVelocity = true;
    }

    // Important: ignore pawn collision briefly so it doesn't instantly hit the player capsule
    CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ReenablePawnCollision);
        GetWorld()->GetTimerManager().SetTimer(
            TimerHandle_ReenablePawnCollision,
            this,
            &AIconoclasmProjectile::ReenablePawnCollision,
            0.12f,
            false
        );
    }
}

void AIconoclasmProjectile::SetHeldCollision(bool bHeld)
{
    if (!CollisionComp)
        return;

    if (bHeld)
    {
        CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    else
    {
        CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

        // Restore normal responses
        CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
        CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
        CollisionComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    }
}

void AIconoclasmProjectile::ReenablePawnCollision()
{
    if (CollisionComp)
    {
        CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    }
}