// Fill out your copyright notice in the Description page of Project Settings.


#include "GrenadeLauncherProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "IconoclasmCharacter.h"

// Sets default values
AGrenadeLauncherProjectile::AGrenadeLauncherProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Use a sphere as a simple collision representation
	GrenadeCollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	GrenadeCollisionComp->InitSphereRadius(5.0f);
	GrenadeCollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
	GrenadeCollisionComp->OnComponentHit.AddDynamic(this, &AGrenadeLauncherProjectile::GrenadeOnHit);		// set up a notification for when this component hits something blocking

	// Players can't walk on it
	GrenadeCollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	GrenadeCollisionComp->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = GrenadeCollisionComp;

	// Use a ProjectileMovementComponent to govern this projectile's movement
	GrenadeProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	GrenadeProjectileMovement->UpdatedComponent = GrenadeCollisionComp;
	GrenadeProjectileMovement->InitialSpeed = 3000.f;
	GrenadeProjectileMovement->MaxSpeed = 3000.f;
	GrenadeProjectileMovement->bRotationFollowsVelocity = true;
	GrenadeProjectileMovement->bShouldBounce = true;

	// Die after 3 seconds by default
	InitialLifeSpan = 8.0f;

}



void AGrenadeLauncherProjectile::GrenadeOnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Check if this is alt fire mode
	if (bIsAltFire)
	{
		// Call the alt fire hit function
		GrenadeAltOnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
		return; // Exit early since alt fire handles everything
	}

	// NORMAL FIRE MODE - Your existing logic
	// Get the hit location for damage radius center
	FVector ExplosionLocation = Hit.Location;

	// Define explosion parameters
	float ExplosionRadius = 700.0f;
	float BaseDamage = 10.0f;
	float MinimumDamage = 20.0f;

	// Apply radial damage to all actors within the explosion radius
	UGameplayStatics::ApplyRadialDamage(
		GetWorld(),
		BaseDamage,
		ExplosionLocation,
		ExplosionRadius,
		UDamageType::StaticClass(),
		TArray<AActor*>(), // Empty ignore list
		this,
		nullptr,
		true, // Full damage at center
		ECollisionChannel::ECC_Visibility
	);

	// Shrapnel system - fire random line traces in all directions
	int32 ShrapnelCount = 35; // Number of shrapnel traces
	float ShrapnelRange = 1250.0f; // Maximum range of shrapnel
	float ShrapnelDamage = 250.0f; // Damage per shrapnel hit

	for (int32 i = 0; i < ShrapnelCount; i++)
	{
		// Generate a random direction vector
		FVector RandomDirection = FVector(
			FMath::RandRange(-1.0f, 1.0f),
			FMath::RandRange(-1.0f, 1.0f),
			FMath::RandRange(-1.0f, 1.0f)
		).GetSafeNormal();

		// Calculate start and end points for the line trace
		FVector TraceStart = ExplosionLocation;
		FVector TraceEnd = ExplosionLocation + (RandomDirection * ShrapnelRange);

		// Perform the line trace
		FHitResult ShrapnelHit;
		FCollisionQueryParams ShrapnelParams;
		ShrapnelParams.AddIgnoredActor(this); // Ignore the projectile itself
		ShrapnelParams.bTraceComplex = true;

		if (GetWorld()->LineTraceSingleByChannel(ShrapnelHit, TraceStart, TraceEnd, ECC_Pawn, ShrapnelParams))
		{
			// Hit something with shrapnel
			if (AActor* HitActor = ShrapnelHit.GetActor())
			{
				// Apply damage to the hit actor
				UGameplayStatics::ApplyDamage(
					HitActor,
					ShrapnelDamage,
					GetInstigatorController(),
					this,
					UDamageType::StaticClass()
				);

				// Debug visualization of shrapnel hit
				if (GEngine)
				{
					DrawDebugLine(GetWorld(), TraceStart, ShrapnelHit.Location, FColor::Yellow, false, 2.0f, 0, 1.0f);
					DrawDebugPoint(GetWorld(), ShrapnelHit.Location, 5.0f, FColor::Orange, false, 2.0f);
				}
			}
		}
		else
		{
			// No hit - draw the full trace for debugging
			if (GEngine)
			{
				DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Blue, false, 1.0f, 0, 0.5f);
			}
		}
	}

	// Spawn explosion visual effect if you have one
	// UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, ExplosionLocation);

	// Play explosion sound if you have one
	// UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplosionSound, ExplosionLocation);

	// Debug visualization of explosion radius
	if (GEngine)
	{
		DrawDebugSphere(GetWorld(), ExplosionLocation, ExplosionRadius, 12, FColor::Red, false, 3.0f);
	}

	// Destroy the projectile after explosion
	Destroy();
}

