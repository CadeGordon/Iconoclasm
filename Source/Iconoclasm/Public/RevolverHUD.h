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

	/** Show or hide the widget */
	UFUNCTION(BlueprintCallable, Category = "Revolver HUD")
	void SetVisibilityState(bool bIsVisible);

protected:
	UPROPERTY(meta = (BindWidget))
	class UImage* RevolverImage;

	UPROPERTY(meta = (BindWidget))
	class UImage* CrosshairImage;

};
