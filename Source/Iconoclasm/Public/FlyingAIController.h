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
	// Player pawn reference
	APawn* PlayerPawn;

	FVector RandomFlyDirection;

	// Movement parameters
	float MaxFollowDistance = 2000.0f;  // Max distance to start moving toward the player
	float MinFollowDistance = 600.0f;   // Min distance to stop approaching the player and hover around
	float HoverSpeed = 300.0f;          // Speed at which the enemy hovers around the player


	UPROPERTY(EditAnywhere, Category = "AI")
	float FlySpeed = 600.0f;

	UPROPERTY(EditAnywhere, Category = "AI")
	float FlyRadius = 1000.0f; // Max distance to fly in one direction before changing

	UPROPERTY(EditAnywhere, Category = "AI")
	float ChangeDirectionInterval = 3.0f; // Time interval between direction changes

	float TimeSinceLastDirectionChange;

	void FlyAround();
	void ChangeFlyDirection();

	void AttackPlayer();

	// Function to make the enemy hover or circle around the player
	//void HoverAroundPlayer(const FVector& PlayerLocation, const FVector& EnemyLocation, float DeltaTime);


	
};