void AGrenadeLauncherProjectile::GrenadeAltOnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Get the hit location for damage radius center
	FVector ExplosionLocation = Hit.Location;

	// Define alternative explosion parameters (could be different from normal hit)
	float ExplosionRadius = 750.0f; // Larger radius for alt fire
	float GrabRadius = 1250.0f; // Grab radius - larger than damage radius
	float BaseDamage = 1.0f; // Higher damage for alt fire
	float MinimumDamage = 30.0f;
	float PullForce = 2000.0f; // Force to pull enemies toward center

	// Apply radial damage to all actors within the explosion radius
	UGameplayStatics::ApplyRadialDamage(
		GetWorld(),
		BaseDamage,
		ExplosionLocation,
		ExplosionRadius,
		UDamageType::StaticClass(),
		TArray<AActor*>(), // Empty ignore list
		this,
		nullptr,
		true, // Full damage at center
		ECollisionChannel::ECC_Visibility
	);

	// Find all actors within grab radius for pull effect
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	CollisionParams.bTraceComplex = false;
	CollisionParams.bReturnPhysicalMaterial = false;

	// Perform sphere overlap to find actors in grab radius
	bool bHasOverlap = GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		ExplosionLocation,
		FQuat::Identity,
		ECollisionChannel::ECC_Pawn, // Target pawns/characters
		FCollisionShape::MakeSphere(GrabRadius),
		CollisionParams
	);

	if (bHasOverlap)
	{
		// Process each overlapped actor
		for (const FOverlapResult& OverlapResult : OverlapResults)
		{
			AActor* TargetActor = OverlapResult.GetActor();
			if (!TargetActor)
				continue;

			// Check if this is an enemy/valid target (you might want to add more filtering here)
			APawn* TargetPawn = Cast<APawn>(TargetActor);
			if (!TargetPawn)
				continue;

			// Get the actor's location
			FVector TargetLocation = TargetActor->GetActorLocation();

			// Calculate direction from target to explosion center
			FVector PullDirection = (ExplosionLocation - TargetLocation).GetSafeNormal();

			// Draw line trace from explosion center to grabbed enemy
			DrawDebugLine(
				GetWorld(),
				ExplosionLocation,
				TargetLocation,
				FColor::Red,
				false,
				3.0f, // Duration
				0,
				5.0f // Thickness
			);

			// Apply pull force to the actor
			UPrimitiveComponent* TargetPrimitive = TargetActor->FindComponentByClass<UPrimitiveComponent>();
			if (TargetPrimitive && TargetPrimitive->IsSimulatingPhysics())
			{
				// For physics-based actors, apply impulse
				FVector PullImpulse = PullDirection * PullForce;
				TargetPrimitive->AddImpulse(PullImpulse, NAME_None, true);
			}
			else
			{
				// For character movement, try to use character movement component
				ACharacter* TargetCharacter = Cast<ACharacter>(TargetPawn);
				if (TargetCharacter && TargetCharacter->GetCharacterMovement())
				{
					// Apply velocity change to character movement
					FVector PullVelocity = PullDirection * (PullForce / TargetCharacter->GetCharacterMovement()->Mass);
					TargetCharacter->GetCharacterMovement()->AddImpulse(PullVelocity, true);
				}
			}

			// Optional: Add visual effect on grabbed targets
			// UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), GrabEffect, TargetLocation);
		}
	}

	// Spawn explosion visual effect if you have one
	// UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), AltExplosionEffect, ExplosionLocation);

	// Play explosion sound if you have one
	// UGameplayStatics::PlaySoundAtLocation(GetWorld(), AltExplosionSound, ExplosionLocation);

	// Debug visualization of explosion radius (blue for alt fire)
	if (GEngine)
	{
		DrawDebugSphere(GetWorld(), ExplosionLocation, ExplosionRadius, 12, FColor::Blue, false, 3.0f);
		// Debug visualization of grab radius (green)
		DrawDebugSphere(GetWorld(), ExplosionLocation, GrabRadius, 16, FColor::Green, false, 3.0f);
	}

	// Destroy the projectile after explosion
	Destroy();
}

void AGrenadeLauncherProjectile::GrenadeFireInDirection(const FVector& ShootDirection)
{
	// Set the projectile's velocity in the specified direction
	if (GrenadeProjectileMovement)
	{
		GrenadeProjectileMovement->Velocity = ShootDirection * GrenadeProjectileMovement->InitialSpeed;
	}

	// Optional: Set the projectile's rotation to face the direction it's traveling
	if (ShootDirection.SizeSquared() > 0)
	{
		FRotator NewRotation = ShootDirection.Rotation();
		SetActorRotation(NewRotation);
	}
}

void AGrenadeLauncherProjectile::SetAltFireMode(bool bAltFire)
{
	bIsAltFire = bAltFire;
}



