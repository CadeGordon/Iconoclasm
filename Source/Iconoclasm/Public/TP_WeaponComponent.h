// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "HealthComponent.h"
#include "TP_WeaponComponent.generated.h"

class AIconoclasmCharacter;

UENUM(BlueprintType)
enum class EWeaponMode : uint8
{
	Mode1 UMETA(DisplayName = "LifeBlood"),
	Mode2 UMETA(DisplayName = "AltMode"),
};

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ICONOCLASM_API UTP_WeaponComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AIconoclasmProjectile> ProjectileClass;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AGrenadeLauncherHealProjectile> HealProjectileClass;


	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;
	
	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector MuzzleOffset;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* FireMappingContext;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* FireAction;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* AltFireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchFireModeAction;

	/** Sets default values for this component's properties */
	UTP_WeaponComponent();

public:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Timer function to apply healing over time
	void ApplyHealing();

	void StopHealing();

	/** Attaches the actor to a FirstPersonCharacter */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	virtual void AttachWeapon(AIconoclasmCharacter* TargetCharacter);

	UFUNCTION(BlueprintCallable)
	virtual void Fire();
	UFUNCTION(BlueprintCallable)
	virtual void AltFire();
	UFUNCTION(BlueprintCallable)
	virtual void SwitchFireMode();

	UFUNCTION(BlueprintCallable)
	virtual void PerformHitscan(FVector& ImpactLocation);

	UFUNCTION(BlueprintCallable)
	void ApplyExplosionEffect(const FVector& ImpactLocation, float Radius, float Strength);

	UFUNCTION(BlueprintCallable)
	void HealingSphere(const FVector& ImpactLocation, float Radius);

	void ImpulseEffect(const FVector& ImpactLocation, float Radius, float Strength);

	void AntiGravity(const FVector& ImpactLocation, float Radius);

	virtual void DetachFromCharacter();

protected:
	/** Ends gameplay for this component. */
	UFUNCTION()
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// HUD class for the grenade launcher
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UGLHUD> GLHUDClass;

    // Instance of the HUD
    UPROPERTY()
    class UGLHUD* GLHUDInstance;

private:
	/** The Character holding this weapon*/
	AIconoclasmCharacter* Character;

	// Healing sphere parameters
	FVector HealingSphereLocation;
	float HealingSphereRadius;

	// Timer handle for healing
	FTimerHandle HealingTimerHandle;

	float LastAltLifeBloodModeTime = 0.0f;
	float LastAltImpulseModeTime = 0.0f;

	float AltLifeBloodCooldown = 5.0f; // Cooldown in seconds
	float AltImpulseCooldown = 7.0f;

	EWeaponMode CurrentWeaponMode;

	void LifeBloodMode();
	void AltlifeBloodMode();
	void ImpulseMode();
	void AltImpulseMode();

	void GetCooldownProgress(float LastFireTime, float CooldownDuration) const;

	void UpdateCooldowns();
};
