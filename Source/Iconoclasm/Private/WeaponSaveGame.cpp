// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponSaveGame.h"

UWeaponSaveGame::UWeaponSaveGame()
{
}

bool UWeaponSaveGame::HasWeapon(EWeaponType WeaponType) const
{
    for (const FWeaponSaveData& Data : UnlockedWeapons)
    {
        if (Data.WeaponType == WeaponType && Data.bHasWeapon)
        {
            return true;
        }
    }
    return false;
}

void UWeaponSaveGame::AddWeapon(EWeaponType WeaponType)
{
    // If already present, do nothing
    for (const FWeaponSaveData& Data : UnlockedWeapons)
    {
        if (Data.WeaponType == WeaponType)
        {
            return;
        }
    }

    FWeaponSaveData NewData;
    NewData.WeaponType = WeaponType;
    NewData.bHasWeapon = true;

    UnlockedWeapons.Add(NewData);
}

FWeaponSaveData* UWeaponSaveGame::GetWeaponData(EWeaponType WeaponType)
{
    for (FWeaponSaveData& Data : UnlockedWeapons)
    {
        if (Data.WeaponType == WeaponType)
        {
            return &Data;
        }
    }
    return nullptr;
}

const FWeaponSaveData* UWeaponSaveGame::GetWeaponData(EWeaponType WeaponType) const
{
    for (const FWeaponSaveData& Data : UnlockedWeapons)
    {
        if (Data.WeaponType == WeaponType)
        {
            return &Data;
        }
    }
    return nullptr;
}