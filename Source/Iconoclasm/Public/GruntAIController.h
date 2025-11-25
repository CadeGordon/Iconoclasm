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
    // Reference to the player pawn
    APawn* PlayerPawn;

    // Attack cooldown management
    bool bCanAttack;
    float AttackCooldown;
    FTimerHandle AttackCooldownTimerHandle;

    // Jump cooldown management
    bool bCanJump;
    float JumpCooldown;
    FTimerHandle JumpCooldownTimerHandle;

    // Functions
    void MoveToPlayer();
    void AttackPlayer();
    void ResetAttack();
    void TryJumpToPlayer();
    void ResetJump();
    bool CalculateJumpVelocity(const FVector& StartLocation, const FVector& TargetLocation, float& OutLaunchSpeed, FVector& OutLaunchVelocity);

};
