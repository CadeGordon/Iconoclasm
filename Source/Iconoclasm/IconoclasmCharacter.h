// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "IconoclasmCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AIconoclasmCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* MoveAction;
	
public:
	AIconoclasmCharacter();

protected:
	virtual void BeginPlay();
	virtual void Tick(float DeltaTime) override;

public:
		
	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	/** Bool for AnimBP to switch to another animation set */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	bool bHasRifle;

	/** Setter to set the bool */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	void SetHasRifle(bool bNewHasRifle);

	/** Getter for the bool */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	bool GetHasRifle();

	UFUNCTION(BlueprintCallable, Category = "Character")
	void DoubleJump();

	UFUNCTION(BlueprintCallable, Category = "Character")
	void Dash();

	UFUNCTION(BlueprintCallable, Category = "Character")
	void StartWallRun(const FVector& WallNormal);

	UFUNCTION(BlueprintCallable, Category = "Character")
	void StopWallRun();

	UFUNCTION(BlueprintCallable, Category = "Character")
	void CheckForWalls();


	// Function to handle cooldown
	void StartDashCooldown();

	// Function to reset cooldown
	void ResetDashCooldown();

	// Function to handle end of dash
	void EndDash();

	// Function to reset jump count when the character lands
	virtual void Landed(const FHitResult& Hit) override;

	

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	int32 JumpCount;

	// Variables for dash
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	int32 DashCharges;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	float DashCooldown;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	bool CanDash;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	bool IsDashing;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	bool CanDashAgain; // New flag to allow immediate dash in a different direction

	// Timer handle for cooldown
	FTimerHandle DashCooldownTimerHandle;

	//Varibales for wallrun
	bool IsWallRunning;
	float WallRunDuration;
	float WallRunSpeed;
	float WallDetectionRange;
	float WallRunMaxAngle;

	FVector WallRunDirection;



	


};

