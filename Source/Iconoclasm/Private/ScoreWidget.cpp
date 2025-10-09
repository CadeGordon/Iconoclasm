// Fill out your copyright notice in the Description page of Project Settings.


#include "ScoreWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/ScrollBox.h"


void UScoreWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Initialize default score thresholds (similar to ULTRAKILL style)
    if (ScoreTierThresholds.Num() == 0)
    {
        ScoreTierThresholds = {
            0,      // D tier starts at 0
            1000,   // C tier starts at 1000
            3000,   // B tier starts at 3000
            6000,   // A tier starts at 6000
            10000,   // S tier starts at 10000
            15000,
            20000,
            25000
        };
    }

    CurrentScore = 0;
    CurrentTier = EScoreTier::D;

    // Initialize the display
    UpdateScoreDisplay();

    // Hide the widget initially since score is 0
    SetVisibility(ESlateVisibility::Hidden);
}

void UScoreWidget::UpdateScore(int32 NewScore)
{
    CurrentScore = NewScore;
    CurrentTier = CalculateTier(CurrentScore);
    UpdateScoreDisplay();

    // Show widget if score > 0, hide if score == 0
    if (CurrentScore > 0)
    {
        if (GetVisibility() == ESlateVisibility::Hidden)
        {
            SetVisibility(ESlateVisibility::Visible);
        }
    }
    else
    {
        SetVisibility(ESlateVisibility::Hidden);
    }
}

FString UScoreWidget::GetTierName(EScoreTier Tier) const
{
    switch (Tier)
    {
    case EScoreTier::D: return TEXT("D");
    case EScoreTier::C: return TEXT("C");
    case EScoreTier::B: return TEXT("B");
    case EScoreTier::A: return TEXT("A");
    case EScoreTier::S: return TEXT("S");
    case EScoreTier::SS: return TEXT("SS");
    case EScoreTier::SSS: return TEXT("SSS");
    case EScoreTier::I: return TEXT("I");
    default: return TEXT("Unknown");
    }
}

float UScoreWidget::GetProgressToNextTier() const
{
    int32 CurrentTierIndex = static_cast<int32>(CurrentTier);

    // If we're at the highest tier, return 1.0 (full)
    if (CurrentTierIndex >= ScoreTierThresholds.Num() - 1)
    {
        return 1.0f;
    }

    int32 CurrentTierThreshold = ScoreTierThresholds[CurrentTierIndex];
    int32 NextTierThreshold = ScoreTierThresholds[CurrentTierIndex + 1];

    // Calculate progress between current and next tier
    if (NextTierThreshold == CurrentTierThreshold)
    {
        return 1.0f;
    }

    float Progress = static_cast<float>(CurrentScore - CurrentTierThreshold) /
        static_cast<float>(NextTierThreshold - CurrentTierThreshold);

    return FMath::Clamp(Progress, 0.0f, 1.0f);
}

EScoreTier UScoreWidget::CalculateTier(int32 Score) const
{
    EScoreTier ResultTier = EScoreTier::D;

    for (int32 i = ScoreTierThresholds.Num() - 1; i >= 0; --i)
    {
        if (Score >= ScoreTierThresholds[i])
        {
            ResultTier = static_cast<EScoreTier>(i);
            break;
        }
    }

    return ResultTier;
}

void UScoreWidget::UpdateScoreDisplay()
{
    if (ScoreTierText)
    {
        // Update the tier text display
        FString TierName = GetTierName(CurrentTier);
        ScoreTierText->SetText(FText::FromString(FString::Printf(TEXT("Rank: %s"), *TierName)));
    }

    if (ScoreProgressBar)
    {
        // Update the progress bar
        float Progress = GetProgressToNextTier();
        ScoreProgressBar->SetPercent(Progress);

        // Optional: Change progress bar color based on tier
        FLinearColor BarColor = FLinearColor::White;
        switch (CurrentTier)
        {
        case EScoreTier::D: BarColor = FLinearColor::Gray; break;
        case EScoreTier::C: BarColor = FLinearColor::Blue; break;
        case EScoreTier::B: BarColor = FLinearColor::Green; break;
        case EScoreTier::A: BarColor = FLinearColor::Yellow; break;
        case EScoreTier::S: BarColor = FLinearColor::Red; break;
        case EScoreTier::SS: BarColor = FLinearColor::Red; break;
        case EScoreTier::SSS: BarColor = FLinearColor::Red; break;
        case EScoreTier::I: BarColor = FLinearColor::Red; break;
        }
        ScoreProgressBar->SetFillColorAndOpacity(BarColor);
    }
}

void UScoreWidget::UpdateMultiplier(float NewMultiplier)
{
    if (MultiplierText)
    {
        FString MultText = FString::Printf(TEXT("Multiplier: x%.1f"), NewMultiplier);
        MultiplierText->SetText(FText::FromString(MultText));
    }
}

void UScoreWidget::AddKillMessage(const FString& Message, const FLinearColor& Color)
{
    if (!KillFeedBox) return;

    // Create a new TextBlock
    UTextBlock* KillText = NewObject<UTextBlock>(this, UTextBlock::StaticClass());
    if (KillText)
    {
        KillText->SetText(FText::FromString(Message));
        KillText->SetColorAndOpacity(FSlateColor(Color));

        // Add to scroll box
        KillFeedBox->AddChild(KillText);

        // Scroll to bottom to show newest message
        KillFeedBox->ScrollToEnd();

        // Remove after 2 seconds
        FTimerHandle RemoveHandle;
        FTimerDelegate RemoveDelegate = FTimerDelegate::CreateLambda([this, KillText]()
            {
                if (KillFeedBox && KillText)
                {
                    KillFeedBox->RemoveChild(KillText);
                }
            });
        GetWorld()->GetTimerManager().SetTimer(RemoveHandle, RemoveDelegate, 2.0f, false);
    }
}