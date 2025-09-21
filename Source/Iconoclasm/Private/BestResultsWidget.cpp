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

    if (Level1ScoreText)
        Level1ScoreText->SetText(FText::FromString(FString::Printf(TEXT("Score: %d  Time: %.2f  Rank: %s"), Level1.Score, Level1.CompletionTime, *Level1.Rank)));

    if (Level2ScoreText)
        Level2ScoreText->SetText(FText::FromString(FString::Printf(TEXT("Score: %d  Time: %.2f  Rank: %s"), Level2.Score, Level2.CompletionTime, *Level2.Rank)));
}


