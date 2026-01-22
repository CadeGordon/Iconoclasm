// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Sound/SoundClass.h"
#include "SettingsWidget.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API USettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    // Master volume slider
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    USlider* MasterVolumeSlider;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* MasterVolumeText;

    // Music volume slider
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    USlider* MusicVolumeSlider;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* MusicVolumeText;

    // Sound FX volume slider
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    USlider* SFXVolumeSlider;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* SFXVolumeText;

    // Sound Class references - Set these in Blueprint!
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Settings")
    USoundClass* MasterSoundClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Settings")
    USoundClass* MusicSoundClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Settings")
    USoundClass* SFXSoundClass;

protected:
    virtual void NativeConstruct() override;

    // Slider change callbacks
    UFUNCTION()
    void OnMasterVolumeChanged(float Value);

    UFUNCTION()
    void OnMusicVolumeChanged(float Value);

    UFUNCTION()
    void OnSFXVolumeChanged(float Value);

    // Helper function to update volume text
    void UpdateVolumeText(UTextBlock* TextBlock, float Value);

    // Helper function to set sound class volume
    void SetSoundClassVolume(USoundClass* SoundClass, float Volume);

    // Save/Load settings
    UFUNCTION(BlueprintCallable, Category = "Audio Settings")
    void SaveAudioSettings();

    UFUNCTION(BlueprintCallable, Category = "Audio Settings")
    void LoadAudioSettings();

private:
    // Volume values (0.0 to 1.0)
    float MasterVolume;
    float MusicVolume;
    float SFXVolume;
	
};
