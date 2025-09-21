// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BestResultsSave.generated.h"


USTRUCT(BlueprintType)
struct FLevelBestResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BestTime = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 BestScore = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString BestRank = TEXT("D");
};


/**
 * 
 */
UCLASS()
class ICONOCLASM_API UBestResultsSave : public USaveGame
{
	GENERATED_BODY()
	
public:
    // Map level names to their best results
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, FLevelBestResult> LevelResults;

};
