// Fill out your copyright notice in the Description page of Project Settings.


#include "ScoreComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "IconoclasmCharacter.h"
#include "GrappleComponent.h"
#include "WallRunComponent.h"

// Sets default values for this component's properties
UScoreComponent::UScoreComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// Initialize decay variables
	TimeSinceLastScore = 0.0f;
	bIsDecaying = false;
	CurrentScore = 0;
	CurrentMultiplier = 0.0f;
}


void UScoreComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize default score values
	InitializeDefaultScoreValues();

	// Create the score widget if we have a widget class set
	if (ScoreWidgetClass)
	{
		CreateScoreWidget();
	}

	// Start the decay system if enabled
	if (bEnableDecay)
	{
		ResetDecayTimer();
	}
}

void UScoreComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Track time since last score gain for decay system
	if (bEnableDecay && !bIsDecaying)
	{
		TimeSinceLastScore += DeltaTime;
	}

	if (ScoreWidgetInstance)
	{
		ScoreWidgetInstance->UpdateMultiplier(CurrentMultiplier);
	}

	UpdateScoreMultiplier();
}

void UScoreComponent::AddScore(int32 Points)
{
	if (Points > 0)
	{
		int32 AdjustedPoints = FMath::RoundToInt(Points * CurrentMultiplier);
		CurrentScore += AdjustedPoints;

		// Reset decay timer when score is gained
		if (bEnableDecay)
		{
			ResetDecayTimer();
		}

		// Broadcast the score change
		OnScoreChanged.Broadcast(CurrentScore);

		// Update the UI
		UpdateScoreWidget();

		// Optional: Log for debugging
		UE_LOG(LogTemp, Log, TEXT("Score added: %d, Total Score: %d"), Points, CurrentScore);
	}
}

