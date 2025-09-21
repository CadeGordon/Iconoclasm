// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BestResultsSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API UBestResultsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    void UpdateResults(int32 NewScore, float NewTime, const FString& NewRank);

    int32 GetBestScore() const { return BestScore; }
    float GetBestTime() const { return BestTime; }
    FString GetBestRank() const { return BestRank; }

private:
    int32 BestScore = 0;
    float BestTime = FLT_MAX;
    FString BestRank = TEXT("D");
	
};
