// Fill out your copyright notice in the Description page of Project Settings.


#include "EndLevelWidget.h"
#include "Components/TextBlock.h"

void UEndLevelWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (ScoreTierThresholds.Num() == 0)
    {
        ScoreTierThresholds = { 0, 1000, 3000, 6000, 10000 }; // D, C, B, A, S
    }
}

void UEndLevelWidget::SetEndLevelScore(int32 Score)
{
    if (ScoreText)
    {
        ScoreText->SetText(FText::FromString(FString::Printf(TEXT("Score: %d"), Score)));
    }

    if (RankText)
    {
        FString Rank = TEXT("D");
        if (Score >= ScoreTierThresholds[4]) Rank = TEXT("S");
        else if (Score >= ScoreTierThresholds[3]) Rank = TEXT("A");
        else if (Score >= ScoreTierThresholds[2]) Rank = TEXT("B");
        else if (Score >= ScoreTierThresholds[1]) Rank = TEXT("C");

        RankText->SetText(FText::FromString(FString::Printf(TEXT("Rank: %s"), *Rank)));
    }
}

