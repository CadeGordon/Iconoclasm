// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EndLevelWidget.generated.h"


class UTextBlock;

/**
 * 
 */
UCLASS()
class ICONOCLASM_API UEndLevelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    // Call this to update the score and optionally the rank
    UFUNCTION(BlueprintCallable)
    void SetEndLevelScore(int32 Score);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetEndLevelTime(float TimeInSeconds, const FString& TimeRank);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetEndLevelKills(int32 Kills, const FString& KillRank);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetFinalRank(const FString& Rank);

protected:
    virtual void NativeConstruct() override;

    // Score text block
    UPROPERTY(meta = (BindWidget))
    UTextBlock* ScoreText;

    // Rank text block
    UPROPERTY(meta = (BindWidget))
    UTextBlock* RankText;

    // Time
    UPROPERTY(meta = (BindWidget))
    UTextBlock* TimeText;

    // Time Rank
    UPROPERTY(meta = (BindWidget))
    UTextBlock* TimeRankText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* KillText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* KillRankText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* FinalRankText;


    // Score thresholds for D/C/B/A/S
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> ScoreTierThresholds;
	
};
