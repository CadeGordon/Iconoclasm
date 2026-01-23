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

    // Debug: periodically validate enemy count
    if (bSpawningActive && GetWorld()->GetTimeSeconds() - LastValidationTime > 2.0f)
    {
        LastValidationTime = GetWorld()->GetTimeSeconds();
        ValidateEnemyCount();
    }
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
    ActiveEnemiesCount = 0;
    bWaitingForWaveClear = false;

    // Clear tracking arrays
    TrackedEnemies.Empty();

    // Clear any existing timers
    GetWorld()->GetTimerManager().ClearTimer(WaveDelayTimerHandle);

    UE_LOG(LogTemp, Warning, TEXT("=== SPAWNER STARTED ==="));
    SpawnCurrentWave();
}

void AEnemySpawner::SpawnCurrentWave()
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
        bWaitingForWaveClear = false;
        CleanupAllEnemies();
        UnlockCompletionDoors();
        return;
    }

    if (!Waves.IsValidIndex(CurrentWaveIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid wave index: %d"), CurrentWaveIndex);
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("=== SPAWNING WAVE %d of %d ==="), CurrentWaveIndex + 1, Waves.Num());

    const FEnemyWave& Wave = Waves[CurrentWaveIndex];

    // Check if wave has enemies
    if (Wave.Enemies.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Wave %d has no enemies configured! Advancing immediately."), CurrentWaveIndex + 1);
        CurrentWaveIndex++;

        // Recursively spawn next wave (handles empty waves in sequence)
        if (CurrentWaveIndex < Waves.Num())
        {
            SpawnCurrentWave();
        }
        else
        {
            bSpawningActive = false;
            UnlockCompletionDoors();
        }
        return;
    }

    // Spawn the enemies
    int32 SpawnedCount = SpawnEnemiesInWave(Wave);

    UE_LOG(LogTemp, Warning, TEXT("Wave %d spawned %d enemies. Total active: %d, Tracked: %d"),
        CurrentWaveIndex + 1, SpawnedCount, ActiveEnemiesCount, TrackedEnemies.Num());

    // Determine how to advance to next wave
    if (SpawnedCount == 0)
    {
        // No enemies spawned, advance immediately
        UE_LOG(LogTemp, Warning, TEXT("No enemies spawned, advancing immediately"));
        CurrentWaveIndex++;
        SpawnCurrentWave();
    }
    else if (bRequireWaveClearBeforeNext)
    {
        // Wait for all enemies to be killed
        UE_LOG(LogTemp, Warning, TEXT("Waiting for wave clear before advancing..."));
        bWaitingForWaveClear = true;
    }
    else if (bAutoStartNextWave)
    {
        // Use delay or spawn immediately
        CurrentWaveIndex++;

        if (Wave.DelayBeforeNextWave > 0.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("Starting next wave after %.1f second delay"), Wave.DelayBeforeNextWave);
            GetWorld()->GetTimerManager().SetTimer(WaveDelayTimerHandle, this,
                &AEnemySpawner::SpawnCurrentWave, Wave.DelayBeforeNextWave, false);
        }
        else
        {
            // Instant spawn - no delay needed
            UE_LOG(LogTemp, Warning, TEXT("Instantly spawning next wave"));
            SpawnCurrentWave();
        }
    }
}

int32 AEnemySpawner::SpawnEnemiesInWave(const FEnemyWave& Wave)
{
    int32 WaveSpawnedCount = 0;
    int32 WaveExpectedCount = 0;

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

        WaveExpectedCount += SpawnInfo.Count;

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
                // Find and bind to health component
                UHealthComponent* HealthComp = SpawnedEnemy->FindComponentByClass<UHealthComponent>();
                if (HealthComp)
                {
                    // Check if already dead (shouldn't happen but let's be safe)
                    if (HealthComp->IsDead())
                    {
                        UE_LOG(LogTemp, Error, TEXT("Enemy spawned already dead: %s"), *SpawnedEnemy->GetName());
                        SpawnedEnemy->Destroy();
                        continue;
                    }

                    // Bind to death event - use lambda to capture enemy reference
                    if (!HealthComp->OnDeath.IsAlreadyBound(this, &AEnemySpawner::OnEnemyDestroyed))
                    {
                        HealthComp->OnDeath.AddDynamic(this, &AEnemySpawner::OnEnemyDestroyed);
                    }

                    // Track this enemy
                    TrackedEnemies.Add(SpawnedEnemy);
                    WaveSpawnedCount++;
                    ActiveEnemiesCount++;

                    UE_LOG(LogTemp, Log, TEXT("Spawned and tracking: %s (Total tracked: %d)"),
                        *SpawnedEnemy->GetName(), TrackedEnemies.Num());
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("ERROR: Enemy %s has NO HealthComponent! Destroying it."),
                        *SpawnedEnemy->GetName());
                    SpawnedEnemy->Destroy();
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to spawn enemy at location %s!"), *FinalLocation.ToString());
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Spawn summary: %d/%d enemies spawned and tracked successfully"),
        WaveSpawnedCount, WaveExpectedCount);

    // Notify music manager
    if (WaveSpawnedCount > 0)
    {
        ACombatMusicManager* MusicManager = ACombatMusicManager::GetInstance(GetWorld());
        if (MusicManager)
        {
            MusicManager->RegisterEnemies(WaveSpawnedCount);
        }
    }

    return WaveSpawnedCount;
}

