// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "RangedEnemyAIController.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API ARangedEnemyAIController : public AAIController
{
	GENERATED_BODY()
	
public:

    ARangedEnemyAIController();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

protected:
    // Reference to the player pawn
    APawn* PlayerPawn;

    // Counter for shots fired before repositioning
    int32 ShotsFired;

    // Maximum shots before the enemy repositions
    int32 MaxShotsBeforeReposition;

    // Minimum and maximum distances for repositioning
    float MinRepositionDistance = 300.0f;
    float MaxRepositionDistance = 800.0f;

    // Time between shots
    float ShootingCooldown = 2.0f;

    // Timer handle for managing shooting behavior
    FTimerHandle ShootingTimerHandle;

private:
    

    void FireAtPlayer();
    void Reposition();
};
