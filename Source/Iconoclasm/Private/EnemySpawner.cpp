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

    UE_LOG(LogTemp, Warning, TEXT("Array Sizes - EnemyTypes: %d, EnemyCounts: %d, SpawnPoints: %d"),
        EnemyTypes.Num(), EnemyCounts.Num(), SpawnPoints.Num());

    int32 MaxSpawns = FMath::Min3(EnemyTypes.Num(), EnemyCounts.Num(), SpawnPoints.Num());

    for (int32 i = 0; i < MaxSpawns; ++i)
    {
        // CRITICAL: Check spawn point validity BEFORE using it
        if (!IsValid(SpawnPoints[i]))
        {
            UE_LOG(LogTemp, Error, TEXT("SpawnPoint at index %d is NULL or invalid! Skipping..."), i);
            continue;
        }

        // Check enemy class validity
        if (!EnemyTypes[i])
        {
            UE_LOG(LogTemp, Error, TEXT("Enemy class at index %d is NULL! Skipping..."), i);
            continue;
        }

        // Now safe to get location
        FVector SpawnLocation = SpawnPoints[i]->GetActorLocation();
        int32 SpawnCount = EnemyCounts[i];

        UE_LOG(LogTemp, Warning, TEXT("Spawning %d enemies at index %d, location: %s"),
            SpawnCount, i, *SpawnLocation.ToString());

        for (int32 j = 0; j < SpawnCount; ++j)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

            AActor* SpawnedEnemy = GetWorld()->SpawnActor<AActor>(EnemyTypes[i], SpawnLocation, FRotator::ZeroRotator, SpawnParams);

            if (IsValid(SpawnedEnemy))
            {
                UE_LOG(LogTemp, Warning, TEXT("Successfully spawned: %s"), *SpawnedEnemy->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to spawn enemy %d at spawn point %d!"), j, i);
            }
        }
    }
}

void AEnemySpawner::ResetSpawner()
{
    // Re-enable the trigger so it can spawn enemies again
    SpawnTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    UE_LOG(LogTemp, Warning, TEXT("Enemy spawner reset, trigger re-enabled."));
}



