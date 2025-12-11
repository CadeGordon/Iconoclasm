// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WeaponTypes.h"
#include "WeaponSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FWeaponSaveData
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EWeaponType WeaponType = EWeaponType::Revolver;

    // Does the player permanently own this weapon?
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bHasWeapon = false;

    // Mode unlocks per weapon
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bImpulseModeUnlocked = false;   // GL
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bDefconModeUnlocked = false;    // Shotgun
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bHellfireModeUnlocked = false;  // Revolver
};

UCLASS()
class ICONOCLASM_API UWeaponSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UWeaponSaveGame();

    // All weapon save data
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FWeaponSaveData> UnlockedWeapons;

    // Helpers
    bool HasWeapon(EWeaponType WeaponType) const;
    void AddWeapon(EWeaponType WeaponType);         // ensures entry exists
    FWeaponSaveData* GetWeaponData(EWeaponType WeaponType);
    const FWeaponSaveData* GetWeaponData(EWeaponType WeaponType) const;
};
