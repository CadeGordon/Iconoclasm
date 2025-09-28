// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BestResultsSave.h"
#include "BestResultsSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API UBestResultsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void SaveLevelResult(FName LevelName, int32 Score, float CompletionTime, const FString& Rank);
    FLevelResult GetLevelResult(FName LevelName) const;

    int32 GetRankValue(const FString& Rank) const;

private:
    void LoadFromDisk();
    void SaveToDisk();

    UPROPERTY()
    UBestResultsSave* CurrentSaveGame = nullptr;

    FString SaveSlot = TEXT("BestResultsSlot");
    uint32 UserIndex = 0;
	
};
