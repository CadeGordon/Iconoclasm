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

int32 UBestResultsSubsystem::GetRankValue(const FString& Rank) const
{
    if (Rank == "I") return 7;
    if (Rank == "SSS") return 6;
    if (Rank == "SS")  return 5;
    if (Rank == "S")   return 4;
    if (Rank == "A")   return 3;
    if (Rank == "B")   return 2;
    if (Rank == "C")   return 1;
    if (Rank == "D")   return 0;
    return -1; // unknown rank
}

void UBestResultsSubsystem::SaveLevelResult(FName LevelName, int32 Score, float CompletionTime, const FString& Rank)
{
    if (!CurrentSaveGame) return;

    FLevelResult& Existing = CurrentSaveGame->LevelResults.FindOrAdd(LevelName);
    int32 NewRankValue = GetRankValue(Rank);
    int32 OldRankValue = GetRankValue(Existing.Rank);

    bool bUpdated = false;

    // Keep the best score ever achieved
    if (Score > Existing.Score)
    {
        Existing.Score = Score;
        bUpdated = true;
    }

    // Keep the best rank ever achieved
    if (NewRankValue > OldRankValue)
    {
        Existing.Rank = Rank;
        bUpdated = true;
    }

    // Keep the fastest time ever achieved
    if (CompletionTime < Existing.CompletionTime || Existing.CompletionTime == 0.0f)
    {
        Existing.CompletionTime = CompletionTime;
        bUpdated = true;
    }

    if (bUpdated)
    {
        SaveToDisk();
    }
}

FLevelResult UBestResultsSubsystem::GetLevelResult(FName LevelName) const
{
    if (CurrentSaveGame && CurrentSaveGame->LevelResults.Contains(LevelName))
    {
        return CurrentSaveGame->LevelResults[LevelName];
    }
    return FLevelResult(); // default empty
}

void UBestResultsSubsystem::AddMoney(int32 Amount)
{
    if (!CurrentSaveGame || Amount <= 0) return;

    CurrentSaveGame->PlayerMoney += Amount;
    OnMoneyChanged.Broadcast(CurrentSaveGame->PlayerMoney);
    SaveToDisk();
}

bool UBestResultsSubsystem::SpendMoney(int32 Amount)
{
    if (!CurrentSaveGame || Amount <= 0) return false;

    if (CurrentSaveGame->PlayerMoney >= Amount)
    {
        CurrentSaveGame->PlayerMoney -= Amount;
        OnMoneyChanged.Broadcast(CurrentSaveGame->PlayerMoney);
        SaveToDisk();
        return true;
    }

    return false;
}

int32 UBestResultsSubsystem::GetCurrentMoney() const
{
    if (CurrentSaveGame)
    {
        return CurrentSaveGame->PlayerMoney;
    }
    return 0;
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

