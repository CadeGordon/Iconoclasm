// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShotgunHUD.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API UShotgunHUD : public UUserWidget
{
	GENERATED_BODY()

public:
    // Updates the color of the HUD based on the shotgun's mode
    UFUNCTION(BlueprintCallable, Category = "ShotgunHUD")
    void UpdateMode(bool bIsRedMode);

    // Activates or deactivates the HUD
    UFUNCTION(BlueprintCallable, Category = "ShotgunHUD")
    void SetActive(bool bIsActive);

protected:
    // The image widget to change color
    UPROPERTY(meta = (BindWidget))
    class UImage* ModeIndicator;

private:
    void SetIndicatorColor(FLinearColor NewColor);
	
};
