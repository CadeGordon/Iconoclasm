// Fill out your copyright notice in the Description page of Project Settings.


#include "EndLevelTrigger.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ScoreComponent.h"
#include "Blueprint/UserWidget.h"
#include "IconoclasmCharacter.h"
#include "EndLevelWidget.h"
#include "GameFramework/HUD.h"         
#include "BestResultsWidget.h"
#include "BestResultsSubsystem.h"

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
            FinalScore = ScoreComp->GetTotalPointsGained();

            // --- Completion Time ---
            CompletionTime = UGameplayStatics::GetTimeSeconds(GetWorld());

            FString TimeRank = GetTimeRank(CompletionTime);

            // --- Final Kills ---
            FinalKills = ScoreComp->GetKillCount();
            FString KillRank = GetKillRank(FinalKills);

            FString FinalRank = CalculateFinalRank(FinalScore, CompletionTime, FinalKills);

            UBestResultsSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UBestResultsSubsystem>();
            if (Subsystem)
            {
                Subsystem->SaveLevelResult(LevelID, FinalScore, CompletionTime, FinalRank);
            }

            // Show the end level widget
            if (EndLevelWidgetClass)
            {
                UEndLevelWidget* WidgetInstance = CreateWidget<UEndLevelWidget>(GetWorld(), EndLevelWidgetClass);
                if (WidgetInstance)
                {
                    WidgetInstance->AddToViewport();
                    WidgetInstance->SetEndLevelScore(FinalScore, MaxScore);

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
    if (TimeInSeconds <= STimeThreshold) return TEXT("S");
    else if (TimeInSeconds <= ATimeThreshold) return TEXT("A");
    else if (TimeInSeconds <= BTimeThreshold) return TEXT("B");
    else if (TimeInSeconds <= CTimeThreshold) return TEXT("C");
    else return TEXT("D");
}

FString AEndLevelTrigger::GetKillRank(int32 Kills) const
{
    if (Kills >= SKillThreshold) return TEXT("S");
    else if (Kills >= AKillThreshold) return TEXT("A");
    else if (Kills >= BKillThreshold) return TEXT("B");
    else if (Kills >= CKillThreshold) return TEXT("C");
    else return TEXT("D");
}

FString AEndLevelTrigger::CalculateFinalRank(float Score, float TimeInSeconds, int32 Kills) const
{
    float NormalizedScore = FMath::Clamp(Score / MaxScore, 0.0f, 1.0f);
    float NormalizedTime = FMath::Clamp(FastestTime / TimeInSeconds, 0.0f, 1.0f);
    float NormalizedKills = FMath::Clamp(static_cast<float>(Kills) / MaxKills, 0.0f, 1.0f);

    float CombinedScore = (NormalizedScore * ScoreWeight + NormalizedTime * TimeWeight + NormalizedKills * KillWeight) * 100.0f;

    if (CombinedScore >= 95.0f) return TEXT("SSS");
    else if (CombinedScore >= 85.0f) return TEXT("SS");
    else if (CombinedScore >= 70.0f) return TEXT("S");
    else if (CombinedScore >= 55.0f) return TEXT("A");
    else if (CombinedScore >= 40.0f) return TEXT("B");
    else if (CombinedScore >= 25.0f) return TEXT("C");
    else return TEXT("D");
}
