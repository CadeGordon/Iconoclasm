// Fill out your copyright notice in the Description page of Project Settings.


#include "ShotgunHUD.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"

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

void UShotgunHUD::UpdateAltTimeWarpCooldown(float Progress)
{
    if (AltTimeWarpProgressBar)
    {
        AltTimeWarpProgressBar->SetPercent(Progress);
    }
}

void UShotgunHUD::UpdateAltDefconCooldown(float Progress)
{
    if (AltDefconProgressBar)
    {
        AltDefconProgressBar->SetPercent(Progress);
    }
}

void UShotgunHUD::SetDefaultProgressBar()
{
    if (AltTimeWarpProgressBar && AltDefconProgressBar)
    {
        AltTimeWarpProgressBar->SetVisibility(ESlateVisibility::Visible);
        AltDefconProgressBar->SetVisibility(ESlateVisibility::Hidden);
    }
}

void UShotgunHUD::UpdateCooldown(float Progress)
{
    if (AltTimeWarpProgressBar->IsVisible())
    {
        AltTimeWarpProgressBar->SetPercent(Progress);
    }
    else if (AltDefconProgressBar->IsVisible())
    {
        AltDefconProgressBar->SetPercent(Progress);
    }
}

void UShotgunHUD::ShowAltTimeWarpProgressBar()
{
    if (AltTimeWarpProgressBar && AltDefconProgressBar)
    {
        AltTimeWarpProgressBar->SetVisibility(ESlateVisibility::Visible);
        AltDefconProgressBar->SetVisibility(ESlateVisibility::Hidden);
    }
}

void UShotgunHUD::ShowAltDefconProgressBar()
{
    if (AltDefconProgressBar && AltTimeWarpProgressBar)
    {
        AltDefconProgressBar->SetVisibility(ESlateVisibility::Visible);
        AltTimeWarpProgressBar->SetVisibility(ESlateVisibility::Hidden);
    }
}

void UShotgunHUD::NativeConstruct()
{
    Super::NativeConstruct();

    // Set the initial visibility of progress bars when the HUD is created
    if (AltTimeWarpProgressBar && AltDefconProgressBar)
    {
        // Make AltTimeWarpProgressBar visible by default and AltDefconProgressBar hidden
        AltTimeWarpProgressBar->SetVisibility(ESlateVisibility::Visible);
        AltDefconProgressBar->SetVisibility(ESlateVisibility::Hidden);
    }
}

void UShotgunHUD::SetIndicatorColor(FLinearColor NewColor)
{
    if (ModeIndicator)
    {
        ModeIndicator->SetColorAndOpacity(NewColor);
    }
}
