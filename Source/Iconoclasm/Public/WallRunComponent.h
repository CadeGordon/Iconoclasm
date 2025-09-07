// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WallRunComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ICONOCLASM_API UWallRunComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWallRunComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void StartWallRun();

	UFUNCTION(BlueprintCallable)
	void StopWallRun();

	void WallRun();

	bool IsWallRunning;

	void ResetWallRunCooldown();

	bool GetIsWallRunning() const { return IsWallRunning; }

private:
	bool DetectWall(FVector& OutWallNormal, FVector& OutWallDirection);

	void EndWallRun();

	void ResetWallJumpCooldown();

private:
	ACharacter* OwningCharacter;

	FVector WallNormal;
	FVector WallRunDirection;
	FVector InitialVelocity;

	float WallRunSpeed;
	float WallRunDuration;

	float DescentRate;

	// Cooldown duration in seconds
	float WallRunCooldownDuration = 0.5f;

	// Timer handle for the cooldown
	FTimerHandle WallRunCooldownTimerHandle;

	// Boolean flag to track if the cooldown is active
	bool WallRunCooldownActive = false;

	FTimerHandle WallRunTimerHandle;

	bool bWallJumped = false;
	float WallJumpCooldownDuration = 0.2f;
	FTimerHandle WallJumpCooldownHandle;


		
};
