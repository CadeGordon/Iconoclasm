// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IconoclasmProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Actor.h"
#include "HomingProjectile.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API AHomingProjectile : public AIconoclasmProjectile
{
	GENERATED_BODY()

public:
    AHomingProjectile();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Reference to the movement component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    UProjectileMovementComponent* ProjectileMovementComponent;

    // Scatter angle for initial randomness
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
    float ScatterAngle = 15.0f; // Adjust as necessary

    // Reference to the target (player)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
    AActor* TargetActor;

    // Homing speed
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
    float HomingSpeed = 500.0f;

    // Function to apply random scatter
    void ApplyScatter();

    // Function to start homing
    void StartHoming();
	
};
