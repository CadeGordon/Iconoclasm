// Fill out your copyright notice in the Description page of Project Settings.


#include "DeathScreenHUD.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "IconoclasmCharacter.h"
#include "EnemySpawner.h"


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
	AIconoclasmCharacter* PlayerCharacter = Cast<AIconoclasmCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

	if (PlayerCharacter && PlayerController)
	{
		//  Ensure the DeathScreenHUD is removed before respawning
		if (PlayerCharacter->DeathScreenHUD)
		{
			PlayerCharacter->DeathScreenHUD->RemoveFromParent();
			PlayerCharacter->DeathScreenHUD = nullptr;  // Destroy it
		}

		// Respawn logic...
		if (PlayerCharacter->IsCheckpointActive())
		{
			// Move to checkpoint
			PlayerCharacter->SetActorLocation(PlayerCharacter->GetCheckpointLocation());

			// Restore full health
			UHealthComponent* HealthComp = PlayerCharacter->GetHealthComponent();
			if (HealthComp)
			{
				HealthComp->SetCurrentHealth(HealthComp->GetMaxHealth());
			}

			// Clear enemies, reactivate spawners, and unpause game
			TArray<AActor*> Enemies;
			UGameplayStatics::GetAllActorsWithTag(this, FName("Enemy"), Enemies);
			for (AActor* Enemy : Enemies)
			{
				Enemy->Destroy();
			}

			TArray<AActor*> EnemySpawners;
			UGameplayStatics::GetAllActorsOfClass(this, AEnemySpawner::StaticClass(), EnemySpawners);
			for (AActor* Spawner : EnemySpawners)
			{
				AEnemySpawner* EnemySpawner = Cast<AEnemySpawner>(Spawner);
				if (EnemySpawner)
				{
					EnemySpawner->ResetSpawner();
				}
			}

			// Restore game input
			UGameplayStatics::SetGamePaused(GetWorld(), false);
			PlayerController->SetInputMode(FInputModeGameOnly());
			PlayerController->bShowMouseCursor = false;
		}
		else
		{
			// Restart level
			UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()), false);
			PlayerController->SetInputMode(FInputModeGameOnly());
			PlayerController->bShowMouseCursor = false;
		}
	}
}

void UDeathScreenHUD::QuitGame()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController)
	{
		PlayerController->ConsoleCommand("quit");
	}
}
