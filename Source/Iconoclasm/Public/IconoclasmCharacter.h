// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WallRunComponent.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "TP_WeaponComponent.h"
#include "DashHUD.h"
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
	
	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* SwitchAction;
	
	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* CycleWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MeleeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* WheelAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DisableWheelAction;
	
public:
	AIconoclasmCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> WeaponWheelWidgetClass;

	UPROPERTY()
	class UWheelHUD* WeaponWheelWidget;

	void EquipRevolver();

	void ToggleWeaponWheel();

	// New function to disable the weapon wheel
	void DisableWeaponWheel();

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

	UFUNCTION(BlueprintCallable, Category = "Slide")
	void StartSlide();

	UFUNCTION(BlueprintCallable, Category = "Slide")
	void UpdateSlide();

	UFUNCTION(BlueprintCallable, Category = "Slide")
	void StopSlide();

	UFUNCTION(BlueprintCallable, Category = "Slide")
	void SlideJump();

	UFUNCTION(BlueprintCallable, Category = "Slide")
	void GroundSlam();

	// Function to handle cooldown
	void StartDashCooldown();

	// Function to reset cooldown
	void ResetDashCooldown();

	UFUNCTION(BlueprintCallable)
	// Function to handle end of dash
	void EndDash();

	// Function to reset jump count when the character lands
	virtual void Landed(const FHitResult& Hit) override;

	UFUNCTION(BlueprintCallable)
	void EquipWeapon(UTP_WeaponComponent* Weapon);
	UFUNCTION(BlueprintCallable)
	void AddWeaponToInventory(UTP_WeaponComponent* Weapon);
	UFUNCTION(BlueprintCallable)
	void CycleWeapon();
	UFUNCTION(BlueprintCallable)
	bool HasWeaponEquipped() const;

	void PerformMelee();

	void UpdateDashProgress();

	

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

public:

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
	UPROPERTY(BlueprintReadOnly)
	bool IsDashingForward;

	float GroundDash;
	float AirDash;

	// Timer handle for cooldown
	FTimerHandle DashCooldownTimerHandle;

	//Variables for Sliding
	bool IsSliding;
	bool FirstSlideUpdate;
	float SlideSpeed;
	float SlideJumpBoostStrength;
	float GroundSlamStrength;

	//Slide FOV Variables
	float CurrentFOV; // Current field of view
	float TargetFOV; // Target field of view
	float InterpSpeed = 10.0f; // Interpolation speed
	float OriginalFOV = 110.0f; // Original FOV value
	float SlideFOV = 120.0f; // Adjusted FOV value when sliding

	//Dash FOV Variables
	float DashFOV = 120.0f;
	float DashInterp = 5.0f;


	FVector SlideDirection;

	UWallRunComponent* WallRunComponent;

	UPROPERTY()
	TArray<UTP_WeaponComponent*> WeaponInventory;

	int32 CurrentWeaponIndex;

	UPROPERTY()
	UDashHUD* DashHUD;

	// Dash HUD reference
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> DashHUDClass;

	


	private:

		float MeleeRange = 200.0f; // Range of the melee attack
		float KnockbackStrength = 1000.0f;

		float TargetDashProgress; // The desired progress value
		float CurrentDashProgress; // The current progress value for interpolation
		float ProgressInterpSpeed; // The speed of interpolation
		
};

