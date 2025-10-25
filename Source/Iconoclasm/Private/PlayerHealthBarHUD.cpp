// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerHealthBarHUD.h"
#include "Components/ProgressBar.h"
#include "HealthComponent.h"
#include "Blueprint/UserWidget.h"
#include "IconoclasmCharacter.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetMathLibrary.h"

void UPlayerHealthBarHUD::NativeConstruct()
{
	Super::NativeConstruct();

	// Initialize values
	CurrentHealthPercent = 1.0f;
	TargetDamageBarPercent = 1.0f;
	CurrentDamageBarPercent = 1.0f;
	DisplayedHealthPercent = 1.0f;
	DamageBarTimer = 0.0f;
	bShouldLerpDamageBar = false;

	// Get the player character
	AIconoclasmCharacter* PlayerCharacter = Cast<AIconoclasmCharacter>(GetOwningPlayerPawn());
	if (PlayerCharacter)
	{
		HealthComponent = PlayerCharacter->FindComponentByClass<UHealthComponent>();

		if (HealthComponent)
		{
			HealthComponent->OnHealthChanged.AddDynamic(this, &UPlayerHealthBarHUD::UpdateHealthBar);
			UpdateHealthBar(HealthComponent->GetCurrentHealth());
		}
	}

	// Initialize bars
	if (HealthBar)
	{
		HealthBar->SetPercent(1.0f);
	}
	if (DamageBar)
	{
		DamageBar->SetPercent(1.0f);
	}
}

void UPlayerHealthBarHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!HealthComponent) return;

	// Smooth health bar transition
	if (!FMath::IsNearlyEqual(DisplayedHealthPercent, CurrentHealthPercent, 0.001f))
	{
		DisplayedHealthPercent = FMath::FInterpTo(
			DisplayedHealthPercent,
			CurrentHealthPercent,
			InDeltaTime,
			HealthBarSmoothSpeed
		);

		if (HealthBar)
		{
			HealthBar->SetPercent(DisplayedHealthPercent);
		}
	}

	// Handle damage bar delay and lerping
	if (CurrentDamageBarPercent > CurrentHealthPercent)
	{
		// Start or continue the delay timer
		if (!bShouldLerpDamageBar)
		{
			DamageBarTimer += InDeltaTime;

			if (DamageBarTimer >= DamageBarDelay)
			{
				bShouldLerpDamageBar = true;
				TargetDamageBarPercent = CurrentHealthPercent;
			}
		}

		// Lerp damage bar down smoothly
		if (bShouldLerpDamageBar)
		{
			CurrentDamageBarPercent = FMath::FInterpTo(
				CurrentDamageBarPercent,
				TargetDamageBarPercent,
				InDeltaTime,
				DamageBarLerpSpeed
			);

			if (DamageBar)
			{
				DamageBar->SetPercent(CurrentDamageBarPercent);
			}

			// Reset when close enough
			if (FMath::IsNearlyEqual(CurrentDamageBarPercent, TargetDamageBarPercent, 0.001f))
			{
				CurrentDamageBarPercent = TargetDamageBarPercent;
				bShouldLerpDamageBar = false;
				DamageBarTimer = 0.0f;
			}
		}
	}
	else if (CurrentDamageBarPercent < CurrentHealthPercent)
	{
		// If healing, instantly update damage bar
		CurrentDamageBarPercent = CurrentHealthPercent;
		if (DamageBar)
		{
			DamageBar->SetPercent(CurrentDamageBarPercent);
		}
	}
}

void UPlayerHealthBarHUD::UpdateHealthBar(float CurrentHealth)
{
	if (!HealthComponent) return;

	// Calculate new health percentage
	float NewHealthPercent = FMath::Clamp(CurrentHealth / HealthComponent->MaxHealth, 0.0f, 1.0f);

	// Only trigger damage bar if health decreased
	if (NewHealthPercent < CurrentHealthPercent)
	{
		// Reset damage bar tracking for new damage
		bShouldLerpDamageBar = false;
		DamageBarTimer = 0.0f;
	}
	else if (NewHealthPercent > CurrentHealthPercent)
	{
		// Healing - instantly update damage bar
		CurrentDamageBarPercent = NewHealthPercent;
		if (DamageBar)
		{
			DamageBar->SetPercent(CurrentDamageBarPercent);
		}
	}

	CurrentHealthPercent = NewHealthPercent;

	// Update health text if available
	if (HealthText)
	{
		FText HealthTextValue = FText::FromString(
			FString::Printf(TEXT("%.0f / %.0f"), CurrentHealth, HealthComponent->MaxHealth)
		);
		HealthText->SetText(HealthTextValue);
	}
	
}

void UPlayerHealthBarHUD::InitializeHealthBar(UHealthComponent* HealthComp)
{
	if (!HealthComp)
	{
		UE_LOG(LogTemp, Error, TEXT("InitializeHealthBar: HealthComponent is NULL!"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Health Widget Initialized"));
	HealthComponent = HealthComp;

	if (!HealthComponent->OnHealthChanged.IsBound())
	{
		HealthComponent->OnHealthChanged.AddDynamic(this, &UPlayerHealthBarHUD::UpdateHealthBar);
	}

	// Set initial health
	float InitialHealth = HealthComponent->GetCurrentHealth();
	CurrentHealthPercent = InitialHealth / HealthComponent->MaxHealth;
	DisplayedHealthPercent = CurrentHealthPercent;
	CurrentDamageBarPercent = CurrentHealthPercent;
	TargetDamageBarPercent = CurrentHealthPercent;

	if (HealthBar)
	{
		HealthBar->SetPercent(CurrentHealthPercent);
	}
	if (DamageBar)
	{
		DamageBar->SetPercent(CurrentHealthPercent);
	}

	UpdateHealthBar(InitialHealth);
}
