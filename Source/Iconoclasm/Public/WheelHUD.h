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

    UFUNCTION(BlueprintCallable, Category = "Weapon Wheel")
    void EquipRevolver();

    UPROPERTY(meta = (BindWidget))
    class UButton* WeaponButton1;

    UPROPERTY(meta = (BindWidget))
    class UButton* WeaponButton2;

    UPROPERTY(meta = (BindWidget))
    class UButton* WeaponButton3;

private:

    class AIconoclasmCharacter* PlayerCharacter;

    class UTP_WeaponComponent* RevolverComponent;
	
};
