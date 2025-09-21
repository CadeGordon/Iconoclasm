// Fill out your copyright notice in the Description page of Project Settings.


#include "BestResultsSubsystem.h"

void UBestResultsSubsystem::UpdateResults(int32 NewScore, float NewTime, const FString& NewRank)
{
    if (NewScore > BestScore)
    {
        BestScore = NewScore;
        BestRank = NewRank;
    }

    if (NewTime < BestTime)
    {
        BestTime = NewTime;
    }
}