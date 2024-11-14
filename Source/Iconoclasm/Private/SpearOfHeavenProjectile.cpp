// Fill out your copyright notice in the Description page of Project Settings.


#include "SpearOfHeavenProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Particles/ParticleSystem.h"
#include "DrawDebugHelpers.h"

// Sets default values
ASpearOfHeavenProjectile::ASpearOfHeavenProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    // Use a sphere as the collision representation
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
    CollisionComponent->InitSphereRadius(15.0f);
    CollisionComponent->SetCollisionProfileName(TEXT("BlockAll"));
    CollisionComponent->OnComponentHit.AddDynamic(this, &ASpearOfHeavenProjectile::OnHit);
    RootComponent = CollisionComponent;

    // Create and configure the movement component
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->UpdatedComponent = CollisionComponent;
    ProjectileMovement->InitialSpeed = 3000.f;
    ProjectileMovement->MaxSpeed = 3000.f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = true;

    // Set default values for explosion
    ExplosionRadius = 200.f;
    ExplosionDamage = 50.f;

}

// Called when the game starts or when spawned
void ASpearOfHeavenProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASpearOfHeavenProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASpearOfHeavenProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    // Explode when it hits something
    Explode();
}

void ASpearOfHeavenProjectile::Explode()
{
    // Play explosion effect
    if (ExplosionEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
    }

    // Apply damage to nearby actors
    TArray<AActor*> IgnoredActors;
    UGameplayStatics::ApplyRadialDamage(GetWorld(), ExplosionDamage, GetActorLocation(), ExplosionRadius, nullptr, IgnoredActors, this, GetInstigatorController(), true);

    // Draw debug sphere for explosion radius (optional for debugging)
    DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Red, false, 2.0f);

    // Destroy the projectile after it explodes
    Destroy();
}

