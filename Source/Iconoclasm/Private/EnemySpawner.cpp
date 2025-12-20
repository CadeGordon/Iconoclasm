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
#include "CombatMusicManager.h"
#include "HealthComponent.h"
#include "SlidingDoor.h"
#include "TimerManager.h"

AEnemySpawner::AEnemySpawner()
{
    PrimaryActorTick.bCanEverTick = true;

    SpawnTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnTrigger"));
    RootComponent = SpawnTrigger;

    SpawnTrigger->OnComponentBeginOverlap.AddDynamic(this, &AEnemySpawner::OnTriggerEnter);
}

void AEnemySpawner::BeginPlay()
{
    Super::BeginPlay();
}

void AEnemySpawner::OnTriggerEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor != this && OtherActor->IsA(AIconoclasmCharacter::StaticClass()))
    {
        UE_LOG(LogTemp, Warning, TEXT("Player triggered spawner!"));
        StartWaveSpawning();
        SpawnTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

void AEnemySpawner::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AEnemySpawner::StartWaveSpawning()
{
    if (Waves.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("No waves configured!"));
        return;
    }

    bSpawningActive = true;
    CurrentWaveIndex = 0;
    SpawnNextWave();
}

void AEnemySpawner::SpawnNextWave()
{
    if (!bSpawningActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("Spawning not active!"));
        return;
    }

    if (CurrentWaveIndex >= Waves.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("All waves completed!"));
        bSpawningActive = false;

        // Unlock doors when all waves are complete
        UnlockCompletionDoors();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Starting wave %d of %d"), CurrentWaveIndex + 1, Waves.Num());
    SpawnWave(CurrentWaveIndex);
    CurrentWaveIndex++;
}

