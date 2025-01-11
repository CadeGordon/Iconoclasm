// Fill out your copyright notice in the Description page of Project Settings.


#include "RevolverHUD.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"

void URevolverHUD::UpdateRevolverModeColor(uint8 Mode)
{
	if (RevolverImage)
	{
		FLinearColor NewColor = (Mode == 0) ? FLinearColor::Red : FLinearColor::Blue;
		RevolverImage->SetColorAndOpacity(NewColor);
	}
}

void URevolverHUD::NativeConstruct()
{
	Super::NativeConstruct();

	if (AltHellfireCooldownProgressBar)
	{
		AltHellfireCooldownProgressBar->SetVisibility(ESlateVisibility::Hidden);
	}

	if (AltFireCooldownBar)
	{
		AltFireCooldownBar->SetVisibility(ESlateVisibility::Visible);
	}
}

void URevolverHUD::SetVisibilityState(bool bIsVisible)
{
	SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}

void URevolverHUD::UpdateAltFireCooldownProgress(float Progress)
{
	if (AltFireCooldownBar)
	{
		AltFireCooldownBar->SetPercent(Progress);
	}
}

void URevolverHUD::UpdateAltHellfireCooldownProgress(float Progress)
{
	if (AltHellfireCooldownProgressBar)
	{
		AltHellfireCooldownProgressBar->SetPercent(Progress); // Update the AltHellfire cooldown progress
	}
}

void URevolverHUD::SetAltHellfireCooldownVisibility(bool bIsVisible)
{
	if (AltHellfireCooldownProgressBar)
	{
		AltHellfireCooldownProgressBar->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void URevolverHUD::SetAltGunslingerCooldownVisibility(bool bIsVisible)
{
	if (AltFireCooldownBar)
	{
		AltFireCooldownBar->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}


