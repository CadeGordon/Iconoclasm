// Fill out your copyright notice in the Description page of Project Settings.


#include "RaphHealthBar.h"
#include "Components/ProgressBar.h"

void URaphHealthBar::NativeConstruct()
{
	Super::NativeConstruct();
}

void URaphHealthBar::UpdateHealthBar(float HealthPercentage)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(HealthPercentage);
	}
}
