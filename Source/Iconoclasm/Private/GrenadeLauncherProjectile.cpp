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
	// Get the hit location for damage radius center
	FVector ExplosionLocation = Hit.Location;

	// Define explosion parameters
	float ExplosionRadius = 500.0f;
	float BaseDamage = 100.0f;
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

	//// Apply radial impulse to push objects away from explosion
	//UGameplayStatics::ApplyRadialImpulse(
	//	GetWorld(),
	//	ExplosionLocation,
	//	ExplosionRadius,
	//	2000.0f, // Impulse strength
	//	ERadialImpulseFalloff::RIF_Linear,
	//	true // Velocity change
	//);

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
	float BaseDamage = 150.0f; // Higher damage for alt fire
	float MinimumDamage = 30.0f;

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

	//// Apply stronger radial impulse for alt fire
	//UGameplayStatics::ApplyRadialImpulse(
	//	GetWorld(),
	//	ExplosionLocation,
	//	ExplosionRadius,
	//	3000.0f, // Stronger impulse for alt fire
	//	ERadialImpulseFalloff::RIF_Linear,
	//	true // Velocity change
	//);

	// Spawn explosion visual effect if you have one
	// UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), AltExplosionEffect, ExplosionLocation);

	// Play explosion sound if you have one
	// UGameplayStatics::PlaySoundAtLocation(GetWorld(), AltExplosionSound, ExplosionLocation);

	// Debug visualization of explosion radius (blue for alt fire)
	if (GEngine)
	{
		DrawDebugSphere(GetWorld(), ExplosionLocation, ExplosionRadius, 12, FColor::Blue, false, 3.0f);
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



