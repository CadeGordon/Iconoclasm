// Fill out your copyright notice in the Description page of Project Settings.


#include "ScoreComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

// Sets default values for this component's properties
UScoreComponent::UScoreComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
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
}

void UScoreComponent::AddScore(int32 Points)
{
	if (Points > 0)
	{
		CurrentScore += Points;

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
	// Check if we have a score value for this enemy type
	if (EnemyScoreValues.Contains(EnemyType))
	{
		int32 Points = EnemyScoreValues[EnemyType];
		AddScore(Points);

		UE_LOG(LogTemp, Log, TEXT("Enemy killed: %s, Points awarded: %d"), *EnemyType, Points);
	}
	else
	{
		// Default score if enemy type not found
		AddScore(100);
		UE_LOG(LogTemp, Warning, TEXT("Unknown enemy type: %s, awarded default 100 points"), *EnemyType);
	}
}

void UScoreComponent::ResetScore()
{
	CurrentScore = 0;
	OnScoreChanged.Broadcast(CurrentScore);
	UpdateScoreWidget();
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
	}
}

