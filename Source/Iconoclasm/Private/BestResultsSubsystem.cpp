// Fill out your copyright notice in the Description page of Project Settings.


#include "BestResultsSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "BestResultsSave.h"

void UBestResultsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadFromDisk();
}

void UBestResultsSubsystem::Deinitialize()
{
    SaveToDisk();
    Super::Deinitialize();
}

void UBestResultsSubsystem::SaveLevelResult(FName LevelName, int32 Score, float CompletionTime, const FString& Rank)
{
    if (!CurrentSaveGame) return;

    FLevelResult& Existing = CurrentSaveGame->LevelResults.FindOrAdd(LevelName);

    // Only update if better
    if (Score > Existing.Score) Existing.Score = Score;
    if (CompletionTime < Existing.CompletionTime || Existing.CompletionTime == 0.0f) Existing.CompletionTime = CompletionTime;
    if (Rank < Existing.Rank) Existing.Rank = Rank; // customize comparison if needed

    SaveToDisk();
}

FLevelResult UBestResultsSubsystem::GetLevelResult(FName LevelName) const
{
    if (CurrentSaveGame && CurrentSaveGame->LevelResults.Contains(LevelName))
    {
        return CurrentSaveGame->LevelResults[LevelName];
    }
    return FLevelResult(); // default empty
}

void UBestResultsSubsystem::LoadFromDisk()
{
    if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SaveSlot, UserIndex))
    {
        CurrentSaveGame = Cast<UBestResultsSave>(Loaded);
    }

    if (!CurrentSaveGame)
    {
        CurrentSaveGame = Cast<UBestResultsSave>(
            UGameplayStatics::CreateSaveGameObject(UBestResultsSave::StaticClass()));
    }
}

void UBestResultsSubsystem::SaveToDisk()
{
    if (CurrentSaveGame)
    {
        UGameplayStatics::SaveGameToSlot(CurrentSaveGame, SaveSlot, UserIndex);
    }
}

