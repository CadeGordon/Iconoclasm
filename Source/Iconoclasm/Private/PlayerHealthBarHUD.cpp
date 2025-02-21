// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerHealthBarHUD.h"
#include "Components/ProgressBar.h"
#include "HealthComponent.h"
#include "Blueprint/UserWidget.h"
#include "IconoclasmCharacter.h"

void UPlayerHealthBarHUD::NativeConstruct()
{
	Super::NativeConstruct();

	// Get the player character
	AIconoclasmCharacter* PlayerCharacter = Cast<AIconoclasmCharacter>(GetOwningPlayerPawn());
	if (PlayerCharacter)
	{
		HealthComponent = PlayerCharacter->FindComponentByClass<UHealthComponent>();

		// Ensure HealthComponent is valid before using it
		if (HealthComponent)
		{
			HealthComponent->OnHealthChanged.AddDynamic(this, &UPlayerHealthBarHUD::UpdateHealthBar);
			UpdateHealthBar(HealthComponent->GetCurrentHealth()); // Initial update
		}
	}
}

void UPlayerHealthBarHUD::UpdateHealthBar(float CurrentHealth)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(CurrentHealth / HealthComponent->MaxHealth);  // Normalize health (0 to 1)
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

	if (!HealthComponent->OnHealthChanged.IsBound())  // Check if already bound
	{
		HealthComponent->OnHealthChanged.AddDynamic(this, &UPlayerHealthBarHUD::UpdateHealthBar);
	}

	// Set initial health
	UpdateHealthBar(HealthComponent->GetCurrentHealth());
}
