// Copyright Epic Games, Inc. All Rights Reserved.

#include "IconoclasmProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

AIconoclasmProjectile::AIconoclasmProjectile() 
{
	// Use a sphere as a simple collision representation
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
	CollisionComp->OnComponentHit.AddDynamic(this, &AIconoclasmProjectile::OnHit);		// set up a notification for when this component hits something blocking

	// Players can't walk on it
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComp;

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 3000.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;

	// Die after 3 seconds by default
	InitialLifeSpan = 3.0f;
}

void AIconoclasmProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if ((OtherActor != nullptr) && (OtherActor != this))
    {
        // Destroy the projectile
        Destroy();

        // Explosion parameters
        FVector ImpactLocation = GetActorLocation();
        float ExplosionRadius = 300.0f; // Radius for the explosion
        float BaseDamage = 100.0f;      // Base damage for the explosion
        float DamageFalloff = 0.0f;     // Optional damage falloff, 0 for no falloff

        // Draw debug sphere to visualize the radius
        DrawDebugSphere(GetWorld(), ImpactLocation, ExplosionRadius, 32, FColor::Red, false, 2.0f);

        // Apply radial damage
        UGameplayStatics::ApplyRadialDamage(
            GetWorld(),                 // World context
            BaseDamage,                 // Damage to apply at the epicenter
            ImpactLocation,             // Explosion location
            ExplosionRadius,            // Radius of the explosion
            UDamageType::StaticClass(), // Damage type
            TArray<AActor*>(),          // Actors to ignore (empty array to include all)
            this,                       // Damage causer (this projectile)
            GetInstigatorController(),  // Instigator's controller (usually the shooter)
            true,                       // Do full damage even if there is line-of-sight obstruction
            ECC_Visibility              // Collision channel for line-of-sight checks
        );

        UE_LOG(LogTemp, Warning, TEXT("Explosion at location: %s"), *ImpactLocation.ToString());
    }
}

void AIconoclasmProjectile::AltOnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if ((OtherActor != nullptr) && (OtherActor != this))
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

   
}
