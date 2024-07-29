// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "RevWeaponComponent.generated.h"

class AIconoclasmCharacter;

UENUM(BlueprintType)
enum class ERevMode : uint8
{
	RevMode1 UMETA(DisplayName = "Gunslinger"),
	RevMode2 UMETA(DisplayName = "Hellfire"),
};

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ICONOCLASM_API URevWeaponComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()
	
public:
	
	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	FVector MuzzleOffset;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* FireMappingContext;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* FireAction;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* AltFireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchFireModeAction;

	/** Sets default values for this component's properties */
	URevWeaponComponent();

protected:
	virtual void BeginPlay() override;

	/** Attaches the actor to a FirstPersonCharacter */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AttachWeapon(AIconoclasmCharacter* TargetCharacter);

	/** Make the weapon Fire a Projectile */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Fire();

	void AltFire();

	void SwitchFireMode();

	UFUNCTION(BlueprintCallable)
	void PerformHitscan(FVector& ImpactLocation);

	

protected:
	/** Ends gameplay for this component. */
	UFUNCTION()
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The Character holding this weapon*/
	AIconoclasmCharacter* Character;

	ERevMode CurrentWeaponMode;

	FTimerHandle TimerHandle_AltGunslingerFire;
	int32 HitscanCount;

	FTimerHandle TimerHandle_AltHellfire;
	float HellfireDuration;

	void GunslingerMode();
	void AltGunslingerMode();
	void HellfireMode();
	void AltHellfireMode();
};
