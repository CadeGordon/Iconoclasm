// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AudioSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API UAudioSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
    UAudioSaveGame();

    UPROPERTY(VisibleAnywhere, Category = "Audio")
    float MasterVolume;

    UPROPERTY(VisibleAnywhere, Category = "Audio")
    float MusicVolume;

    UPROPERTY(VisibleAnywhere, Category = "Audio")
    float SFXVolume;


	
};
