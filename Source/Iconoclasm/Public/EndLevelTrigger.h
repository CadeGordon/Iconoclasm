// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EndLevelTrigger.generated.h"

class UBoxComponent;
class UScoreComponent;
class UUserWidget;

UCLASS()
class ICONOCLASM_API AEndLevelTrigger : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEndLevelTrigger();



protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trigger")
    UBoxComponent* BoxCollider;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, 
                        AActor* OtherActor, 
                        UPrimitiveComponent* OtherComp, 
                        int32 OtherBodyIndex, 
                        bool bFromSweep, 
                        const FHitResult & SweepResult);

  

    // Widget to display end level score
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI")
    TSubclassOf<UUserWidget> EndLevelWidgetClass;

    UPROPERTY()
    UUserWidget* EndLevelWidget;

    // Store the final score without decay
    int32 FinalScore;

    // Store the time taken to reach the trigger
    float CompletionTime;



    // Function to calculate a rank based on time
    FString GetTimeRank(float TimeInSeconds) const;

    int32 FinalKills;
    FString GetKillRank(int32 Kills) const;

    FString CalculateFinalRank(float Score, float TimeInSeconds, int32 Kills) const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UBestResultsWidget> BestResultsWidgetClass;

    // ----- Time Rank Thresholds (in seconds) -----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Rank|Time")
    float STimeThreshold = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Rank|Time")
    float ATimeThreshold = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Rank|Time")
    float BTimeThreshold = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Rank|Time")
    float CTimeThreshold = 300.0f;

    // ----- Kill Rank Thresholds -----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Rank|Kills")
    int32 SKillThreshold = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Rank|Kills")
    int32 AKillThreshold = 35;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Rank|Kills")
    int32 BKillThreshold = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Rank|Kills")
    int32 CKillThreshold = 10;

    // ----- Final Rank Calculation -----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Rank|Final Rank")
    float MaxScore = 10000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Rank|Final Rank")
    float FastestTime = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Rank|Final Rank")
    int32 MaxKills = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Rank|Final Rank")
    float ScoreWeight = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Rank|Final Rank")
    float TimeWeight = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Rank|Final Rank")
    float KillWeight = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
    FName LevelID = "Level1";
};
