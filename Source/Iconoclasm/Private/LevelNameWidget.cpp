// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelNameWidget.h"
#include "Kismet/GameplayStatics.h"

void ULevelNameWidget::NativeConstruct()
{
    Super::NativeConstruct();

    CurrentCharIndex = 0;
    TimeSinceLastChar = 0.0f;
    bIsTyping = false;
    TypeSpeed = 0.05f;
}

void ULevelNameWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bIsTyping && TypewriterText)
    {
        TimeSinceLastChar += InDeltaTime;

        if (TimeSinceLastChar >= TypeSpeed)
        {
            TimeSinceLastChar = 0.0f;

            if (CurrentCharIndex < FullText.Len())
            {
                CurrentText.AppendChar(FullText[CurrentCharIndex]);
                CurrentCharIndex++;

                TypewriterText->SetText(FText::FromString(CurrentText));

                // Play sound if set
                if (TypeSound)
                {
                    UGameplayStatics::PlaySound2D(this, TypeSound);
                }
            }
            else
            {
                // Finished typing
                bIsTyping = false;
            }
        }
    }
}

void ULevelNameWidget::StartTypewriter(const FText& InText, float InSpeed)
{
    if (!TypewriterText)
    {
        UE_LOG(LogTemp, Error, TEXT("TypewriterText is not bound!"));
        return;
    }

    FullText = InText.ToString();
    CurrentText = "";
    CurrentCharIndex = 0;
    TimeSinceLastChar = 0.0f;
    TypeSpeed = FMath::Max(0.01f, InSpeed);
    bIsTyping = true;

    TypewriterText->SetText(FText::FromString(""));
}

void ULevelNameWidget::StopTypewriter()
{
    bIsTyping = false;

    if (TypewriterText)
    {
        TypewriterText->SetText(FText::FromString(FullText));
    }
}
