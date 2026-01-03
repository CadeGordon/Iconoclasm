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

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Combat Music")
    void RegisterEnemies(int32 Count);

    UFUNCTION(BlueprintCallable, Category = "Combat Music")
    void OnEnemyKilled();

    static ACombatMusicManager* GetInstance(UWorld* World);

    UFUNCTION(BlueprintCallable, Category = "Combat Music")
    void StopAllMusic(bool bImmediate = false, float FadeOutDuration = 1.5f);

    // Single theme track (your old ActionTrack)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Music")
    USoundBase* ThemeTrack = nullptr;

    // Base volume when enemies exist
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Music", meta = (ClampMin = "0.0"))
    float MusicVolume = 1.0f;

    // Volume multiplier when there are NO enemies (idle/calm)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Music", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CalmVolumeMultiplier = 0.35f;

    // How quickly we fade between calm and combat volume
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Music", meta = (ClampMin = "0.01"))
    float FadeTime = 1.0f;

    static ACombatMusicManager* Instance;

    UPROPERTY(VisibleAnywhere, Category = "Combat Music")
    UAudioComponent* ThemeAudioComponent = nullptr;

    int32 ActiveEnemyCount = 0;
    bool bInCombat = false;

    // NEW: track whether the theme has ever been started
    bool bThemeStarted = false;

    // Fade / ducking state
    bool bIsFading = false;
    bool bStoppingMusic = false;

    float FadeTimer = 0.0f;
    float StopFadeTime = 1.5f;

    float CurrentVolume = 0.0f;
    float StartFadeVolume = 0.0f;
    float TargetVolume = 0.0f;

    void StartThemeIfNeeded();           // NEW: only starts on first enemy spawn
    void UpdateTargetFromEnemyState();

    void BeginVolumeFade(float NewTargetVolume);
    void UpdateVolumeFade(float DeltaTime);

    void UpdateStopFade(float DeltaTime);

    float GetCombatVolume() const { return MusicVolume; }
    float GetCalmVolume() const { return MusicVolume * CalmVolumeMultiplier; }
};
