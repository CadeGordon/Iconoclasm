// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BestResultsWidget.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API UBestResultsWidget : public UUserWidget
{
	GENERATED_BODY()

public:

    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable)
    void RefreshBestResults();

protected:
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* BestScoreText;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* BestTimeText;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* BestRankText;

private:
    int32 BestScore = 0;
    float BestTime = FLT_MAX;
    FString BestRank = TEXT("D");
	
};
