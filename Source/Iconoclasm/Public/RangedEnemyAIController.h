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
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

protected:
    UPROPERTY(EditAnywhere, Category = "AI")
    float MinimumDistanceFromPlayer = 600.0f; // Desired distance to maintain

    UPROPERTY(EditAnywhere, Category = "AI")
    float MaximumDistanceFromPlayer = 1000.0f; // Maximum range from player

    UPROPERTY(EditAnywhere, Category = "AI")
    float ShootingCooldown = 1.0f; // Time between shots

    UPROPERTY(EditAnywhere, Category = "AI")
    float MoveRadius = 200.0f; // Distance to move after shooting

private:
    APawn* PlayerPawn;
    FTimerHandle ShootingTimerHandle;

    void FireAtPlayer();
    void Reposition();
};
