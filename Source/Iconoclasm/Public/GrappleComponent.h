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
	float GrappleLength = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Grapple")
	float GrappleSpeed = 5000.0f;

	UPROPERTY(EditAnywhere, Category = "Grapple")
	float GrappleReleaseForce = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Grapple")
	float GrappleCooldownDuration = 3.0f;

	bool HitWall;
	bool GrappleOnCooldown = false;

	FTimerHandle GrappleCooldownTimerHandle;



private:
	UPROPERTY()
	ACharacter* OwningCharacter;

	bool IsGrappleActive;
	FVector GrappleLocation;

		
};
