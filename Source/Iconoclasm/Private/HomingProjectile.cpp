// Fill out your copyright notice in the Description page of Project Settings.


#include "HomingProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AHomingProjectile::AHomingProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create the movement component
    ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovementComponent->bShouldBounce = false;
    ProjectileMovementComponent->ProjectileGravityScale = 0.0f;  // Make sure it's not affected by gravity
}

void AHomingProjectile::BeginPlay()
{
    Super::BeginPlay();

    // Apply random scatter to the projectile
    ApplyScatter();
}

void AHomingProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // If there's a target (player), start homing
    if (TargetActor)
    {
        StartHoming();
    }
}

void AHomingProjectile::ApplyScatter()
{
    // Scatter the projectile by a random angle within the defined range
    FRotator RandomScatter = FRotator(FMath::RandRange(-ScatterAngle, ScatterAngle), FMath::RandRange(-ScatterAngle, ScatterAngle), 0);
    FQuat ScatterRotation = FQuat(RandomScatter);

    // Apply this random scatter to the initial direction of the projectile
    ProjectileMovementComponent->Velocity = ScatterRotation.RotateVector(ProjectileMovementComponent->Velocity);
}

void AHomingProjectile::StartHoming()
{
    // Homing logic: Track the player (target)
    FVector DirectionToTarget = TargetActor->GetActorLocation() - GetActorLocation();
    DirectionToTarget.Normalize();

    // Update the velocity to start homing towards the player
    ProjectileMovementComponent->Velocity = DirectionToTarget * HomingSpeed;
}
