// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MoneyWidget.generated.h"

class UTextBlock;
class UBestResultsSubsystem;

/**
 * 
 */
UCLASS()
class ICONOCLASM_API UMoneyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

protected:
    // Bind this to a TextBlock in your widget blueprint
    UPROPERTY(meta = (BindWidget))
    UTextBlock* MoneyText;

    UFUNCTION()
    void UpdateMoneyDisplay(int32 NewMoney);

private:
    UPROPERTY()
    UBestResultsSubsystem* BestResultsSubsystem;

	
};
