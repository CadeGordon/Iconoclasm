// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatMusicManager.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

ACombatMusicManager* ACombatMusicManager::Instance = nullptr;

ACombatMusicManager::ACombatMusicManager()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create audio components
    ChillAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("ChillAudio"));
    ChillAudioComponent->bAutoActivate = false;
    ChillAudioComponent->SetupAttachment(RootComponent);

    ActionAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("ActionAudio"));
    ActionAudioComponent->bAutoActivate = false;
    ActionAudioComponent->SetupAttachment(RootComponent);
}

void ACombatMusicManager::BeginPlay()
{
    Super::BeginPlay();

    Instance = this;

    // Set up audio components but DON'T play chill track yet
    if (ChillTrack)
    {
        ChillAudioComponent->SetSound(ChillTrack);
        ChillAudioComponent->SetVolumeMultiplier(MusicVolume);
        // Don't play until after combat
        UE_LOG(LogTemp, Log, TEXT("Chill track ready but not playing yet"));
    }

    if (ActionTrack)
    {
        ActionAudioComponent->SetSound(ActionTrack);
        ActionAudioComponent->SetVolumeMultiplier(0.0f);
    }
}

void ACombatMusicManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bStoppingMusic)
    {
        UpdateStopFade(DeltaTime);
    }
    else if (bIsFading)
    {
        UpdateFade(DeltaTime);
    }
}

void ACombatMusicManager::RegisterEnemies(int32 Count)
{
    ActiveEnemyCount += Count;
    UE_LOG(LogTemp, Warning, TEXT("Registered %d enemies. Total active: %d"), Count, ActiveEnemyCount);

    // Start combat if we have enemies and aren't already in combat
    if (!bInCombat && ActiveEnemyCount > 0)
    {
        StartCombat();
    }
}

void ACombatMusicManager::OnEnemyKilled()
{
    ActiveEnemyCount = FMath::Max(0, ActiveEnemyCount - 1);
    UE_LOG(LogTemp, Warning, TEXT("Enemy killed. Remaining enemies across ALL spawners: %d"), ActiveEnemyCount);

    // Only end combat when ALL enemies from ALL spawners are dead
    if (bInCombat && ActiveEnemyCount <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("All enemies eliminated - ending combat"));
        EndCombat();
    }
}

void ACombatMusicManager::StartCombat()
{
    if (bInCombat)
        return;

    bInCombat = true;
    UE_LOG(LogTemp, Warning, TEXT("Combat started - switching to action music"));
    CrossfadeToAction();
}

void ACombatMusicManager::EndCombat()
{
    if (!bInCombat)
        return;

    bInCombat = false;
    UE_LOG(LogTemp, Warning, TEXT("Combat ended - switching to chill music"));
    CrossfadeToChill();
}

void ACombatMusicManager::CrossfadeToAction()
{
    if (!ActionAudioComponent->IsPlaying())
    {
        // Start at a random position in the track
        if (ActionTrack)
        {
            float TrackDuration = ActionTrack->Duration;
            float RandomStartTime = FMath::FRandRange(0.0f, FMath::Max(0.0f, TrackDuration - 1.0f));
            ActionAudioComponent->Play(RandomStartTime);
            UE_LOG(LogTemp, Log, TEXT("Starting action track at %.2f seconds"), RandomStartTime);
        }
        else
        {
            ActionAudioComponent->Play();
        }
    }

    bIsFading = true;
    bFadingToAction = true;
    FadeTimer = 0.0f;
}

void ACombatMusicManager::CrossfadeToChill()
{
    // Always start at a random position, even if already playing
    if (ChillTrack)
    {
        float TrackDuration = ChillTrack->Duration;
        float RandomStartTime = FMath::FRandRange(0.0f, FMath::Max(0.0f, TrackDuration - 1.0f));

        if (!ChillAudioComponent->IsPlaying())
        {
            ChillAudioComponent->Play(RandomStartTime);
            UE_LOG(LogTemp, Log, TEXT("Starting chill track at %.2f seconds"), RandomStartTime);
        }
        else
        {
            // If already playing, just let it continue from current position
            UE_LOG(LogTemp, Log, TEXT("Chill track already playing, continuing from current position"));
        }
    }
    else
    {
        if (!ChillAudioComponent->IsPlaying())
        {
            ChillAudioComponent->Play();
        }
    }

    bIsFading = true;
    bFadingToAction = false;
    FadeTimer = 0.0f;
}

void ACombatMusicManager::UpdateFade(float DeltaTime)
{
    FadeTimer += DeltaTime;
    float FadeAlpha = FMath::Clamp(FadeTimer / FadeTime, 0.0f, 1.0f);

    if (bFadingToAction)
    {
        // Fade in action, fade out chill
        ActionAudioComponent->SetVolumeMultiplier(FadeAlpha * MusicVolume);
        ChillAudioComponent->SetVolumeMultiplier((1.0f - FadeAlpha) * MusicVolume);
    }
    else
    {
        // Fade in chill, fade out action
        ChillAudioComponent->SetVolumeMultiplier(FadeAlpha * MusicVolume);
        ActionAudioComponent->SetVolumeMultiplier((1.0f - FadeAlpha) * MusicVolume);
    }

    // Finish fade
    if (FadeAlpha >= 1.0f)
    {
        bIsFading = false;

        // Stop the silent track to save resources
        if (bFadingToAction)
        {
            ChillAudioComponent->Stop();
            UE_LOG(LogTemp, Log, TEXT("Fade to action complete - stopped chill track"));
        }
        else
        {
            ActionAudioComponent->Stop();
            UE_LOG(LogTemp, Log, TEXT("Fade to chill complete - stopped action track"));
        }
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

    if (bImmediate)
    {
        // Stop both tracks immediately
        if (ChillAudioComponent && ChillAudioComponent->IsPlaying())
        {
            ChillAudioComponent->Stop();
        }
        if (ActionAudioComponent && ActionAudioComponent->IsPlaying())
        {
            ActionAudioComponent->Stop();
        }

        bInCombat = false;
        bIsFading = false;
        bStoppingMusic = false;

        UE_LOG(LogTemp, Warning, TEXT("All music stopped immediately"));
    }
    else
    {
        // Fade out over time
        bStoppingMusic = true;
        bIsFading = false;
        StopFadeTime = FadeOutDuration;
        FadeTimer = 0.0f;

        UE_LOG(LogTemp, Warning, TEXT("Starting music fade out over %.1f seconds"), FadeOutDuration);
    }
}

void ACombatMusicManager::UpdateStopFade(float DeltaTime)
{
    FadeTimer += DeltaTime;
    float FadeAlpha = FMath::Clamp(FadeTimer / StopFadeTime, 0.0f, 1.0f);
    float Volume = (1.0f - FadeAlpha) * MusicVolume;

    // Fade both tracks to zero
    if (ChillAudioComponent && ChillAudioComponent->IsPlaying())
    {
        ChillAudioComponent->SetVolumeMultiplier(Volume);
    }
    if (ActionAudioComponent && ActionAudioComponent->IsPlaying())
    {
        ActionAudioComponent->SetVolumeMultiplier(Volume);
    }

    // Finish fade
    if (FadeAlpha >= 1.0f)
    {
        if (ChillAudioComponent)
        {
            ChillAudioComponent->Stop();
        }
        if (ActionAudioComponent)
        {
            ActionAudioComponent->Stop();
        }

        bStoppingMusic = false;
        bInCombat = false;

        UE_LOG(LogTemp, Warning, TEXT("Music fade out complete - all tracks stopped"));
    }
}

