// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "FlyingAIController.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API AFlyingAIController : public AAIController
{
	GENERATED_BODY()
	

public:
	// Constructor
	AFlyingAIController();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
    // Player reference
    UPROPERTY()
    APawn* PlayerPawn;

    // Movement parameters
    UPROPERTY(EditAnywhere, Category = "AI")
    float PreferredDistance;  // Ideal distance to maintain from player

    UPROPERTY(EditAnywhere, Category = "AI")
    float MinDistance;  // If closer than this, actively flee

    UPROPERTY(EditAnywhere, Category = "AI")
    float FlySpeed;

    UPROPERTY(EditAnywhere, Category = "AI")
    float ChangeDirectionInterval;

    UPROPERTY(EditAnywhere, Category = "AI")
    float StrafeSpeed;

    // Evasive maneuver parameters
    UPROPERTY(EditAnywhere, Category = "AI")
    float EvasiveManeuverChance;

    UPROPERTY(EditAnywhere, Category = "AI")
    float EvasiveManeuverDuration;

    // Shooting parameters
    UPROPERTY(EditAnywhere, Category = "Combat")
    float ShootRange;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float TimeBetweenShots;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float BurstShotChance;  // 0.0 to 1.0, chance to do burst instead of single shot

    UPROPERTY(EditAnywhere, Category = "Combat")
    int32 BurstShotCount;  // Number of shots in a burst

    UPROPERTY(EditAnywhere, Category = "Combat")
    float TimeBetweenBurstShots;  // Delay between shots in a burst

    // Internal state
    float TimeSinceLastDirectionChange;
    float TimeSinceLastShot;
    bool bIsChasingPlayer;

    // Burst shooting state
    bool bIsBursting;
    int32 CurrentBurstCount;
    float TimeSinceLastBurstShot;

    // Evasive maneuver state
    bool bIsEvading;
    float EvasiveManeuverTimer;
    FVector CurrentEvasiveDirection;

    FVector RandomFlyDirection;

    // Smoothing parameters
    FVector CurrentVelocity;  // Current movement velocity
    FVector TargetDirection;  // Direction we want to move toward

    UPROPERTY(EditAnywhere, Category = "AI")
    float AccelerationRate;  // How fast to change velocity

    // AI behavior functions
    void FlyAround(float DeltaTime);
    void ChangeFlyDirection();
    void FleeFromPlayer(float DeltaTime);
    void MaintainDistance(float DeltaTime);
    void AttackPlayer();
    void ShootProjectile();
    void TriggerEvasiveManeuver();
    void ApplySmoothMovement(float DeltaTime);  // New - handles smooth interpolation

    // Helper function to avoid other enemies
    FVector AvoidNearbyEnemies();
};
