// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RevolverUnlockWidget.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API URevolverUnlockWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Constructor
	URevolverUnlockWidget(const FObjectInitializer& ObjectInitializer);

protected:
	// Called when the widget is constructed
	virtual void NativeConstruct() override;

	// Called when the widget is destructed
	virtual void NativeDestruct() override;

	// Button widget reference - bind this in the Widget Designer
	UPROPERTY(meta = (BindWidget))
	class UButton* UnlockButton;

	// Text widget reference - optional, for the button text
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* UnlockText;

	// Optional: Background overlay
	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* BackgroundImage;

private:
	// Button click handler
	UFUNCTION()
	void OnUnlockButtonClicked();

	// Reference to the weapon component
	UPROPERTY()
	class URevolver_WeaponComponent* RevolverComponent;

	// Find and cache the revolver component reference
	void CacheRevolverComponent();
	
};
