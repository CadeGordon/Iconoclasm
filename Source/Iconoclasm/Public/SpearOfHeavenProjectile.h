// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpearOfHeavenProjectile.generated.h"

UCLASS()
class ICONOCLASM_API ASpearOfHeavenProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpearOfHeavenProjectile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    // Function to handle projectile impact
    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    // Function to handle explosion
    void Explode();

    // Projectile movement component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    class UProjectileMovementComponent* ProjectileMovement;

    // Sphere collision component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
    class USphereComponent* CollisionComponent;

    // Particle system for explosion effect
    UPROPERTY(EditAnywhere, Category = "Effects")
    class UParticleSystem* ExplosionEffect;

    // Damage radius for explosion
    UPROPERTY(EditAnywhere, Category = "Explosion")
    float ExplosionRadius;

    // Damage amount for explosion
    UPROPERTY(EditAnywhere, Category = "Explosion")
    float ExplosionDamage;

};
