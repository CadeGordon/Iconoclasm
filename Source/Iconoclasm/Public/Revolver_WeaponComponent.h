// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TP_WeaponComponent.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "Revolver_WeaponComponent.generated.h"

UENUM(BlueprintType)
enum class ERevolverMode : uint8
{
	RevolverMode1 UMETA(DisplayName = "Gunslinger"),
	RevolverMode2 UMETA(DisplayName = "Hellfire"),
};

/**
 * 
 */
UCLASS()
class ICONOCLASM_API URevolver_WeaponComponent : public UTP_WeaponComponent
{
	GENERATED_BODY()
	
public:

	/** Sets default values for this component's properties */
	URevolver_WeaponComponent();

	
	virtual void Fire() override;
	
	virtual void AltFire() override;
	
	virtual void SwitchFireMode() override;

	virtual void AttachWeapon(AIconoclasmCharacter* TargetCharacter) override;

	virtual void DetachFromCharacter() override;

	virtual void PerformHitscan(FVector& ImpactLocation) override;



	// Fire mode particle systems
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* GunslingerParticle;

	// Fire mode particle systems
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* RevolverShotParticle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* AltGunslingerParticle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* HellfireParticle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* AltHellfireParticle;

	UNiagaraComponent* NiagaraComp;

	UPROPERTY()
	class URevolverHUD* RevolverHUD;

	// Dash HUD reference
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> RevolverHUDClass;

protected:
	/** Ends gameplay for this component. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	// Cooldown flags
	bool bCanFireAltGunslinger = true;
	bool bCanFireAltHellfire = true;

	float ElapsedTime = 0.0f; // Track the elapsed time during cooldown
	float CooldownDuration = 2.0f; // Duration of the cooldown
	FTimerHandle CooldownTimerHandle; // Timer handle for the cooldown

	// Timer handle for updating the cooldown progress of AltHellfire
	FTimerHandle TimerHandle_AltHellfireProgress;

	// Variable to track the elapsed time for the AltHellfire cooldown
	float ElapsedHellfireTime = 0.0f;

	FTimerHandle GunslingerCooldownTimer;
	FTimerHandle HellfireCooldownTimer;

	bool bCanFireGunslinger = true;
	bool bCanFireHellfire = true;

	float GunslingerCooldown = 0.5f;
	float HellfireCooldown = 0.5f;

	// Timers for cooldowns
	FTimerHandle TimerHandle_AltGunslingerCooldown;
	FTimerHandle TimerHandle_AltHellfireCooldown;
	FTimerHandle TimerHandle_HellfireEffect;
	FTimerHandle TimerHandle_HellfireStop;

	

	ERevolverMode CurrentWeaponMode;

	FTimerHandle TimerHandle_AltGunslingerFire;
	int32 HitscanCount;

	FTimerHandle TimerHandle_AltHellfire;
	float HellfireDuration;

	// Charging system variables
	UPROPERTY()
	bool bIsChargingShot = false;

	UPROPERTY()
	float ChargeStartTime = 0.0f;

	UPROPERTY()
	float CurrentChargeLevel = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charging", meta = (AllowPrivateAccess = "true"))
	float MaxChargeTime = 3.0f; // Time to reach 100% charge

	UPROPERTY()
	FTimerHandle ChargeTimerHandle;

	UPROPERTY()
	FTimerHandle TraceVisualizationHandle;

	UFUNCTION(BlueprintCallable)
	void GunslingerMode();
	UFUNCTION(BlueprintCallable)
	void AltGunslingerMode();
	UFUNCTION(BlueprintCallable)
	void HellfireMode();
	UFUNCTION(BlueprintCallable)
	void AltHellfireMode();

	void HandleGunslingerAltCooldown();

	void HandleHellfireAltCooldown();

	void ResetGunslingerCooldown();

	void ResetHellfireCooldown();

	//UFUNCTION()
	//TArray<AActor*> FindNearestEnemies(const FVector& Location, int32 MaxEnemies);

	TArray<AActor*> FindNearestEnemiesInRadius(const FVector& Location, int32 MaxEnemies, float SearchRadius);

	// Private helper functions
	void StartChargingShot();

	void UpdateCharge();

	void UpdateChargeTrace();

	FLinearColor GetChargeTraceColor(float ChargePercent);

	void ReleaseChargedShot();

	float GetDamageMultiplier(float ChargePercent);

	void FireChargedShot(float DamageAmount);

	void PlayChargedShotEffects(float ChargeLevel);

	public:
		// Input handling functions - call these from your input bindings
		UFUNCTION(BlueprintCallable, Category = "Weapon")
		void OnAltFirePressed();

		UFUNCTION(BlueprintCallable, Category = "Weapon")
		void OnAltFireReleased();

		// Optional: Getter functions for blueprints
		UFUNCTION(BlueprintPure, Category = "Weapon")
		bool IsChargingShot() const { return bIsChargingShot; }

		UFUNCTION(BlueprintPure, Category = "Weapon")
		float GetCurrentChargeLevel() const { return CurrentChargeLevel; }

		UFUNCTION(BlueprintPure, Category = "Weapon")
		float GetChargePercentage() const { return CurrentChargeLevel * 100.0f; }

		// In the public section:
		UFUNCTION(BlueprintCallable, Category = "Weapon")
		void UnlockHellfireMode();

		UFUNCTION(BlueprintPure, Category = "Weapon")
		bool IsHellfireModeUnlocked() const { return bHellfireModeUnlocked; }

		// Widget class for the unlock UI
		UPROPERTY(EditDefaultsOnly, Category = "UI")
		TSubclassOf<class UUserWidget> HellfireUnlockWidgetClass;

		// Reference to the unlock widget instance
		UPROPERTY()
		class UUserWidget* HellfireUnlockWidget;

		// In the private section:
		UPROPERTY()
		bool bHellfireModeUnlocked = false;

		/** The Character holding this weapon*/
		AIconoclasmCharacter* Character;

		// Static function to get the revolver component from any actor (usually the player)
		UFUNCTION(BlueprintPure, Category = "Weapon", meta = (WorldContext = "WorldContextObject"))
		static URevolver_WeaponComponent* GetRevolverComponentFromPlayer(const UObject* WorldContextObject);

		// Helper function to show/hide unlock widget
		//void UpdateUnlockWidgetVisibility();
};
