// Fill out your copyright notice in the Description page of Project Settings.


#include "MoneyWidget.h"
#include "Components/TextBlock.h"
#include "BestResultsSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UMoneyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Get the subsystem
    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld());
    if (GameInstance)
    {
        BestResultsSubsystem = GameInstance->GetSubsystem<UBestResultsSubsystem>();

        if (BestResultsSubsystem)
        {
            // Bind to the money changed delegate
            BestResultsSubsystem->OnMoneyChanged.AddDynamic(this, &UMoneyWidget::UpdateMoneyDisplay);

            // Initialize with current money
            UpdateMoneyDisplay(BestResultsSubsystem->GetCurrentMoney());
        }
    }
}

void UMoneyWidget::NativeDestruct()
{
    // Unbind from delegate
    if (BestResultsSubsystem)
    {
        BestResultsSubsystem->OnMoneyChanged.RemoveDynamic(this, &UMoneyWidget::UpdateMoneyDisplay);
    }

    Super::NativeDestruct();
}

void UMoneyWidget::UpdateMoneyDisplay(int32 NewMoney)
{
    if (MoneyText)
    {
        // Format the money display (you can customize this)
        FString MoneyString = FString::Printf(TEXT("$%d"), NewMoney);
        MoneyText->SetText(FText::FromString(MoneyString));
    }
}