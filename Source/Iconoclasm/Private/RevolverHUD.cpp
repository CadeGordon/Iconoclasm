// Fill out your copyright notice in the Description page of Project Settings.


#include "RevolverHUD.h"
#include "Components/Image.h"

void URevolverHUD::UpdateRevolverModeColor(uint8 Mode)
{
	if (RevolverImage)
	{
		FLinearColor NewColor = (Mode == 0) ? FLinearColor::Red : FLinearColor::Blue;
		RevolverImage->SetColorAndOpacity(NewColor);
	}
}

void URevolverHUD::SetVisibilityState(bool bIsVisible)
{
	SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}
