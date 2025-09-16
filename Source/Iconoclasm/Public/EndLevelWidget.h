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

protected:
    virtual void NativeConstruct() override;

    // Score text block
    UPROPERTY(meta = (BindWidget))
    UTextBlock* ScoreText;

    // Rank text block
    UPROPERTY(meta = (BindWidget))
    UTextBlock* RankText;

    // Score thresholds for D/C/B/A/S
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> ScoreTierThresholds;
	
};
