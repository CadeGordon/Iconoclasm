// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BestResultsSave.h"
#include "BestResultsSubsystem.generated.h"

// Delegate for money changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoneyChanged, int32, NewMoney);

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

    // Money functions
    UFUNCTION(BlueprintCallable)
    void AddMoney(int32 Amount);

    UFUNCTION(BlueprintCallable)
    bool SpendMoney(int32 Amount);

    UFUNCTION(BlueprintCallable)
    int32 GetCurrentMoney() const;

    // Delegate that broadcasts when money changes
    UPROPERTY(BlueprintAssignable)
    FOnMoneyChanged OnMoneyChanged;

private:
    void LoadFromDisk();
    void SaveToDisk();

    UPROPERTY()
    UBestResultsSave* CurrentSaveGame = nullptr;

    FString SaveSlot = TEXT("BestResultsSlot");
    uint32 UserIndex = 0;
	
};
