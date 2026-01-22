// Fill out your copyright notice in the Description page of Project Settings.


#include "SettingsWidget.h"
#include "Kismet/GameplayStatics.h"
#include "AudioSaveGame.h"
#include "GameFramework/SaveGame.h"

void USettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Bind slider events
    if (MasterVolumeSlider)
    {
        MasterVolumeSlider->OnValueChanged.AddDynamic(this, &USettingsWidget::OnMasterVolumeChanged);
    }

    if (MusicVolumeSlider)
    {
        MusicVolumeSlider->OnValueChanged.AddDynamic(this, &USettingsWidget::OnMusicVolumeChanged);
    }

    if (SFXVolumeSlider)
    {
        SFXVolumeSlider->OnValueChanged.AddDynamic(this, &USettingsWidget::OnSFXVolumeChanged);
    }

    // Load saved settings
    LoadAudioSettings();
}

void USettingsWidget::OnMasterVolumeChanged(float Value)
{
    MasterVolume = Value;
    UpdateVolumeText(MasterVolumeText, Value);
    SetSoundClassVolume(MasterSoundClass, Value);

    // Master affects both music and SFX
    SetSoundClassVolume(MusicSoundClass, MusicVolume * MasterVolume);
    SetSoundClassVolume(SFXSoundClass, SFXVolume * MasterVolume);

    // Auto-save when changed
    SaveAudioSettings();
}

void USettingsWidget::OnMusicVolumeChanged(float Value)
{
    MusicVolume = Value;
    UpdateVolumeText(MusicVolumeText, Value);
    SetSoundClassVolume(MusicSoundClass, Value * MasterVolume);

    // Auto-save when changed
    SaveAudioSettings();
}

void USettingsWidget::OnSFXVolumeChanged(float Value)
{
    SFXVolume = Value;
    UpdateVolumeText(SFXVolumeText, Value);
    SetSoundClassVolume(SFXSoundClass, Value * MasterVolume);

    // Auto-save when changed
    SaveAudioSettings();
}

void USettingsWidget::UpdateVolumeText(UTextBlock* TextBlock, float Value)
{
    if (TextBlock)
    {
        int32 VolumePercent = FMath::RoundToInt(Value * 100.0f);
        TextBlock->SetText(FText::AsNumber(VolumePercent));
    }
}

void USettingsWidget::SetSoundClassVolume(USoundClass* SoundClass, float Volume)
{
    if (SoundClass)
    {
        SoundClass->Properties.Volume = Volume;
    }
}

void USettingsWidget::SaveAudioSettings()
{
    UAudioSaveGame* SaveGameInstance = Cast<UAudioSaveGame>(UGameplayStatics::CreateSaveGameObject(UAudioSaveGame::StaticClass()));

    if (SaveGameInstance)
    {
        // Store current values
        SaveGameInstance->MasterVolume = MasterVolume;
        SaveGameInstance->MusicVolume = MusicVolume;
        SaveGameInstance->SFXVolume = SFXVolume;

        // Save to slot
        UGameplayStatics::SaveGameToSlot(SaveGameInstance, TEXT("AudioSettings"), 0);
    }
}

void USettingsWidget::LoadAudioSettings()
{
    UAudioSaveGame* LoadedGame = Cast<UAudioSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("AudioSettings"), 0));

    if (LoadedGame)
    {
        // Load saved values
        MasterVolume = LoadedGame->MasterVolume;
        MusicVolume = LoadedGame->MusicVolume;
        SFXVolume = LoadedGame->SFXVolume;
    }
    else
    {
        // Use defaults if no save exists
        MasterVolume = 1.0f;
        MusicVolume = 1.0f;
        SFXVolume = 1.0f;
    }

    // Set slider values (this won't trigger OnValueChanged)
    if (MasterVolumeSlider)
    {
        MasterVolumeSlider->SetValue(MasterVolume);
        UpdateVolumeText(MasterVolumeText, MasterVolume);
    }

    if (MusicVolumeSlider)
    {
        MusicVolumeSlider->SetValue(MusicVolume);
        UpdateVolumeText(MusicVolumeText, MusicVolume);
    }

    if (SFXVolumeSlider)
    {
        SFXVolumeSlider->SetValue(SFXVolume);
        UpdateVolumeText(SFXVolumeText, SFXVolume);
    }

    // Apply volumes to sound classes
    SetSoundClassVolume(MasterSoundClass, MasterVolume);
    SetSoundClassVolume(MusicSoundClass, MusicVolume * MasterVolume);
    SetSoundClassVolume(SFXSoundClass, SFXVolume * MasterVolume);
}