// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatMusicManager.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ACombatMusicManager* ACombatMusicManager::Instance = nullptr;

ACombatMusicManager::ACombatMusicManager()
{
    PrimaryActorTick.bCanEverTick = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    ThemeAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("ThemeAudio"));
    ThemeAudioComponent->bAutoActivate = false;
    ThemeAudioComponent->SetupAttachment(RootComponent);
}

void ACombatMusicManager::BeginPlay()
{
    Super::BeginPlay();

    Instance = this;

    if (ThemeTrack)
    {
        ThemeAudioComponent->SetSound(ThemeTrack);

        // IMPORTANT: do NOT play yet. We only start when first enemy spawns.
        bThemeStarted = false;

        // Set initial volumes for when we eventually start.
        CurrentVolume = GetCalmVolume();
        TargetVolume = CurrentVolume;
        ThemeAudioComponent->SetVolumeMultiplier(CurrentVolume);

        UE_LOG(LogTemp, Log, TEXT("Theme track ready (will NOT start until first enemy is spawned)."));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatMusicManager: ThemeTrack is null!"));
    }
}

void ACombatMusicManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bStoppingMusic)
    {
        UpdateStopFade(DeltaTime);
        return;
    }

    if (bIsFading)
    {
        UpdateVolumeFade(DeltaTime);
    }
}

void ACombatMusicManager::RegisterEnemies(int32 Count)
{
    ActiveEnemyCount += Count;
    ActiveEnemyCount = FMath::Max(0, ActiveEnemyCount);

    UE_LOG(LogTemp, Warning, TEXT("Registered %d enemies. Total active: %d"), Count, ActiveEnemyCount);

    // Start music ONLY when first enemies appear
    if (ActiveEnemyCount > 0)
    {
        StartThemeIfNeeded();
    }

    // If enemies exist, we are "in combat"
    if (ActiveEnemyCount > 0 && !bInCombat)
    {
        bInCombat = true;
        UE_LOG(LogTemp, Warning, TEXT("Combat started - raising music volume"));
    }

    UpdateTargetFromEnemyState();
}

void ACombatMusicManager::OnEnemyKilled()
{
    ActiveEnemyCount = FMath::Max(0, ActiveEnemyCount - 1);
    UE_LOG(LogTemp, Warning, TEXT("Enemy killed. Remaining enemies across ALL spawners: %d"), ActiveEnemyCount);

    // If all enemies are dead, go calm (duck volume)
    if (bInCombat && ActiveEnemyCount <= 0)
    {
        bInCombat = false;
        UE_LOG(LogTemp, Warning, TEXT("All enemies eliminated - lowering music volume"));
    }

    UpdateTargetFromEnemyState();
}

void ACombatMusicManager::StartThemeIfNeeded()
{
    if (!ThemeAudioComponent || !ThemeTrack)
        return;

    // If StopAllMusic fade-out is in progress, don't start again until that finishes
    if (bStoppingMusic)
        return;

    if (bThemeStarted && ThemeAudioComponent->IsPlaying())
        return;

    // Start at random position (your original behavior)
    const float TrackDuration = ThemeTrack->Duration;
    const float RandomStartTime = FMath::FRandRange(0.0f, FMath::Max(0.0f, TrackDuration - 1.0f));

    ThemeAudioComponent->Play(RandomStartTime);
    bThemeStarted = true;

    UE_LOG(LogTemp, Log, TEXT("Starting theme track at %.2f seconds (first enemy spawned)"), RandomStartTime);
}

void ACombatMusicManager::UpdateTargetFromEnemyState()
{
    if (!ThemeAudioComponent || !ThemeTrack)
        return;

    // If we're fading out from StopAllMusic, don't fight it
    if (bStoppingMusic)
        return;

    // NEW RULE: if theme has never started yet, don't do any fades/volume work
    if (!bThemeStarted)
        return;

    const float Desired = (ActiveEnemyCount > 0) ? GetCombatVolume() : GetCalmVolume();
    BeginVolumeFade(Desired);
}

