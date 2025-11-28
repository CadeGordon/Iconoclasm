// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "GrappleHUD.h"
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

	// Add these public functions to your GrappleComponent.h:
	UFUNCTION(BlueprintCallable, Category = "Grapple")
	void ExecuteGrabbedEnemy(); // Call this from your melee button input

	UFUNCTION(BlueprintCallable, Category = "Grapple")
	bool IsHoldingEnemy() const { return bIsHoldingEnemy; }

private:
	void PullCharacterToLocation(const FVector& Location);

	void ResetGrappleCooldown();

	// Function declarations:
	void CreateGrappleVisual();

	void ShowGrappleVisual();

	void HideGrappleVisual();

	void UpdateGrappleVisual();

	//UFUNCTION()
	//bool ShouldTransitionToSwing();

	UFUNCTION()
	void StartSwinging();

	//UFUNCTION()
	//void ApplySwingPhysics(float DeltaTime);

	UFUNCTION()
	void ApplyCombinedGrapplePhysics(float DeltaTime);

	UFUNCTION()
	void ApplySwingInput(FVector& CurrentVelocity, const FVector& ToGrapplePoint, float DeltaTime);

	void StartWorldGrapple();

	void StartEnemyGrapple();

	void ApplyEnemyGrapplePhysics(float DeltaTime);

	

	void GrabEnemy();

	void UpdateGrabbedEnemyPosition();

	

public:
	UPROPERTY(EditAnywhere, Category = "Grapple")
	float GrappleLength = 6500.0f;

	UPROPERTY(EditAnywhere, Category = "Grapple")
	float GrappleSpeed = 8000.0f;

	float GrappleSwingSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple")
	float GrappleCooldownDuration = 3.0f;

	float GrappleEndThreshold = 100.0f;

	bool GrappleOnCooldown = false;

	FTimerHandle GrappleCooldownTimerHandle;

	UPROPERTY(BlueprintReadOnly, Category = "Grapple")
	bool IsSwinging;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Swing")
	float SwingForce = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Swing")
	float MaxSwingSpeed = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Swing")
	float SwingDamping = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Swing")
	float GrappleReleaseThreshold = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Swing")
	float SwingTransitionSpeed = 1500.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Grapple")
	float GrappleDistance;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsGrappleActive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Swing")
	float MinimumPullForce = 1500.0f;

	UPROPERTY()
	bool bWasGroundedWhenGrappleStarted;

	UPROPERTY()
	class AActor* GrappledActor; // The actor we're grappling (if it's an enemy)

	UPROPERTY()
	class AActor* GrabbedEnemy;

	UPROPERTY()
	bool bIsHoldingEnemy = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple", meta = (AllowPrivateAccess = "true"))
	bool bCanGrappleEnemies = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple", meta = (AllowPrivateAccess = "true"))
	float EnemyPullForce = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple", meta = (AllowPrivateAccess = "true"))
	float EnemyGrappleRange = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple", meta = (AllowPrivateAccess = "true"))
	float EnemyGrappleEndThreshold = 500.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple", meta = (AllowPrivateAccess = "true"))
	float EnemyHoldDistance = 150.0f; // Distance in front of player to hold enemy

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple", meta = (AllowPrivateAccess = "true"))
	float EnemyHoldHeight = 50.0f; // Height offset for holding enemy



	UPROPERTY()
	class AStaticMeshActor* GrappleVisualActor;

	UPROPERTY()
	class UStaticMeshComponent* GrappleVisualMesh;

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

	UPROPERTY()
	class UGrappleHUD* GrappleHUD;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UUserWidget> GrappleHUDClass;

	float CooldownProgress;


	

	
	

	


		
};
