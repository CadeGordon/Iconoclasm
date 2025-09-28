// Fill out your copyright notice in the Description page of Project Settings.


#include "EndLevelWidget.h"
#include "Components/TextBlock.h"
#include "EndLevelTrigger.h"

void UEndLevelWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (ScoreTierThresholds.Num() == 0)
    {
        ScoreTierThresholds = { 0, 1000, 3000, 6000, 10000 }; // D, C, B, A, S
    }
}

void UEndLevelWidget::SetEndLevelScore(int32 Score, int32 MaxScore)
{
    if (ScoreText)
    {
        ScoreText->SetText(FText::FromString(FString::Printf(TEXT("Score: %d"), Score)));
    }

    if (RankText)
    {
        // Dynamically calculate thresholds based on MaxScore
        ScoreTierThresholds = {
            0,                          // D
            FMath::RoundToInt(MaxScore * 0.25f), // C
            FMath::RoundToInt(MaxScore * 0.40f), // B
            FMath::RoundToInt(MaxScore * 0.55f), // A
            MaxScore                    // S
        };

        FString Rank = TEXT("D");
        if (Score >= ScoreTierThresholds[4]) Rank = TEXT("S");
        else if (Score >= ScoreTierThresholds[3]) Rank = TEXT("A");
        else if (Score >= ScoreTierThresholds[2]) Rank = TEXT("B");
        else if (Score >= ScoreTierThresholds[1]) Rank = TEXT("C");

        RankText->SetText(FText::FromString(FString::Printf(TEXT("Rank: %s"), *Rank)));
    }
}

void UEndLevelWidget::SetEndLevelTime(float TimeInSeconds, const FString& TimeRank)
{
    int32 TotalMilliseconds = FMath::RoundToInt(TimeInSeconds * 1000.0f);

    int32 Minutes = TotalMilliseconds / 60000;
    int32 Seconds = (TotalMilliseconds % 60000) / 1000;
    int32 Milliseconds = TotalMilliseconds % 1000;

    FString TimeString = FString::Printf(TEXT("Time: %02d:%02d:%03d"), Minutes, Seconds, Milliseconds);

    if (TimeText)
    {
        TimeText->SetText(FText::FromString(TimeString));
    }

    if (TimeRankText)
    {
        FString RankString = FString::Printf(TEXT("Time Rank: %s"), *TimeRank);
        TimeRankText->SetText(FText::FromString(RankString));
    }
}

void UEndLevelWidget::SetEndLevelKills(int32 Kills, const FString& KillRank)
{
    if (KillText)
    {
        FString KillString = FString::Printf(TEXT("Kills: %d"), Kills);
        KillText->SetText(FText::FromString(KillString));
    }

    if (KillRankText)
    {
        FString RankString = FString::Printf(TEXT("Kill Rank: %s"), *KillRank);
        KillRankText->SetText(FText::FromString(RankString));
    }
}

void UEndLevelWidget::SetFinalRank(const FString& Rank)
{
    if (FinalRankText)
    {
        FString RankString = FString::Printf(TEXT("Rank: %s"), *Rank);
        FinalRankText->SetText(FText::FromString(RankString));
    }
}



