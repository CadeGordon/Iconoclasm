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

    if (RevolverButton)
    {
        RevolverButton->OnClicked.AddDynamic(this, &UWheelHUD::OnRevolverButtonClicked);
    }

    if (ShotgunButton)
    {
        ShotgunButton->OnClicked.AddDynamic(this, &UWheelHUD::OnShotgunButtonClicked);
    }

    if (GrenadeLauncherButton)
    {
        GrenadeLauncherButton->OnClicked.AddDynamic(this, &UWheelHUD::OnGrenadeLauncherButtonClicked);
    }

    // Assuming your player character has a reference to the weapon wheel widget
    PlayerCharacter = Cast<AIconoclasmCharacter>(GetOwningPlayerPawn());
}

void UWheelHUD::OnRevolverButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Revolver Button Pressed"));

    DisableWheel();
}

void UWheelHUD::OnShotgunButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Shotgun Button Pressed"));

    DisableWheel();
}

void UWheelHUD::OnGrenadeLauncherButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Grenade Launcher Button Pressed"));

    DisableWheel();
}

void UWheelHUD::DisableWheel()
{
    if (PlayerCharacter)
    {
        // Hide the weapon wheel
        PlayerCharacter->DisableWeaponWheel();
    }
}
