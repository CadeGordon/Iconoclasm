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
    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

private:
    APawn* PlayerPawn;
    FVector WanderLocation;
    bool IsPlayerInRange;

    void MoveToPlayer();
    void AttackPlayer();
    void Wander();
    void CheckPlayerProximity();
	
};
