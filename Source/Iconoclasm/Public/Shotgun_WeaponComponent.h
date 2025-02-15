// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TP_WeaponComponent.h"
#include "HealthComponent.h"
#include "ShotgunHUD.h"
#include "Shotgun_WeaponComponent.generated.h"

UENUM(BlueprintType)
enum class EShotgunMode : uint8
{
	ShotgunMode1 UMETA(DisplayName = "Teleport"),
	ShotgunMode2 UMETA(DisplayName = "Defcon"),
};

/**
 * 
 */
UCLASS()
class ICONOCLASM_API UShotgun_WeaponComponent : public UTP_WeaponComponent
{
	GENERATED_BODY()

public:
	/** Sets default values for this component's properties */
	UShotgun_WeaponComponent();


public:

	virtual void BeginPlay() override;

	
	virtual void AttachWeapon(AIconoclasmCharacter* TargetCharacter) override;

	
	virtual void Fire() override;

	virtual void AltFire() override;

	virtual void SwitchFireMode() override;

	virtual void PerformHitscan(FVector& ImpactLocation) override;

	void PerformTeleportHitscan(FVector& ImpactLocation, FHitResult& OutHit);

	UFUNCTION(BlueprintCallable)
	void PerformPumpHitscan(FVector& StartLocation, FVector& EndLocation, int32 MaxRicochets);

	void TeleportPlayer(FVector TeleportLocation);

	virtual void DetachFromCharacter() override;

protected:
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:


	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UShotgunHUD> ShotgunHUDClass;

	UShotgunHUD* ShotgunHUDInstance;

	/** The Character holding this weapon*/
	AIconoclasmCharacter* Character;

	EShotgunMode CurrentWeaponMode;

	FTimerHandle FireCooldownTimerHandle;
	int32 HitscanCount;

	bool bCanUseAltTimeWarp = true;
	bool bCanUseAltDefcon = true;
	float AltTimeWarpCooldownDuration = 5.0f; // Example cooldown in seconds
	float AltDefconCooldownDuration = 7.0f;
	// Cooldown progress
	float AltTimeWarpProgress = 1.0f;
	float AltDefconProgress = 1.0f;

	float ShotgunDamage = 100.0f;

	bool bCanFireTimeWarp = true;
	bool bCanFireDefcon = true;
	float TimeWarpCooldown = 2.0f; // Set desired cooldown time
	float DefconCooldown = 2.0f;

	// Cooldown durations
	float AltTimeWarpCooldown = 5.0f;
	float AltDefconCooldown = 7.0f;

	FTimerHandle AltDefconCooldownTimer;
	FTimerHandle AltTimeWarpCooldownTimer;

	FTimerHandle TimeWarpCooldownTimer;
	FTimerHandle DefconCooldownTimer;

	// Timers for cooldowns
	FTimerHandle AltTimeWarpTimerHandle;
	FTimerHandle AltDefconTimerHandle;

	bool CanFire;

	void TimeWarpMode();

	void AltTimeWarpMode();

	void DefconMode();

	void AltDefconMode();

	void UpdateCooldowns();

	void ResetTimeWarpCooldown();

	void ResetDefconCooldown();


	struct FPlayerStae
	{
		FVector Location;
		FRotator Rotation;
	};
	
};
