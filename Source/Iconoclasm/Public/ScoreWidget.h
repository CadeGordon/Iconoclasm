// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "ScoreWidget.generated.h"

UENUM(BlueprintType)
enum class EScoreTier : uint8
{
    D = 0,
    C = 1,
    B = 2,
    A = 3,
    S = 4
};

/**
 * 
 */
UCLASS()
class ICONOCLASM_API UScoreWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

protected:
    

    // UI Components - bind these in the Blueprint
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* ScoreProgressBar;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* ScoreTierText;

    // Score thresholds for each tier
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score Settings")
    TArray<int32> ScoreTierThresholds;

    // Current score tracking
    UPROPERTY(BlueprintReadOnly, Category = "Score")
    int32 CurrentScore;

    UPROPERTY(BlueprintReadOnly, Category = "Score")
    EScoreTier CurrentTier;

public:
    // Function to update the score and UI
    UFUNCTION(BlueprintCallable, Category = "Score")
    void UpdateScore(int32 NewScore);

    // Function to get the tier name as string
    UFUNCTION(BlueprintPure, Category = "Score")
    FString GetTierName(EScoreTier Tier) const;

    // Function to get progress to next tier (0.0 to 1.0)
    UFUNCTION(BlueprintPure, Category = "Score")
    float GetProgressToNextTier() const;

private:
    // Internal function to determine tier based on score
    EScoreTier CalculateTier(int32 Score) const;

    // Update the visual elements
    void UpdateScoreDisplay();
	
};
