// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemySpawner.h"
#include "GruntEnemyCharacter.h"
#include "GruntAIController.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "IconoclasmCharacter.h"
#include "FlyingEnemyCharacter.h"
#include "FlyingAIController.h"

// Sets default values
AEnemySpawner::AEnemySpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    // Create the trigger box
    SpawnTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnTrigger"));
    RootComponent = SpawnTrigger;

    // Bind the overlap event
    SpawnTrigger->OnComponentBeginOverlap.AddDynamic(this, &AEnemySpawner::OnTriggerEnter);

}

// Called when the game starts or when spawned
void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();
	
}

void AEnemySpawner::OnTriggerEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor != this && OtherActor->IsA(AIconoclasmCharacter::StaticClass()))  // Replace with your actual player class
    {
        // Add a debug log message
        UE_LOG(LogTemp, Warning, TEXT("Overlap detected with player!"));

        SpawnEnemies();
        SpawnTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);  // Disable after trigger
    }
    else
    {
        // Add a debug log message for other actors
        UE_LOG(LogTemp, Warning, TEXT("Overlap detected, but it's not the player."));
    }
}

// Called every frame
void AEnemySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEnemySpawner::SpawnEnemies()
{
    if (EnemyTypes.Num() == 0 || EnemyCounts.Num() == 0 || SpawnPoints.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Enemy types, counts, or spawn points not set!"));
        return;
    }

    for (int32 i = 0; i < SpawnPoints.Num(); ++i)
    {
        if (!EnemyTypes.IsValidIndex(i) || !SpawnPoints.IsValidIndex(i) || !SpawnPoints[i])
        {
            UE_LOG(LogTemp, Error, TEXT("Invalid spawn data at index: %d"), i);
            continue;
        }

        UClass* EnemyClass = EnemyTypes[i];
        if (!EnemyClass)
        {
            UE_LOG(LogTemp, Error, TEXT("Enemy class at index %d is NULL!"), i);
            continue;
        }

        int32 SpawnCount = EnemyCounts[i];

        for (int32 j = 0; j < SpawnCount; ++j)
        {
            FVector SpawnLocation = SpawnPoints[i]->GetActorLocation();
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

            AActor* SpawnedEnemy = GetWorld()->SpawnActor<AActor>(EnemyClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

            if (!SpawnedEnemy)
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to spawn enemy at index %d!"), i);
                continue;
            }

            UE_LOG(LogTemp, Warning, TEXT("Successfully spawned: %s"), *SpawnedEnemy->GetName());
        }
    }
}


