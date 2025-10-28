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

	UPROPERTY(EditAnywhere, BlueprintReadWrite , Category = "UI")
	TSubclassOf<UUserWidget> HealthWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UHealthComponent* HealthComponent;

	UPROPERTY()
	class UPlayerHealthBarHUD* HealthWidget;
	
	UPROPERTY()
	class UWheelHUD* WeaponWheelWidget;

	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Score")
	class UScoreComponent* ScoreComponent;

	void EquipRevolver();

	void EquipShotgun();

	void EquipGrenadeLauncher();

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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Respawn")
	FVector LastSafeLocation;

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

	void ShowDeathScreen();

	public:
		void SetCheckpointLocation(FVector NewLocation);

		FVector GetCheckpointLocation() const;

		bool IsCheckpointActive() const;

		void RestoreFullHealth();

		void ResetJumpCount();

		// And this function declaration:
		void CheckGroundSlamImpact();

		UFUNCTION()
		UHealthComponent* GetHealthComponent() const;

		// Wall Jump Functions
		void WallJump();
		bool CanPerformWallJump(FVector& OutWallNormal);
		void ResetWallJumpCooldown();
	
	


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

	UPROPERTY()
	FTimerHandle GroundSlamTimerHandle;

	FVector CheckpointLocation;
	bool bCheckpointActive = false;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dash")
	float LastDashTime = -999.f;  // initialized far in the past

	bool bLastAttackWasMelee = false;

	bool bLastAttackWasShatterShot = false;

	bool bLastAttackWasChargedShot = false;

	bool bLastAttackWasSlam = false;

	bool bLastKillWasSlam = false;

	bool bLastAttackWasBoomStick = false;
	float BoomStickKillWindow = 2.0f; // Time window in seconds for bonus
	float LastBoomStickTime = -10.0f; // Initialize outside the window

	bool bLastAttackWasTimeWarp = false;  // Flag for grenade alt-fire kill
	float TimeWarpKillWindow = 10.0f;     // Max duration before teleport expires
	float LastTimeWarpTime = -10.0f;      // Stores the time AltImpulseMode was activated

	bool bLastActionWasSlideJump = false;
	float SlideJumpKillWindow = 2.0f;   // 2 seconds window for kill
	float LastSlideJumpTime = -10.0f;   // Time double jump was performed

	
	bool bLastActionWasDoubleJump = false;
	float DoubleJumpKillWindow = 2.0f;   // 2 seconds window for kill
	float LastDoubleJumpTime = -10.0f;   // Time slide jump was performed

	// --- Cycle Weapon Kill Tracking ---
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Kill Bonuses")
	bool bLastActionWasCycleWeapon = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Kill Bonuses")
	float LastCycleWeaponTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Kill Bonuses")
	float CycleWeaponKillWindow = 2.0f; // 2-second window

	// For AltDefcon kills
	bool bLastAttackWasZeroPoint = false;



	FVector SlideDirection;

	UWallRunComponent* WallRunComponent;

	UPROPERTY()
	TArray<UTP_WeaponComponent*> WeaponInventory;

	int32 CurrentWeaponIndex;

	bool bCanMelee = true;
	FTimerHandle MeleeCooldownTimerHandle;
	float MeleeCooldownDuration = 1.0f;

	UPROPERTY()
	UDashHUD* DashHUD;

	// Dash HUD reference
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> DashHUDClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> DeathScreenWidgetClass;
	
	UPROPERTY()
	class UDeathScreenHUD* DeathScreenHUD;

	private:

		float MeleeRange = 200.0f; // Range of the melee attack
		float KnockbackStrength = 1000.0f;

		float TargetDashProgress; // The desired progress value
		float CurrentDashProgress; // The current progress value for interpolation
		float ProgressInterpSpeed; // The speed of interpolation

		bool bHasLeftGround = false;

		// Wall Jump Variables
		bool bCanWallJump = true;
		int32 WallJumpCount = 0;
		int32 MaxWallJumps = 4;
		float WallJumpCooldown = 0.3f;
		FTimerHandle WallJumpCooldownTimerHandle;

		float WallJumpUpwardForce = 1200.0f;
		float WallJumpBackwardForce = 1500.0f;
		float WallCheckDistance = 150.0f;
		float WallJumpAngleThreshold = 45.0f;

		bool bIsDeceleratingFromSlide;
		float CurrentSlideSpeed;
		float DefaultWalkSpeed;
		float SlideDecelerationRate; // How fast we slow down after sliding
		
};

