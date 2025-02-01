// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WheelHUD.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API UWheelHUD : public UUserWidget
{
	GENERATED_BODY()

public:

    virtual void NativeConstruct() override;

    UFUNCTION()
    void OnRevolverButtonClicked();

    UFUNCTION()
    void OnShotgunButtonClicked();

    UFUNCTION()
    void OnGrenadeLauncherButtonClicked();

    void DisableWheel();

    UPROPERTY(meta = (BindWidget))
    class UButton* RevolverButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* ShotgunButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* GrenadeLauncherButton;

private:

    class AIconoclasmCharacter* PlayerCharacter;

    class UTP_WeaponComponent* RevolverComponent;
	
};
