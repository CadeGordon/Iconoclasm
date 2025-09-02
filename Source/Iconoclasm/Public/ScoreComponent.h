// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ScoreWidget.h"
#include "ScoreComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreChanged, int32, NewScore);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ICONOCLASM_API UScoreComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UScoreComponent();

protected:
	virtual void BeginPlay() override;

	// Current score
	UPROPERTY(BlueprintReadOnly, Category = "Score")
	int32 CurrentScore;

	// Reference to the score widget
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UScoreWidget* ScoreWidgetInstance;

	// Widget class to create
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UScoreWidget> ScoreWidgetClass;

	// Score values for different enemy types
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score Settings")
	TMap<FString, int32> EnemyScoreValues;

public:
	// Event dispatcher for score changes
	UPROPERTY(BlueprintAssignable, Category = "Score")
	FOnScoreChanged OnScoreChanged;

	// Add score points
	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddScore(int32 Points);

	// Add score for specific enemy type
	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddScoreForEnemy(const FString& EnemyType);

	// Get current score
	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetCurrentScore() const { return CurrentScore; }

	// Reset score
	UFUNCTION(BlueprintCallable, Category = "Score")
	void ResetScore();

	// Create and show the score widget
	UFUNCTION(BlueprintCallable, Category = "UI")
	void CreateScoreWidget();

	// Get the score widget instance
	UFUNCTION(BlueprintPure, Category = "UI")
	UScoreWidget* GetScoreWidget() const { return ScoreWidgetInstance; }

private:
	// Initialize default enemy score values
	void InitializeDefaultScoreValues();

	// Update the UI widget
	void UpdateScoreWidget();
};
