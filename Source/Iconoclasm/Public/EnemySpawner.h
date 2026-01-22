// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"

class AGruntEnemyCharacter;
class AFlyingEnemyCharacter;

// Struct to define a single enemy spawn in a wave
USTRUCT(BlueprintType)
struct FEnemySpawnInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn")
    TSubclassOf<AActor> EnemyClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn")
    int32 Count = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Spawn")
    AActor* SpawnPoint = nullptr;
};

// Struct to define a complete wave
USTRUCT(BlueprintType)
struct FEnemyWave
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
    TArray<FEnemySpawnInfo> Enemies;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
    float DelayBeforeNextWave = 5.0f;
};

UCLASS()
class ICONOCLASM_API AEnemySpawner : public AActor
{
	GENERATED_BODY()
	
public:
    AEnemySpawner();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBoxComponent* SpawnTrigger;

    // Wave configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    TArray<FEnemyWave> Waves;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    bool bAutoStartNextWave = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    bool bRequireWaveClearBeforeNext = true;

    // Doors to unlock after final wave
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave System")
    TArray<class ASlidingDoor*> DoorsToUnlockOnCompletion;

    UFUNCTION()
    void OnTriggerEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    void StartWaveSpawning();

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    void SpawnCurrentWave();

   // UFUNCTION(BlueprintCallable, Category = "Wave System")
    //void SpawnWave(int32 WaveIndex);

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    void ResetSpawner();

    UFUNCTION(BlueprintCallable, Category = "Wave System")
    void OnEnemyDestroyed();

private:
    int32 CurrentWaveIndex = 0;
    int32 ActiveEnemiesCount = 0;
    bool bSpawningActive = false;
    FTimerHandle WaveDelayTimerHandle;

    bool bWaitingForWaveClear = false;

    int32 SpawnEnemiesInWave(const FEnemyWave& Wave);
    void CheckWaveCompletion();
    void UnlockCompletionDoors();


};