void AEnemySpawner::SpawnWave(int32 WaveIndex)
{
    if (!Waves.IsValidIndex(WaveIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid wave index: %d"), WaveIndex);
        return;
    }

    const FEnemyWave& Wave = Waves[WaveIndex];
    SpawnEnemiesInWave(Wave);
}

void AEnemySpawner::SpawnEnemiesInWave(const FEnemyWave& Wave)
{
    if (Wave.Enemies.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Wave has no enemies configured!"));

        if (bAutoStartNextWave)
        {
            GetWorld()->GetTimerManager().SetTimer(WaveDelayTimerHandle, this,
                &AEnemySpawner::SpawnNextWave, Wave.DelayBeforeNextWave, false);
        }
        return;
    }

    int32 WaveSpawnedCount = 0;

    for (const FEnemySpawnInfo& SpawnInfo : Wave.Enemies)
    {
        if (!SpawnInfo.EnemyClass)
        {
            UE_LOG(LogTemp, Error, TEXT("Enemy class not set in spawn info!"));
            continue;
        }

        if (!IsValid(SpawnInfo.SpawnPoint))
        {
            UE_LOG(LogTemp, Error, TEXT("Spawn point not set or invalid!"));
            continue;
        }

        FVector SpawnLocation = SpawnInfo.SpawnPoint->GetActorLocation();
        FRotator SpawnRotation = SpawnInfo.SpawnPoint->GetActorRotation();

        for (int32 i = 0; i < SpawnInfo.Count; ++i)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride =
                ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

            // Add slight offset for multiple enemies at same point
            FVector FinalLocation = SpawnLocation;
            if (i > 0)
            {
                FinalLocation += FVector(
                    FMath::RandRange(-100.0f, 100.0f),
                    FMath::RandRange(-100.0f, 100.0f),
                    0.0f
                );
            }

            AActor* SpawnedEnemy = GetWorld()->SpawnActor<AActor>(
                SpawnInfo.EnemyClass, FinalLocation, SpawnRotation, SpawnParams);

            if (IsValid(SpawnedEnemy))
            {
                WaveSpawnedCount++;
                ActiveEnemiesCount++;
                UE_LOG(LogTemp, Warning, TEXT("Spawned: %s"), *SpawnedEnemy->GetName());

                // Bind to health component's death event
                if (UHealthComponent* HealthComp = SpawnedEnemy->FindComponentByClass<UHealthComponent>())
                {
                    HealthComp->OnDeath.AddDynamic(this, &AEnemySpawner::OnEnemyDestroyed);
                    UE_LOG(LogTemp, Log, TEXT("Bound to health component death event"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to spawn enemy!"));
            }
        }
    }

    // Notify music manager
    if (WaveSpawnedCount > 0)
    {
        ACombatMusicManager* MusicManager = ACombatMusicManager::GetInstance(GetWorld());
        if (MusicManager)
        {
            MusicManager->RegisterEnemies(WaveSpawnedCount);
            UE_LOG(LogTemp, Warning, TEXT("Registered %d enemies with music manager"), WaveSpawnedCount);
        }
    }

    // Handle next wave spawning
    if (bAutoStartNextWave)
    {
        if (bRequireWaveClearBeforeNext)
        {
            // Check wave completion will handle starting next wave
            UE_LOG(LogTemp, Warning, TEXT("Waiting for wave clear before next wave..."));
        }
        else
        {
            // Start next wave after delay
            GetWorld()->GetTimerManager().SetTimer(WaveDelayTimerHandle, this,
                &AEnemySpawner::SpawnNextWave, Wave.DelayBeforeNextWave, false);
        }
    }
}

void AEnemySpawner::OnEnemyDestroyed()
{
    ActiveEnemiesCount--;
    UE_LOG(LogTemp, Warning, TEXT("Enemy destroyed. Remaining: %d"), ActiveEnemiesCount);

    CheckWaveCompletion();
}

void AEnemySpawner::CheckWaveCompletion()
{
    UE_LOG(LogTemp, Warning, TEXT("CheckWaveCompletion - Active enemies: %d, bSpawningActive: %s, CurrentWaveIndex: %d/%d"),
        ActiveEnemiesCount, bSpawningActive ? TEXT("true") : TEXT("false"), CurrentWaveIndex, Waves.Num());

    if (ActiveEnemiesCount <= 0 && bSpawningActive && bRequireWaveClearBeforeNext)
    {
        // Check if there are more waves to spawn
        if (CurrentWaveIndex < Waves.Num())
        {
            UE_LOG(LogTemp, Warning, TEXT("Wave cleared! Starting next wave (wave %d)..."), CurrentWaveIndex + 1);

            if (Waves.IsValidIndex(CurrentWaveIndex - 1))
            {
                float Delay = Waves[CurrentWaveIndex - 1].DelayBeforeNextWave;
                GetWorld()->GetTimerManager().SetTimer(WaveDelayTimerHandle, this,
                    &AEnemySpawner::SpawnNextWave, Delay, false);
            }
            else
            {
                SpawnNextWave();
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("All waves completed after final wave clear!"));
            bSpawningActive = false;
            UnlockCompletionDoors();
        }
    }
}

void AEnemySpawner::ResetSpawner()
{
    SpawnTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CurrentWaveIndex = 0;
    ActiveEnemiesCount = 0;
    bSpawningActive = false;
    GetWorld()->GetTimerManager().ClearTimer(WaveDelayTimerHandle);
    UE_LOG(LogTemp, Warning, TEXT("Enemy spawner reset."));
}

void AEnemySpawner::UnlockCompletionDoors()
{
    if (DoorsToUnlockOnCompletion.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("No doors to unlock."));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Unlocking %d doors after wave completion!"), DoorsToUnlockOnCompletion.Num());

    for (ASlidingDoor* Door : DoorsToUnlockOnCompletion)
    {
        if (IsValid(Door))
        {
            Door->UnlockDoor();
            UE_LOG(LogTemp, Warning, TEXT("Unlocked door: %s"), *Door->GetName());
        }
    }
}