// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "GrappleComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ICONOCLASM_API UGrappleComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	// Sets default values for this component's properties
	UGrappleComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void FireGrapple();

	UFUNCTION(BlueprintCallable)
	void ReleaseGrapple();

private:
	void PullCharacterToLocation(const FVector& Location);

	void ResetGrappleCooldown();

public:
	UPROPERTY(EditAnywhere, Category = "Grapple")
	float GrappleLength = 5000.0f;

	UPROPERTY(EditAnywhere, Category = "Grapple")
	float GrappleSpeed = 8000.0f;

	float GrappleSwingSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple")
	float GrappleCooldownDuration = 3.0f;

	float GrappleEndThreshold = 100.0f;

	bool GrappleOnCooldown = false;

	FTimerHandle GrappleCooldownTimerHandle;

	//variables for swining
	FVector SwingStartLocation;
	FVector SwingStartDirection;
	FVector SwingAxis;
	float SwingSpeed = 100.0f;
	
	UPROPERTY(BlueprintReadOnly)
	bool IsGrappleActive;


private:

	UPROPERTY(EditAnywhere, Category = "Grapple")
	bool bUseVelocityChange = true;

	UPROPERTY()
	ACharacter* OwningCharacter;

	
	FVector GrappleLocation;

	// Variables to store original and target FOV
	float OriginalFOV;
	float TargetFOV;

	float InterpSpeed;
	float CurrentFOV;
	float GrappleFOV;

	


	

	
	

	


		
};
