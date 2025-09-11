// Fill out your copyright notice in the Description page of Project Settings.


#include "ShotgunProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "IconoclasmCharacter.h"
#include "Engine/DamageEvents.h"

// Sets default values
AShotgunProjectile::AShotgunProjectile()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // Use a sphere as a simple collision representation
    ShotgunCollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    ShotgunCollisionComp->InitSphereRadius(5.0f);
    ShotgunCollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
    ShotgunCollisionComp->OnComponentHit.AddDynamic(this, &AShotgunProjectile::ShotgunOnHit);
    // Players can't walk on it
    ShotgunCollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
    ShotgunCollisionComp->CanCharacterStepUpOn = ECB_No;
    // Set as root component
    RootComponent = ShotgunCollisionComp;

    // Create damage radius sphere component
    DamageRadiusComp = CreateDefaultSubobject<USphereComponent>(TEXT("DamageRadiusComp"));
    DamageRadiusComp->InitSphereRadius(DamageRadius);
    DamageRadiusComp->SetupAttachment(RootComponent);
    DamageRadiusComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DamageRadiusComp->SetCollisionProfileName("NoCollision");
    DamageRadiusComp->SetHiddenInGame(false); // Set to true if you don't want to see it in game
    DamageRadiusComp->SetVisibility(true); // For debugging - set to false in final build

    // Use a ProjectileMovementComponent to govern this projectile's movement
    ShotgunProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
    ShotgunProjectileMovement->UpdatedComponent = ShotgunCollisionComp;
    ShotgunProjectileMovement->InitialSpeed = 3000.f;
    ShotgunProjectileMovement->MaxSpeed = 10000.f; // Increased to allow speed boosts
    ShotgunProjectileMovement->bRotationFollowsVelocity = true;
    ShotgunProjectileMovement->bShouldBounce = true;
    // DISABLE GRAVITY for straight-line movement
    ShotgunProjectileMovement->ProjectileGravityScale = 0.0f;
    // Set bounce properties
    ShotgunProjectileMovement->Bounciness = 1.0f; // Perfect bounce
    ShotgunProjectileMovement->Friction = 0.0f;   // No friction loss

    // Initialize bounce counter
    BounceCount = 0;
    MaxBounces = 10;
    SpeedMultiplierPerBounce = 1.1f; // 10% speed increase per bounce

    // Set maximum lifespan to 20 seconds
    InitialLifeSpan = 20.0f;

    // Initialize damage properties
    DamageAmount = 50000.0f;
    DamageRadius = 100.0f;
    bDamageOnEachBounce = true;

    // Initialize player character reference
    PlayerCharacter = nullptr;
}

void AShotgunProjectile::ShotgunOnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    // Apply damage on hit if enabled
    if (bDamageOnEachBounce)
    {
        ApplyDamageInRadius(GetActorLocation());
    }

    // Check if we've reached max bounces
    if (BounceCount >= MaxBounces)
    {
        // Apply damage one final time if we haven't been doing it on each bounce
        if (!bDamageOnEachBounce)
        {
            ApplyDamageInRadius(GetActorLocation());
        }

        // Destroy the projectile after max bounces
        Destroy();
        return;
    }

    // Increment bounce counter
    BounceCount++;

    // Increase speed after each bounce
    float CurrentSpeed = ShotgunProjectileMovement->Velocity.Size();
    float NewSpeed = CurrentSpeed * SpeedMultiplierPerBounce;

    // Apply new speed while maintaining direction
    FVector Direction = ShotgunProjectileMovement->Velocity.GetSafeNormal();
    ShotgunProjectileMovement->Velocity = Direction * NewSpeed;

    // Update damage radius sphere size (in case it was changed in editor)
    DamageRadiusComp->SetSphereRadius(DamageRadius);

    // Optional: Add some visual/audio feedback for bounces
    UE_LOG(LogTemp, Warning, TEXT("Projectile bounced! Bounce count: %d, New speed: %f"), BounceCount, NewSpeed);
}

void AShotgunProjectile::ShotgunFireInDirection(const FVector& ShootDirection)
{
    // Set the projectile's velocity in the specified direction
    ShotgunProjectileMovement->Velocity = ShootDirection * ShotgunProjectileMovement->InitialSpeed;

    // Reset bounce counter when firing
    BounceCount = 0;

    // Update damage radius sphere size
    DamageRadiusComp->SetSphereRadius(DamageRadius);
}

void AShotgunProjectile::ApplyDamageInRadius(const FVector& Origin)
{
    // Get all actors within damage radius
    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(this); // Ignore the projectile itself

    // Add the projectile's owner to ignore list if it exists
    if (GetOwner())
    {
        ActorsToIgnore.Add(GetOwner());
    }

    // Add the player character to ignore list if it exists
    if (PlayerCharacter)
    {
        ActorsToIgnore.Add(PlayerCharacter);
    }

    TArray<AActor*> HitActors;
    bool bHit = UKismetSystemLibrary::SphereOverlapActors(
        GetWorld(),
        Origin,
        DamageRadius,
        TArray<TEnumAsByte<EObjectTypeQuery>>(), // Empty array means all object types
        APawn::StaticClass(), // Only get pawns (characters)
        ActorsToIgnore,
        HitActors
    );

    if (bHit)
    {
        for (AActor* HitActor : HitActors)
        {
            if (HitActor && HitActor->CanBeDamaged())
            {
                // Double-check that this isn't the player character
                if (HitActor != PlayerCharacter && HitActor != GetOwner())
                {
                    if (ACharacter* CharacterHit = Cast<ACharacter>(HitActor))
                    {
                        // Proper damage event
                        FPointDamageEvent DamageEvent;
                        DamageEvent.Damage = DamageAmount;
                        DamageEvent.HitInfo.ImpactPoint = CharacterHit->GetActorLocation();
                        DamageEvent.ShotDirection = (CharacterHit->GetActorLocation() - Origin).GetSafeNormal();

                        // Get controller from player character
                        AController* PlayerController = PlayerCharacter ? PlayerCharacter->GetController() : nullptr;

                        // Apply damage
                        CharacterHit->TakeDamage(DamageAmount, DamageEvent, PlayerController, this);

                        UE_LOG(LogTemp, Warning, TEXT("Applied %f damage to %s"), DamageAmount, *CharacterHit->GetName());
                    }
                }
            }
        }
    }

    // Optional: Draw debug sphere to visualize damage radius
    if (GetWorld())
    {
        DrawDebugSphere(GetWorld(), Origin, DamageRadius, 12, FColor::Red, false, 1.0f);
    }


}

void AShotgunProjectile::SetPlayerCharacter(ACharacter* InPlayerCharacter)
{
    PlayerCharacter = InPlayerCharacter;
}



