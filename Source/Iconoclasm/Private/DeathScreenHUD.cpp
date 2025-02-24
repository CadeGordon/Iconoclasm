// Fill out your copyright notice in the Description page of Project Settings.


#include "DeathScreenHUD.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "IconoclasmCharacter.h"


void UDeathScreenHUD::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind functions to buttons
	if (RestartButton)
	{
		RestartButton->OnClicked.AddDynamic(this, &UDeathScreenHUD::RestartLevel);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UDeathScreenHUD::QuitGame);
	}
}

void UDeathScreenHUD::RestartLevel()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		// Unpause the game before restarting
		UGameplayStatics::SetGamePaused(GetWorld(), false);

		// Reset input mode to game only
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}

	// Restart the level
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()), false);
}

void UDeathScreenHUD::QuitGame()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController)
	{
		PlayerController->ConsoleCommand("quit");
	}
}