void UScoreComponent::AddScoreForEnemy(const FString& EnemyType)
{
	int32 Points = 0;

	// Base points
	if (EnemyScoreValues.Contains(EnemyType))
	{
		Points = EnemyScoreValues[EnemyType];
	}
	else
	{
		Points = 100;
		UE_LOG(LogTemp, Warning, TEXT("Unknown enemy type: %s, awarded default %d points"), *EnemyType, Points);
	}

	AIconoclasmCharacter* OwnerCharacter = Cast<AIconoclasmCharacter>(GetOwner());
	if (OwnerCharacter)
	{
		UCharacterMovementComponent* MoveComp = OwnerCharacter->FindComponentByClass<UCharacterMovementComponent>();
		TArray<FString> BonusMessages;
		TArray<FLinearColor> BonusColors;

		// ---- Normal Kill ----
		BonusMessages.Add(TEXT("+Kill"));
		BonusColors.Add(FLinearColor::Green);

		// ---- Air Kill ----
		if (MoveComp && MoveComp->IsFalling())
		{
			Points += 150;
			BonusMessages.Add(TEXT("+AirKill"));
			BonusColors.Add(FLinearColor::Yellow);
			UE_LOG(LogTemp, Log, TEXT("Air kill bonus applied"));
		}

		// ---- Slide Kill ----
		if (OwnerCharacter->IsSliding)
		{
			Points += 200;
			BonusMessages.Add(TEXT("+SlideKill"));
			BonusColors.Add(FLinearColor::Blue);
			UE_LOG(LogTemp, Log, TEXT("Slide kill bonus applied"));
		}

		// ---- Grapple Kill ----
		if (UGrappleComponent* GrappleComp = OwnerCharacter->FindComponentByClass<UGrappleComponent>())
		{
			if (GrappleComp->IsGrappleActive)
			{
				Points += 250;
				BonusMessages.Add(TEXT("+GrappleKill"));
				BonusColors.Add(FLinearColor::White);
				UE_LOG(LogTemp, Log, TEXT("Grapple kill bonus applied"));
			}
		}

		// ---- Dash Kill ----
		if (OwnerCharacter->LastDashTime > 0.f)
		{
			float TimeSinceDash = GetWorld()->GetTimeSeconds() - OwnerCharacter->LastDashTime;
			if (TimeSinceDash <= 1.0f) // within 1 second after dashing
			{
				Points += 175;
				BonusMessages.Add(TEXT("+DashKill"));
				BonusColors.Add(FLinearColor::White);
				UE_LOG(LogTemp, Log, TEXT("Dash kill bonus applied"));
			}
		}

		// ---- Wall Run Kill ----
		if (OwnerCharacter->WallRunComponent && OwnerCharacter->WallRunComponent->GetIsWallRunning())
		{
			Points += 200;
			BonusMessages.Add(TEXT("+WallRunKill"));
			BonusColors.Add(FLinearColor::Yellow);
			UE_LOG(LogTemp, Log, TEXT("Wall run kill bonus applied"));
		}

		// ---- Melee Kill ----
		if (OwnerCharacter->bLastAttackWasMelee)
		{
			Points += 250; // melee bonus
			BonusMessages.Add(TEXT("+MeleeKill"));
			BonusColors.Add(FLinearColor::Red);
			UE_LOG(LogTemp, Log, TEXT("Melee kill bonus applied"));
		}

		// ---- ShatterShot Kill ----
		if (OwnerCharacter->bLastAttackWasShatterShot)
		{
			Points += 300; // bonus value, tweak as needed
			BonusMessages.Add(TEXT("+ShatterShotKill"));
			BonusColors.Add(FLinearColor::White);
			UE_LOG(LogTemp, Log, TEXT("Shatter Shot kill bonus applied"));
		}

		// ---- Charged Shot Kill ----
		if (OwnerCharacter->bLastAttackWasChargedShot)
		{
			Points += 350; // bonus points, adjust as needed
			BonusMessages.Add(TEXT("+ChargedShotKill"));
			BonusColors.Add(FLinearColor::Yellow);
			UE_LOG(LogTemp, Log, TEXT("Charged Shot kill bonus applied"));
		}

		// ---- Update UI with all messages ----
		if (ScoreWidgetInstance)
		{
			for (int32 i = 0; i < BonusMessages.Num(); i++)
			{
				ScoreWidgetInstance->AddKillMessage(BonusMessages[i], BonusColors[i]);
			}
		}
	}

	// ---- Apply score after all bonuses ----
	AddScore(Points);

	OwnerCharacter->bLastAttackWasMelee = false;

	OwnerCharacter->bLastAttackWasShatterShot = false;

	UE_LOG(LogTemp, Log, TEXT("Enemy killed: %s, Total Points awarded: %d"), *EnemyType, Points);
}

void UScoreComponent::ResetScore()
{
	CurrentScore = 0;
	OnScoreChanged.Broadcast(CurrentScore);
	UpdateScoreWidget();

	// Reset decay system
	if (bEnableDecay)
	{
		ResetDecayTimer();
	}
}

void UScoreComponent::StartScoreDecay()
{
	if (!bEnableDecay) return;

	UWorld* World = GetWorld();
	if (World && !bIsDecaying)
	{
		bIsDecaying = true;

		// Start the decay tick timer
		World->GetTimerManager().SetTimer(
			DecayTickTimerHandle,
			this,
			&UScoreComponent::ApplyScoreDecay,
			DecayTickRate,
			true // Loop
		);

		UE_LOG(LogTemp, Log, TEXT("Score decay started"));
	}
}

void UScoreComponent::StopScoreDecay()
{
	UWorld* World = GetWorld();
	if (World && bIsDecaying)
	{
		bIsDecaying = false;
		World->GetTimerManager().ClearTimer(DecayTickTimerHandle);
		UE_LOG(LogTemp, Log, TEXT("Score decay stopped"));
	}
}

