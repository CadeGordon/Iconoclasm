// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeathScreenHUD.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API UDeathScreenHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Initializes the widget */
	virtual void NativeConstruct() override;

	/** Function to restart the level */
	UFUNCTION()
	void RestartLevel();

	/** Function to quit the game */
	UFUNCTION()
	void QuitGame();

protected:
	/** Button to restart the level */
	UPROPERTY(meta = (BindWidget))
	class UButton* RestartButton;

	/** Button to quit the game */
	UPROPERTY(meta = (BindWidget))
	class UButton* QuitButton;
	
};
