// Fill out your copyright notice in the Description page of Project Settings.


#include "ShotgunHUD.h"
#include "Components/Image.h"

void UShotgunHUD::UpdateMode(bool bIsRedMode)
{
    if (ModeIndicator)
    {
        FLinearColor NewColor = bIsRedMode ? FLinearColor::Red : FLinearColor::Blue;
        SetIndicatorColor(NewColor);
    }
}

void UShotgunHUD::SetActive(bool bIsActive)
{
    SetVisibility(bIsActive ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}

void UShotgunHUD::SetIndicatorColor(FLinearColor NewColor)
{
    if (ModeIndicator)
    {
        ModeIndicator->SetColorAndOpacity(NewColor);
    }
}
