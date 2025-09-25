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
	CollisionComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block); // Block walls

	// Players can't walk on it
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComp;

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 1400.f; // Slower for bullet hell feel
	ProjectileMovement->MaxSpeed = 1400.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false; // No bouncing for bullet hell

	// DISABLE GRAVITY - Key for bullet hell projectiles
	ProjectileMovement->ProjectileGravityScale = 0.0f;

	// Disable built-in homing (we'll handle it manually)
	ProjectileMovement->bIsHomingProjectile = false;

	// Bullet hell properties
	TrackingStrength = 3000.0f; // Much more aggressive tracking
	MaxTrackingDistance = 3500.0f;
	bCanTrackPlayer = true;
	TargetPlayer = nullptr;

	// Set tick to update tracking
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.02f; // Update tracking frequently

	// Die after 8 seconds (longer for bullet hell)
	InitialLifeSpan = 8.0f;
}

void AIconoclasmProjectile::BeginPlay()
{
    Super::BeginPlay();

    // Find the player character
    FindTargetPlayer();
}

void AIconoclasmProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bCanTrackPlayer && TargetPlayer)
    {
        UpdatePlayerTracking(DeltaTime);
    }
}

void AIconoclasmProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    // Check if we hit a wall/static object
    if (OtherComp && OtherComp->GetCollisionObjectType() == ECollisionChannel::ECC_WorldStatic)
    {
        // Just destroy the projectile when hitting walls, no explosion
        Destroy();
        UE_LOG(LogTemp, Warning, TEXT("Projectile hit wall and was destroyed"));
        return;
    }

    // Only react to hitting the PLAYER character specifically
    if (OtherActor && OtherActor != this && OtherActor == TargetPlayer)
    {
        // Destroy the projectile
        Destroy();

        // Explosion parameters
        FVector ImpactLocation = GetActorLocation();
        float ExplosionRadius = 300.0f;
        float BaseDamage = 100.0f;
        float DamageFalloff = 0.0f;

        // Draw debug sphere to visualize the radius
        DrawDebugSphere(GetWorld(), ImpactLocation, ExplosionRadius, 32, FColor::Red, false, 2.0f);

        // Create array of actors to ignore (all enemies)
        TArray<AActor*> IgnoreActors;

        // Find all actors that are NOT the player and add them to ignore list
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

        // Apply radial damage, ignoring all enemies
        UGameplayStatics::ApplyRadialDamage(
            GetWorld(),
            BaseDamage,
            ImpactLocation,
            ExplosionRadius,
            UDamageType::StaticClass(),
            IgnoreActors, // Pass the ignore list
            this,
            GetInstigatorController(),
            true,
            ECC_Visibility
        );

        UE_LOG(LogTemp, Warning, TEXT("Explosion at location: %s"), *ImpactLocation.ToString());
    }
    // For all other actors (enemies, etc.), ignore the hit completely
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