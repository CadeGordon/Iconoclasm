// Fill out your copyright notice in the Description page of Project Settings.


#include "BestResultsWidget.h"
#include "Components/TextBlock.h"
#include "BestResultsSubsystem.h"

void UBestResultsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    RefreshBestResults();
}

void UBestResultsWidget::RefreshBestResults()
{
    if (!GetGameInstance()) return;

    UBestResultsSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UBestResultsSubsystem>();
    if (!Subsystem) return;

    // Update best score
    if (BestScoreText)
    {
        BestScoreText->SetText(FText::AsNumber(Subsystem->GetBestScore()));
    }

    // Update best time in mm:ss.ms format
    if (BestTimeText)
    {
        float TimeValue = Subsystem->GetBestTime();
        if (TimeValue == FLT_MAX) TimeValue = 0.f;

        int32 Minutes = FMath::FloorToInt(TimeValue / 60.f);
        int32 Seconds = FMath::FloorToInt(FMath::Fmod(TimeValue, 60.f));
        int32 Milliseconds = FMath::RoundToInt((TimeValue - FMath::FloorToInt(TimeValue)) * 1000.f);

        BestTimeText->SetText(FText::FromString(
            FString::Printf(TEXT("%02d:%02d.%03d"), Minutes, Seconds, Milliseconds)));
    }

    // Update best rank
    if (BestRankText)
    {
        BestRankText->SetText(FText::FromString(Subsystem->GetBestRank()));
    }
}


