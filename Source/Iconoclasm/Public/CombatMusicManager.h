// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundBase.h"
#include "CombatMusicManager.generated.h"

class UAudioComponent;

UCLASS()
class ICONOCLASM_API ACombatMusicManager : public AActor
{
	GENERATED_BODY()
	
public:
    ACombatMusicManager();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Called by spawner when enemies are spawned
    UFUNCTION(BlueprintCallable, Category = "Combat Music")
    void RegisterEnemies(int32 Count);

    // Called when an enemy dies
    UFUNCTION(BlueprintCallable, Category = "Combat Music")
    void OnEnemyKilled();

    // Manual combat state control
    UFUNCTION(BlueprintCallable, Category = "Combat Music")
    void StartCombat();

    UFUNCTION(BlueprintCallable, Category = "Combat Music")
    void EndCombat();

    // Stop all music tracks
    UFUNCTION(BlueprintCallable, Category = "Combat Music")
    void StopAllMusic(bool bImmediate = false, float FadeOutDuration = 2.0f);

protected:
    // Music tracks
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
    USoundBase* ChillTrack;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
    USoundBase* ActionTrack;

    // Audio components
    UPROPERTY()
    UAudioComponent* ChillAudioComponent;

    UPROPERTY()
    UAudioComponent* ActionAudioComponent;

    // Fade settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
    float FadeTime = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
    float MusicVolume = 0.5f;

private:
    int32 ActiveEnemyCount = 0;
    bool bInCombat = false;
    bool bIsFading = false;
    float FadeTimer = 0.0f;
    bool bFadingToAction = false;
    bool bStoppingMusic = false;
    float StopFadeTime = 2.0f;

    void UpdateMusicState();
    void CrossfadeToAction();
    void CrossfadeToChill();
    void UpdateFade(float DeltaTime);
    void UpdateStopFade(float DeltaTime);

public:
    // Singleton access
    static ACombatMusicManager* GetInstance(UWorld* World);

private:
    static ACombatMusicManager* Instance;
};
