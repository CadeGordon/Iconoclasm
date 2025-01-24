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

    // Update the progress bars
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void UpdateAltTimeWarpCooldown(float Progress);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void UpdateAltDefconCooldown(float Progress);

    void SetDefaultProgressBar();

    // Update the progress bar
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void UpdateCooldown(float Progress);

    // Show the progress bar for AltTimeWarpMode
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void ShowAltTimeWarpProgressBar();

    // Show the progress bar for AltDefconMode
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void ShowAltDefconProgressBar();

    // Bindable progress bar references (set in UMG Blueprint)
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* AltTimeWarpProgressBar;

    UPROPERTY(meta = (BindWidget))
    class UProgressBar* AltDefconProgressBar;

protected:
    // The image widget to change color
    UPROPERTY(meta = (BindWidget))
    class UImage* ModeIndicator;

    virtual void NativeConstruct() override;

private:
    void SetIndicatorColor(FLinearColor NewColor);


	
};