void AEnemySpawner::OnEnemyDestroyed()
{
    ActiveEnemiesCount = FMath::Max(0, ActiveEnemiesCount - 1);

    UE_LOG(LogTemp, Warning, TEXT("Enemy destroyed. Active count: %d, Tracked: %d, WaitingForClear: %s"),
        ActiveEnemiesCount, TrackedEnemies.Num(), bWaitingForWaveClear ? TEXT("YES") : TEXT("NO"));

    // Clean up null/invalid entries from tracked array
    CleanupTrackedEnemies();

    // Only check wave completion if we're waiting for it
    if (bWaitingForWaveClear)
    {
        CheckWaveCompletion();
    }
}

void AEnemySpawner::CheckWaveCompletion()
{
    // Only proceed if we're actively waiting for wave clear
    if (!bWaitingForWaveClear || !bSpawningActive)
    {
        return;
    }

    // Validate the count before checking completion
    ValidateEnemyCount();

    // Wave is complete when all enemies are dead
    if (ActiveEnemiesCount <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("=== WAVE %d CLEARED ==="), CurrentWaveIndex + 1);

        bWaitingForWaveClear = false;

        // Clean up any remaining tracked enemies (they should all be dead/destroyed)
        CleanupTrackedEnemies();

        // Get the current wave's delay before advancing
        float Delay = 0.0f;
        if (Waves.IsValidIndex(CurrentWaveIndex))
        {
            Delay = Waves[CurrentWaveIndex].DelayBeforeNextWave;
        }

        // Advance to next wave
        CurrentWaveIndex++;

        // Check if there are more waves
        if (CurrentWaveIndex < Waves.Num())
        {
            if (bAutoStartNextWave)
            {
                if (Delay > 0.0f)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Next wave in %.1f seconds..."), Delay);
                    GetWorld()->GetTimerManager().SetTimer(WaveDelayTimerHandle, this,
                        &AEnemySpawner::SpawnCurrentWave, Delay, false);
                }
                else
                {
                    // Instant spawn - no delay
                    UE_LOG(LogTemp, Warning, TEXT("Instantly spawning next wave"));
                    SpawnCurrentWave();
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Wave cleared but auto-start disabled"));
                bSpawningActive = false;
            }
        }
        else
        {
            // All waves complete
            UE_LOG(LogTemp, Warning, TEXT("=== ALL WAVES COMPLETE ==="));
            bSpawningActive = false;
            CleanupAllEnemies();
            UnlockCompletionDoors();
        }
    }
}

void AEnemySpawner::ValidateEnemyCount()
{
    // Remove invalid entries
    CleanupTrackedEnemies();

    // Count how many enemies are actually alive
    int32 ActualAliveCount = 0;
    for (AActor* Enemy : TrackedEnemies)
    {
        if (IsValid(Enemy))
        {
            UHealthComponent* HealthComp = Enemy->FindComponentByClass<UHealthComponent>();
            if (HealthComp && !HealthComp->IsDead())
            {
                ActualAliveCount++;
            }
        }
    }

    // If counts don't match, fix it
    if (ActualAliveCount != ActiveEnemiesCount)
    {
        UE_LOG(LogTemp, Error, TEXT("ENEMY COUNT MISMATCH! Tracked alive: %d, ActiveCount: %d. Correcting..."),
            ActualAliveCount, ActiveEnemiesCount);

        ActiveEnemiesCount = ActualAliveCount;

        // If we're waiting for clear and count is now 0, trigger completion
        if (bWaitingForWaveClear && ActiveEnemiesCount == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("Count corrected to 0 - triggering wave completion"));
            CheckWaveCompletion();
        }
    }
}

void AEnemySpawner::CleanupTrackedEnemies()
{
    // Remove null or destroyed entries from tracking array
    int32 RemovedCount = TrackedEnemies.RemoveAll([](const AActor* Enemy)
        {
            return !IsValid(Enemy);
        });

    if (RemovedCount > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Removed %d invalid enemies from tracking. Remaining: %d"),
            RemovedCount, TrackedEnemies.Num());
    }
}

void AEnemySpawner::CleanupAllEnemies()
{
    UE_LOG(LogTemp, Warning, TEXT("Cleaning up all tracked enemies..."));

    // Destroy any remaining enemies (shouldn't happen in normal play)
    for (AActor* Enemy : TrackedEnemies)
    {
        if (IsValid(Enemy))
        {
            UE_LOG(LogTemp, Warning, TEXT("Force destroying enemy: %s"), *Enemy->GetName());
            Enemy->Destroy();
        }
    }

    TrackedEnemies.Empty();
    ActiveEnemiesCount = 0;

    UE_LOG(LogTemp, Warning, TEXT("All enemies cleaned up."));
}

void AEnemySpawner::ResetSpawner()
{
    SpawnTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CurrentWaveIndex = 0;
    ActiveEnemiesCount = 0;
    bSpawningActive = false;
    bWaitingForWaveClear = false;
    GetWorld()->GetTimerManager().ClearTimer(WaveDelayTimerHandle);

    CleanupAllEnemies();

    UE_LOG(LogTemp, Warning, TEXT("Enemy spawner reset."));
}

void AEnemySpawner::UnlockCompletionDoors()
{
    if (DoorsToUnlockOnCompletion.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("No doors to unlock."));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Unlocking %d doors after completion!"), DoorsToUnlockOnCompletion.Num());

    for (ASlidingDoor* Door : DoorsToUnlockOnCompletion)
    {
        if (IsValid(Door))
        {
            Door->UnlockDoor();
            UE_LOG(LogTemp, Warning, TEXT("Unlocked door: %s"), *Door->GetName());
        }
    }
}
