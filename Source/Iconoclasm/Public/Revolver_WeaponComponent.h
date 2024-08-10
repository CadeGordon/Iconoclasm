// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TP_WeaponComponent.h"
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

protected:
	/** Ends gameplay for this component. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The Character holding this weapon*/
	AIconoclasmCharacter* Character;

	ERevolverMode CurrentWeaponMode;

	FTimerHandle TimerHandle_AltGunslingerFire;
	int32 HitscanCount;

	FTimerHandle TimerHandle_AltHellfire;
	float HellfireDuration;

	void GunslingerMode();
	void AltGunslingerMode();
	void HellfireMode();
	void AltHellfireMode();


};
