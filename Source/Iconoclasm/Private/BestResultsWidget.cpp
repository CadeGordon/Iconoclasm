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
    UBestResultsSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UBestResultsSubsystem>();
    if (!Subsystem) return;

    FLevelResult Level1 = Subsystem->GetLevelResult("Demo_Level");
    FLevelResult Level2 = Subsystem->GetLevelResult("FirstPersonMap");

    auto FormatTime = [](float TimeSeconds) -> FString
        {
            int32 TotalMilliseconds = FMath::RoundToInt(TimeSeconds * 1000.0f);
            int32 Minutes = TotalMilliseconds / 60000;
            int32 Seconds = (TotalMilliseconds % 60000) / 1000;
            int32 Milliseconds = TotalMilliseconds % 1000;

            return FString::Printf(TEXT("%02d:%02d:%03d"), Minutes, Seconds, Milliseconds);
        };

    FString Level1Time = FormatTime(Level1.CompletionTime);
    FString Level2Time = FormatTime(Level2.CompletionTime);

    if (Level1ScoreText)
    {
        Level1ScoreText->SetText(FText::FromString(
            FString::Printf(TEXT("Score: %d  Time: %s  Rank: %s"),
                Level1.Score, *Level1Time, *Level1.Rank)));
    }

    if (Level2ScoreText)
    {
        Level2ScoreText->SetText(FText::FromString(
            FString::Printf(TEXT("Score: %d  Time: %s  Rank: %s"),
                Level2.Score, *Level2Time, *Level2.Rank)));
    }
}


