// Fill out your copyright notice in the Description page of Project Settings.


#include "EndLevelTrigger.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ScoreComponent.h"
#include "Blueprint/UserWidget.h"
#include "IconoclasmCharacter.h"
#include "EndLevelWidget.h"

// Sets default values
AEndLevelTrigger::AEndLevelTrigger()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    BoxCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollider"));
    RootComponent = BoxCollider;
    BoxCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BoxCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
    BoxCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    BoxCollider->OnComponentBeginOverlap.AddDynamic(this, &AEndLevelTrigger::OnOverlapBegin);

    FinalScore = 0;
    CompletionTime = 0.0f;
}

// Called when the game starts or when spawned
void AEndLevelTrigger::BeginPlay()
{
	Super::BeginPlay();
	
}

void AEndLevelTrigger::OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!OtherActor) return;

    AIconoclasmCharacter* Player = Cast<AIconoclasmCharacter>(OtherActor);
    if (Player)
    {
        // Get the player's ScoreComponent
        UScoreComponent* ScoreComp = Player->FindComponentByClass<UScoreComponent>();
        if (ScoreComp)
        {
            // Capture the total points gained ignoring decay
            FinalScore = ScoreComp->GetTotalPointsGained(); // We'll add this function

            // --- Completion Time ---
            CompletionTime = UGameplayStatics::GetTimeSeconds(GetWorld());

            FString TimeRank = GetTimeRank(CompletionTime);

            // --- Final Kills ---
            FinalKills = ScoreComp->GetKillCount();
            FString KillRank = GetKillRank(FinalKills);

            FString FinalRank = CalculateFinalRank(FinalScore, CompletionTime, FinalKills);

            // Show the end level widget
            if (EndLevelWidgetClass)
            {
                UEndLevelWidget* WidgetInstance = CreateWidget<UEndLevelWidget>(GetWorld(), EndLevelWidgetClass);
                if (WidgetInstance)
                {
                    WidgetInstance->AddToViewport();
                    WidgetInstance->SetEndLevelScore(FinalScore);

                    // Send time + rank
                    WidgetInstance->SetEndLevelTime(CompletionTime, TimeRank);

                    // Send kills
                    WidgetInstance->SetEndLevelKills(FinalKills, KillRank);

                    WidgetInstance->SetFinalRank(FinalRank);
                }
            }

            // Optionally: Stop decay timer or freeze gameplay here
        }
    }
}

FString AEndLevelTrigger::GetTimeRank(float TimeInSeconds) const
{
    if (TimeInSeconds <= 60.0f) // under 1 min
        return TEXT("S");
    else if (TimeInSeconds <= 120.0f) // under 2 min
        return TEXT("A");
    else if (TimeInSeconds <= 180.0f) // under 3 min
        return TEXT("B");
    else if (TimeInSeconds <= 300.0f) // under 5 min
        return TEXT("C");
    else
        return TEXT("D");
}

FString AEndLevelTrigger::GetKillRank(int32 Kills) const
{
    if (Kills >= 50)
        return TEXT("S");
    else if (Kills >= 35)
        return TEXT("A");
    else if (Kills >= 20)
        return TEXT("B");
    else if (Kills >= 10)
        return TEXT("C");
    else
        return TEXT("D");
}

FString AEndLevelTrigger::CalculateFinalRank(float Score, float TimeInSeconds, int32 Kills) const
{
    // ---- Config ----
    const float MaxScore = 10000.0f;      // Adjust to your level's max score
    const float FastestTime = 60.0f;      // Best possible time in seconds
    const int32 MaxKills = 50;            // Total enemies in level

    const float ScoreWeight = 0.4f;
    const float TimeWeight = 0.3f;
    const float KillWeight = 0.3f;

    // ---- Normalize ----
    float NormalizedScore = FMath::Clamp(Score / MaxScore, 0.0f, 1.0f);

    // For time, lower is better
    float NormalizedTime = FMath::Clamp(FastestTime / TimeInSeconds, 0.0f, 1.0f);

    float NormalizedKills = FMath::Clamp(static_cast<float>(Kills) / MaxKills, 0.0f, 1.0f);

    // ---- Weighted sum ----
    float CombinedScore = (NormalizedScore * ScoreWeight + NormalizedTime * TimeWeight + NormalizedKills * KillWeight) * 100.0f;

    // ---- Determine Rank ----
    if (CombinedScore >= 95.0f) return TEXT("SSS");
    else if (CombinedScore >= 85.0f) return TEXT("SS");
    else if (CombinedScore >= 70.0f) return TEXT("S");
    else if (CombinedScore >= 55.0f) return TEXT("A");
    else if (CombinedScore >= 40.0f) return TEXT("B");
    else if (CombinedScore >= 25.0f) return TEXT("C");
    else return TEXT("D");
}
