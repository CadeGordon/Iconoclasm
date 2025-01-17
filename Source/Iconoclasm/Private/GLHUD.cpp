// Fill out your copyright notice in the Description page of Project Settings.


#include "GLHUD.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"

void UGLHUD::UpdateImageColor(FLinearColor NewColor)
{
    if (ModeImage)
    {
        ModeImage->SetColorAndOpacity(NewColor);
    }
}

void UGLHUD::UpdateAltLifeBloodCooldownProgress(float Progress)
{
    if (AltLifeBloodProgressBar)
    {
        if (Progress > 0.0f)
        {
            AltLifeBloodProgressBar->SetVisibility(ESlateVisibility::Visible);
            AltLifeBloodProgressBar->SetPercent(Progress);
        }
        else
        {
            AltLifeBloodProgressBar->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

void UGLHUD::UpdateAltImpulseCooldownProgress(float Progress)
{
    if (AltImpulseProgressBar)
    {
        if (Progress > 0.0f)
        {
            AltImpulseProgressBar->SetVisibility(ESlateVisibility::Visible);
            AltImpulseProgressBar->SetPercent(Progress);
        }
        else
        {
            AltImpulseProgressBar->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

void UGLHUD::SetAltLifeBloodModeActive(bool bIsActive)
{
    if (AltLifeBloodProgressBar)
    {
        AltLifeBloodProgressBar->SetVisibility(bIsActive ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
}

void UGLHUD::SetAltImpulseModeActive(bool bIsActive)
{
    if (AltImpulseProgressBar)
    {
        AltImpulseProgressBar->SetVisibility(bIsActive ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
}