void UScoreComponent::SetDecaySettings(float InDecayDelay, float InDecayRate, int32 InDecayAmount)
{
	DecayDelayTime = FMath::Max(0.1f, InDecayDelay);
	DecayTickRate = FMath::Max(0.1f, InDecayRate);
	DecayAmount = FMath::Max(1, InDecayAmount);

	// If we're currently in a decay state, restart with new settings
	if (bIsDecaying)
	{
		StopScoreDecay();
		StartScoreDecay();
	}
}

void UScoreComponent::CreateScoreWidget()
{
	if (ScoreWidgetClass && !ScoreWidgetInstance)
	{
		// Get the player controller to create the widget
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			ScoreWidgetInstance = CreateWidget<UScoreWidget>(PC, ScoreWidgetClass);
			if (ScoreWidgetInstance)
			{
				ScoreWidgetInstance->AddToViewport();
				UpdateScoreWidget(); // Initialize with current score
			}
		}
	}
}

void UScoreComponent::InitializeDefaultScoreValues()
{
	// Set default score values for different enemy types
	// You can modify these values or add them in Blueprint
	EnemyScoreValues.Empty();
	EnemyScoreValues.Add(TEXT("GruntEnemy"), 100);
	EnemyScoreValues.Add(TEXT("HeavyEnemy"), 250);
	EnemyScoreValues.Add(TEXT("BossEnemy"), 500);

	// Add more enemy types as needed
}

void UScoreComponent::UpdateScoreWidget()
{
	if (ScoreWidgetInstance)
	{
		ScoreWidgetInstance->UpdateScore(CurrentScore);
		ScoreWidgetInstance->UpdateMultiplier(CurrentMultiplier);
	}
}

void UScoreComponent::ResetDecayTimer()
{
    if (!bEnableDecay) return;
    
    UWorld* World = GetWorld();
    if (World)
    {
        // Stop any existing decay
        StopScoreDecay();
        
        // Reset the time counter
        TimeSinceLastScore = 0.0f;
        
        // Clear any existing delay timer
        World->GetTimerManager().ClearTimer(DecayDelayTimerHandle);
        
        // Start the delay timer
        World->GetTimerManager().SetTimer(
            DecayDelayTimerHandle,
            this,
            &UScoreComponent::OnDecayDelayComplete,
            DecayDelayTime,
            false // Don't loop
        );
    }
}

void UScoreComponent::OnDecayDelayComplete()
{
    // The delay period has passed without gaining score, start decay
    StartScoreDecay();
}

void UScoreComponent::ApplyScoreDecay()
{
    if (CurrentScore <= MinimumScore)
    {
        // We've reached the minimum score, stop decaying
        StopScoreDecay();
        return;
    }
    
    int32 OldScore = CurrentScore;
    CurrentScore = FMath::Max(MinimumScore, CurrentScore - DecayAmount);
    
    // Only broadcast and update if the score actually changed
    if (CurrentScore != OldScore)
    {
        OnScoreChanged.Broadcast(CurrentScore);
        UpdateScoreWidget();
        
        UE_LOG(LogTemp, Log, TEXT("Score decayed: -%d, New Score: %d"), 
               DecayAmount, CurrentScore);
    }
}

void UScoreComponent::UpdateScoreMultiplier()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	UCharacterMovementComponent* MoveComp = OwnerPawn->FindComponentByClass<UCharacterMovementComponent>();
	if (!MoveComp) return;

	float Speed = MoveComp->Velocity.Size();

	float TargetMultiplier = FMath::GetMappedRangeValueClamped(
		FVector2D(0.0f, MaxSpeedForMultiplier),
		FVector2D(0.0f, MaxMultiplier),
		Speed
	);

	// Smoothly interpolate instead of snapping
	CurrentMultiplier = FMath::FInterpTo(CurrentMultiplier, TargetMultiplier, GetWorld()->GetDeltaSeconds(), 5.0f);
}