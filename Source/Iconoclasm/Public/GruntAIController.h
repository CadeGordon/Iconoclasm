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
    // Player reference
    APawn* PlayerPawn;

    // Attack system
    bool bCanAttack;
    float AttackCooldown;
    FTimerHandle AttackCooldownTimerHandle;
    void AttackPlayer();
    void ResetAttack();

    // Jump to airborne player system
    bool bCanJump;
    float JumpCooldown;
    FTimerHandle JumpCooldownTimerHandle;
    void TryJumpToPlayer();
    void ResetJump();
    bool CalculateJumpVelocity(const FVector& StartLocation, const FVector& TargetLocation,
        float& OutLaunchSpeed, FVector& OutLaunchVelocity);

    // Lunge attack system
    bool bCanLunge;
    float LungeCooldown;
    FTimerHandle LungeCooldownTimerHandle;
    void TryLungeAtPlayer();
    void ResetLunge();
    float LungeChance;
    float NextLungeCheckTime;

    // NEW: Circle strafe behavior
    bool bIsCircling;
    float CircleDirection; // 1.0 = clockwise, -1.0 = counter-clockwise
    float CircleDuration;
    float CircleTimer;
    void TryCircleStrafe();
    void StopCircling();

    // NEW: Dodge behavior (sidestep when player attacks)
    bool bCanDodge;
    float DodgeCooldown;
    FTimerHandle DodgeCooldownTimerHandle;
    void TryDodge();
    void ResetDodge();
    FVector LastPlayerForward;

    // NEW: Feint/fake lunge behavior
    bool bCanFeint;
    float FeintCooldown;
    FTimerHandle FeintCooldownTimerHandle;
    void TryFeint();
    void ResetFeint();

    // NEW: Pack coordination
    void CheckPackBehavior();
    bool ShouldHangBack(); // Stay back if too many allies are close
    int32 GetNearbyAlliesCount(float Radius);

    // NEW: Retreat when low health (if you add health to grunts)
    bool bIsRetreating;
    float RetreatTimer;
    void CheckRetreat();

    // Flanking system
    FVector TargetFlankPosition;
    float MyFlankAngle;
    float FlankDistance;
    float RepositionTimer;
    void CalculateFlankPosition();
    float FindBestFlankAngle();
    bool IsPositionOccupied(const FVector& Position, float Radius);

    // Movement
    void MoveToPlayer();

    // NEW: Personality traits (set randomly per grunt)
    float Aggression; // 0.0 - 1.0, affects behavior chances
    float Caution; // 0.0 - 1.0, affects retreat/dodge behavior
    bool bIsAlpha; // Alpha grunts are more aggressive
};
