// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GLHUD.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API UGLHUD : public UUserWidget
{
	GENERATED_BODY()
	
public:
    // Updates the displayed color for the grenade launcher mode
    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateImageColor(FLinearColor NewColor);

    // Update progress bar for AltLifeBloodMode
    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateAltLifeBloodCooldownProgress(float Progress);

    // Update progress bar for AltImpulseMode
    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateAltImpulseCooldownProgress(float Progress);

    void SetAltLifeBloodModeActive(bool bIsActive);

    void SetAltImpulseModeActive(bool bIsActive);



protected:
    // Reference to the image widget in the UI
    UPROPERTY(meta = (BindWidget))
    class UImage* ModeImage;


    // Reference to the image widget in the UI
    UPROPERTY(meta = (BindWidget))
    class UImage* Crosshair;

    // References to the progress bars
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* AltLifeBloodProgressBar;

    UPROPERTY(meta = (BindWidget))
    class UProgressBar* AltImpulseProgressBar;

    
};