void ACombatMusicManager::BeginVolumeFade(float NewTargetVolume)
{
    NewTargetVolume = FMath::Max(0.0f, NewTargetVolume);

    const bool bNearlySame =
        FMath::IsNearlyEqual(TargetVolume, NewTargetVolume, 0.001f) &&
        FMath::IsNearlyEqual(CurrentVolume, NewTargetVolume, 0.001f);

    if (bNearlySame)
        return;

    TargetVolume = NewTargetVolume;
    StartFadeVolume = CurrentVolume;
    FadeTimer = 0.0f;
    bIsFading = true;

    UE_LOG(LogTemp, Log, TEXT("Music fade started: %.2f -> %.2f (FadeTime %.2fs)"),
        StartFadeVolume, TargetVolume, FadeTime);
}

void ACombatMusicManager::UpdateVolumeFade(float DeltaTime)
{
    if (!ThemeAudioComponent)
    {
        bIsFading = false;
        return;
    }

    FadeTimer += DeltaTime;
    const float Alpha = FMath::Clamp(FadeTimer / FadeTime, 0.0f, 1.0f);

    CurrentVolume = FMath::Lerp(StartFadeVolume, TargetVolume, Alpha);
    ThemeAudioComponent->SetVolumeMultiplier(CurrentVolume);

    if (Alpha >= 1.0f)
    {
        bIsFading = false;
        CurrentVolume = TargetVolume;
        ThemeAudioComponent->SetVolumeMultiplier(CurrentVolume);

        UE_LOG(LogTemp, Log, TEXT("Music fade complete. Current volume: %.2f"), CurrentVolume);
    }
}

ACombatMusicManager* ACombatMusicManager::GetInstance(UWorld* World)
{
    if (!Instance && World)
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(World, ACombatMusicManager::StaticClass(), FoundActors);

        if (FoundActors.Num() > 0)
        {
            Instance = Cast<ACombatMusicManager>(FoundActors[0]);
        }
    }

    return Instance;
}

void ACombatMusicManager::StopAllMusic(bool bImmediate, float FadeOutDuration)
{
    UE_LOG(LogTemp, Warning, TEXT("StopAllMusic called - Immediate: %s"), bImmediate ? TEXT("true") : TEXT("false"));

    if (!ThemeAudioComponent)
        return;

    if (bImmediate)
    {
        if (ThemeAudioComponent->IsPlaying())
        {
            ThemeAudioComponent->Stop();
        }

        bInCombat = false;
        bIsFading = false;
        bStoppingMusic = false;

        ActiveEnemyCount = 0;

        CurrentVolume = 0.0f;
        TargetVolume = 0.0f;

        // IMPORTANT: allow it to start again on next enemy spawn
        bThemeStarted = false;

        UE_LOG(LogTemp, Warning, TEXT("Theme music stopped immediately"));
    }
    else
    {
        // If it's not even started / not playing, just reset state
        if (!bThemeStarted || !ThemeAudioComponent->IsPlaying())
        {
            bInCombat = false;
            bIsFading = false;
            bStoppingMusic = false;

            ActiveEnemyCount = 0;
            CurrentVolume = 0.0f;
            TargetVolume = 0.0f;

            bThemeStarted = false;

            UE_LOG(LogTemp, Warning, TEXT("StopAllMusic called but theme was not playing - reset state"));
            return;
        }

        bStoppingMusic = true;
        bIsFading = false;

        StopFadeTime = FMath::Max(0.01f, FadeOutDuration);
        FadeTimer = 0.0f;

        StartFadeVolume = CurrentVolume;

        UE_LOG(LogTemp, Warning, TEXT("Starting theme fade out over %.2f seconds"), StopFadeTime);
    }
}

void ACombatMusicManager::UpdateStopFade(float DeltaTime)
{
    if (!ThemeAudioComponent)
    {
        bStoppingMusic = false;
        return;
    }

    FadeTimer += DeltaTime;
    const float Alpha = FMath::Clamp(FadeTimer / StopFadeTime, 0.0f, 1.0f);

    const float NewVolume = FMath::Lerp(StartFadeVolume, 0.0f, Alpha);
    CurrentVolume = NewVolume;

    if (ThemeAudioComponent->IsPlaying())
    {
        ThemeAudioComponent->SetVolumeMultiplier(NewVolume);
    }

    if (Alpha >= 1.0f)
    {
        ThemeAudioComponent->Stop();

        bStoppingMusic = false;
        bInCombat = false;
        ActiveEnemyCount = 0;

        TargetVolume = 0.0f;
        CurrentVolume = 0.0f;

        // IMPORTANT: allow it to start again on next enemy spawn
        bThemeStarted = false;

        UE_LOG(LogTemp, Warning, TEXT("Theme fade out complete - music stopped"));
    }
}