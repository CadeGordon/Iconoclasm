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

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
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

	// Score decay settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score Decay", meta = (ClampMin = "0.1"))
	float DecayDelayTime = 3.0f; // Time in seconds before decay starts

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score Decay", meta = (ClampMin = "0.1"))
	float DecayTickRate = 0.016f; // How often to apply decay (in seconds)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score Decay", meta = (ClampMin = "1"))
	int32 DecayAmount = 10; // Points to remove per decay tick

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score Decay")
	int32 MinimumScore = 0; // Minimum score (decay won't go below this)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score Decay")
	bool bEnableDecay = true; // Master switch for decay system

	// Current score multiplier
	UPROPERTY(BlueprintReadOnly, Category = "Score")
	float CurrentMultiplier;

	// Max multiplier (3x)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	float MaxMultiplier = 3.0f;

	// Movement speed that equals max multiplier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	float MaxSpeedForMultiplier = 1200.0f; // adjust to your character’s max speed

	// Extra points for special actions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score Settings")
	int32 BaseKillPoints = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score Settings")
	int32 SlideKillBonus = 20;


	// Internal decay tracking
	FTimerHandle DecayDelayTimerHandle;
	FTimerHandle DecayTickTimerHandle;
	float TimeSinceLastScore;
	bool bIsDecaying;

	// Track total points earned without decay
	int32 TotalPointsEarned;

	int32 GetTotalPointsGained() const { return TotalPointsEarned; }

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

	// Score decay functions
	UFUNCTION(BlueprintCallable, Category = "Score")
	void StartScoreDecay();

	UFUNCTION(BlueprintCallable, Category = "Score")
	void StopScoreDecay();

	UFUNCTION(BlueprintCallable, Category = "Score")
	void SetDecaySettings(float InDecayDelay, float InDecayRate, int32 InDecayAmount);

	void ResetDecayTimer();

	void OnDecayDelayComplete();

	void ApplyScoreDecay();


private:
	// Initialize default enemy score values
	void InitializeDefaultScoreValues();

	// Update the UI widget
	void UpdateScoreWidget();

	// Update the multiplier based on player movement speed
	void UpdateScoreMultiplier();
};
