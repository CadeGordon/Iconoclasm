// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RaphHealthBar.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API URaphHealthBar : public UUserWidget
{
	GENERATED_BODY()

protected:
	// Reference to the progress bar in the UI
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;


public:
	/** Updates the health bar progress */
	void UpdateHealthBar(float HealthPercentage);

	virtual void NativeConstruct() override;
	
};
