// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HealthComponent.h"
#include "PlayerHealthBarHUD.generated.h"

class UProgressBar;
class UHealthComponent;

/**
 * 
 */
UCLASS()
class ICONOCLASM_API UPlayerHealthBarHUD : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar; // Progress bar for health

	UFUNCTION()
	void UpdateHealthBar(float CurrentHealth);

	UPROPERTY()
	UHealthComponent* HealthComponent;

public:
	void InitializeHealthBar(UHealthComponent* HealthComp);
	
};
