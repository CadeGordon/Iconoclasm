// Fill out your copyright notice in the Description page of Project Settings.


#include "WheelHUD.h"
#include "Components/Button.h"
#include "IconoclasmCharacter.h"
#include "TP_WeaponComponent.h"
#include "Revolver_WeaponComponent.h"
#include "GameFramework/PlayerController.h"

void UWheelHUD::NativeConstruct()
{
    Super::NativeConstruct();

    PlayerCharacter = Cast<AIconoclasmCharacter>(GetOwningPlayerPawn());

    /*if (WeaponButton1)
    {
        WeaponButton1->OnClicked.AddDynamic(this, &UWheelHUD::EquipRevolver);
    }*/
}

void UWheelHUD::EquipRevolver()
{
    if (PlayerCharacter && RevolverComponent)
    {
        if (PlayerCharacter->WeaponInventory.Contains(RevolverComponent))
        {
            int32 WeaponIndex = PlayerCharacter->WeaponInventory.Find(RevolverComponent);
            if (WeaponIndex != INDEX_NONE)
            {
                // Update CurrentWeaponIndex to switch to the revolver
                PlayerCharacter->CurrentWeaponIndex = WeaponIndex;
                PlayerCharacter->EquipWeapon(RevolverComponent);
                UE_LOG(LogTemp, Warning, TEXT("Switched to revolver: %s"), *RevolverComponent->GetName());
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Revolver not in inventory, cannot equip."));
        }
    }
}
