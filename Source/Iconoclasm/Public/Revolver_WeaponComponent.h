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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* AltGunslingerParticle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* HellfireParticle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* AltHellfireParticle;

	UNiagaraComponent* NiagaraComp;

protected:
	/** Ends gameplay for this component. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	// Cooldown flags
	bool bCanFireAltGunslinger = true;
	bool bCanFireAltHellfire = true;

	// Timers for cooldowns
	FTimerHandle TimerHandle_AltGunslingerCooldown;
	FTimerHandle TimerHandle_AltHellfireCooldown;
	FTimerHandle TimerHandle_HellfireEffect;
	FTimerHandle TimerHandle_HellfireStop;

	/** The Character holding this weapon*/
	AIconoclasmCharacter* Character;

	ERevolverMode CurrentWeaponMode;

	FTimerHandle TimerHandle_AltGunslingerFire;
	int32 HitscanCount;

	FTimerHandle TimerHandle_AltHellfire;
	float HellfireDuration;

	UFUNCTION(BlueprintCallable)
	void GunslingerMode();
	UFUNCTION(BlueprintCallable)
	void AltGunslingerMode();
	UFUNCTION(BlueprintCallable)
	void HellfireMode();
	UFUNCTION(BlueprintCallable)
	void AltHellfireMode();
};
