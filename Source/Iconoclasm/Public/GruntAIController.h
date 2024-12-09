// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GruntAIController.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API AGruntAIController : public AAIController
{
	GENERATED_BODY()

public:
    AGruntAIController();


    virtual void Tick(float DeltaTime) override;


protected:
    virtual void BeginPlay() override;

private:
    APawn* PlayerPawn;
    FVector WanderLocation;
    bool IsPlayerInRange;

    // Can the AI attack
    bool bCanAttack;

    // Cooldown duration for attacks
    float AttackCooldown;

    // Timer handle for attack cooldown
    FTimerHandle AttackCooldownTimerHandle;

    void MoveToPlayer();
    void AttackPlayer();

    // Reset attack ability after cooldown
    void ResetAttack();
};
