// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BestResultsSave.generated.h"

USTRUCT(BlueprintType)
struct FLevelResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere)
    int32 Score = 0;

    UPROPERTY(VisibleAnywhere)
    float CompletionTime = 0.0f;

    UPROPERTY(VisibleAnywhere)
    FString Rank = TEXT("D");
};


/**
 * 
 */
UCLASS()
class ICONOCLASM_API UBestResultsSave : public USaveGame
{
	GENERATED_BODY()
	
public:
    UPROPERTY(VisibleAnywhere, Category = "Results")
    TMap<FName, FLevelResult> LevelResults; // key = LevelName

    

};
