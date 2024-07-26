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

        // Create a radial force component
        FVector ImpactLocation = GetActorLocation();
        float ExplosionRadius = 300.0f; // Set the radius for the explosion
        float ExplosionStrength = 2000.0f; // Set the strength of the explosion

        // Draw debug sphere to visualize the radius
        DrawDebugSphere(GetWorld(), ImpactLocation, ExplosionRadius, 32, FColor::Red, false, 2.0f);

        // Create and configure the radial force component
        URadialForceComponent* RadialForce = NewObject<URadialForceComponent>(this);
        RadialForce->SetupAttachment(RootComponent);
        RadialForce->Radius = ExplosionRadius;
        RadialForce->ImpulseStrength = ExplosionStrength;
        RadialForce->bImpulseVelChange = true;
        RadialForce->AddCollisionChannelToAffect(ECC_Pawn); // Ensure it affects characters
        RadialForce->RegisterComponent(); // Register the component to properly initialize it
        RadialForce->FireImpulse(); // Apply the radial impulse

        // Apply force to any player within the radius
        TArray<AActor*> OverlappingActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), OverlappingActors);

        for (AActor* Actor : OverlappingActors)
        {
            if (ACharacter* Character = Cast<ACharacter>(Actor))
            {
                if (FVector::Dist(Character->GetActorLocation(), ImpactLocation) <= ExplosionRadius)
                {
                    FVector LaunchDirection = (Character->GetActorLocation() - ImpactLocation).GetSafeNormal();
                    Character->LaunchCharacter(LaunchDirection * ExplosionStrength, true, true);
                }
            }
        }
    }
}

void AIconoclasmProjectile::AltOnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if ((OtherActor != nullptr) && (OtherActor != this))
    {
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
