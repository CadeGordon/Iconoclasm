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

            // Show the end level widget
            if (EndLevelWidgetClass)
            {
                UEndLevelWidget* WidgetInstance = CreateWidget<UEndLevelWidget>(GetWorld(), EndLevelWidgetClass);
                if (WidgetInstance)
                {
                    WidgetInstance->AddToViewport();
                    WidgetInstance->SetEndLevelScore(FinalScore);
                }
            }

            // Optionally: Stop decay timer or freeze gameplay here
        }
    }
}

