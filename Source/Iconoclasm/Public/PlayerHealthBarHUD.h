// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HealthComponent.h"
#include "PlayerHealthBarHUD.generated.h"

class UProgressBar;
class UHealthComponent;
class UTextBlock;

/**
 * 
 */
UCLASS()
class ICONOCLASM_API UPlayerHealthBarHUD : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar; // Progress bar for health

	UFUNCTION()
	void UpdateHealthBar(float CurrentHealth);

	UPROPERTY()
	UHealthComponent* HealthComponent;

	// Delayed damage bar (shows previous health, lerps down)
	UPROPERTY(meta = (BindWidget))
	UProgressBar* DamageBar;

	// Optional: Text to show numerical health
	UPROPERTY(meta = (BindWidget))
	UTextBlock* HealthText;

public:
	void InitializeHealthBar(UHealthComponent* HealthComp);

private:
	// Current normalized health (0-1)
	float CurrentHealthPercent;

	// Target for the damage bar to lerp towards
	float TargetDamageBarPercent;

	// Current damage bar position
	float CurrentDamageBarPercent;

	// Delay before damage bar starts following
	UPROPERTY(EditAnywhere, Category = "Health Bar Settings")
	float DamageBarDelay = 0.5f;

	// Speed at which damage bar follows health bar
	UPROPERTY(EditAnywhere, Category = "Health Bar Settings")
	float DamageBarLerpSpeed = 2.0f;

	// Timer for damage bar delay
	float DamageBarTimer;

	// Whether damage bar should be lerping
	bool bShouldLerpDamageBar;

	// Smoothing for health bar transitions
	UPROPERTY(EditAnywhere, Category = "Health Bar Settings")
	float HealthBarSmoothSpeed = 8.0f;

	float DisplayedHealthPercent;
	
};
