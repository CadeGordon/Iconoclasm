// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RevolverHUD.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API URevolverHUD : public UUserWidget
{
	GENERATED_BODY()
	
public:
	/** Update the UI color based on revolver mode */
	UFUNCTION(BlueprintCallable, Category = "Revolver HUD")
	void UpdateRevolverModeColor(uint8 Mode);

	void NativeConstruct();

	/** Show or hide the widget */
	UFUNCTION(BlueprintCallable, Category = "Revolver HUD")
	void SetVisibilityState(bool bIsVisible);

	// Bindable progress bar for AltFire cooldown
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* AltFireCooldownBar;

	// Updates the progress bar value (0 to 1)
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateAltFireCooldownProgress(float Progress);

	// Function to update the AltHellfire cooldown progress bar
	UFUNCTION(BlueprintCallable, Category = "RevolverHUD")
	void UpdateAltHellfireCooldownProgress(float Progress);

	void SetAltHellfireCooldownVisibility(bool bIsVisible);

	void SetAltGunslingerCooldownVisibility(bool bIsVisible);

	// Bindable progress bar for AltFire cooldown
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* AltHellfireCooldownProgressBar;

protected:
	UPROPERTY(meta = (BindWidget))
	class UImage* RevolverImage;

	UPROPERTY(meta = (BindWidget))
	class UImage* CrosshairImage;

};
