// Fill out your copyright notice in the Description page of Project Settings.


#include "RevolverUnlockWidget.h"
#include "Revolver_WeaponComponent.h"
#include "IconoclasmCharacter.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"

URevolverUnlockWidget::URevolverUnlockWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RevolverComponent = nullptr;
}

void URevolverUnlockWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Cache the revolver component
	CacheRevolverComponent();

	// Bind the button click event
	if (UnlockButton)
	{
		UnlockButton->OnClicked.AddDynamic(this, &URevolverUnlockWidget::OnUnlockButtonClicked);
	}

	// Set default text if we have a text widget
	if (UnlockText)
	{
		UnlockText->SetText(FText::FromString("Unlock Hellfire Mode"));
	}

	// Check if already unlocked (in case widget is created after unlock)
	if (RevolverComponent && RevolverComponent->IsHellfireModeUnlocked())
	{
		RemoveFromParent();
	}
}

void URevolverUnlockWidget::NativeDestruct()
{
	// Unbind the button click event
	if (UnlockButton)
	{
		UnlockButton->OnClicked.RemoveDynamic(this, &URevolverUnlockWidget::OnUnlockButtonClicked);
	}

	Super::NativeDestruct();
}

void URevolverUnlockWidget::CacheRevolverComponent()
{
	// Get the player character
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("HellfireUnlockWidget: No owning player controller!"));
		return;
	}

	APawn* PlayerPawn = PC->GetPawn();
	if (!PlayerPawn)
	{
		UE_LOG(LogTemp, Error, TEXT("HellfireUnlockWidget: No player pawn!"));
		return;
	}

	// Cast to your character class
	AIconoclasmCharacter* PlayerCharacter = Cast<AIconoclasmCharacter>(PlayerPawn);
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("HellfireUnlockWidget: Failed to cast to IconoclasmCharacter!"));
		return;
	}

	// Method 1: Try to find the component directly on the character
	RevolverComponent = PlayerCharacter->FindComponentByClass<URevolver_WeaponComponent>();

	if (!RevolverComponent)
	{
		// Method 2: Search all attached actors for the revolver component
		TArray<AActor*> AttachedActors;
		PlayerCharacter->GetAttachedActors(AttachedActors);

		for (AActor* AttachedActor : AttachedActors)
		{
			if (AttachedActor)
			{
				RevolverComponent = AttachedActor->FindComponentByClass<URevolver_WeaponComponent>();
				if (RevolverComponent)
				{
					UE_LOG(LogTemp, Warning, TEXT("HellfireUnlockWidget: Found Revolver_WeaponComponent on attached actor: %s"), *AttachedActor->GetName());
					break;
				}
			}
		}
	}

	if (!RevolverComponent)
	{
		// Method 3: Search all components of all types in the world (last resort)
		TArray<URevolver_WeaponComponent*> FoundComponents;

		// Get all actors in the world
		TArray<AActor*> AllActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), AllActors);

		for (AActor* Actor : AllActors)
		{
			URevolver_WeaponComponent* FoundComponent = Actor->FindComponentByClass<URevolver_WeaponComponent>();
			if (FoundComponent)
			{
				// Check if this component's Character matches our player
				if (FoundComponent->Character == PlayerCharacter)
				{
					RevolverComponent = FoundComponent;
					UE_LOG(LogTemp, Warning, TEXT("HellfireUnlockWidget: Found Revolver_WeaponComponent via world search on: %s"), *Actor->GetName());
					break;
				}
			}
		}
	}

	if (!RevolverComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("HellfireUnlockWidget: No Revolver_WeaponComponent found anywhere!"));
	}
}

void URevolverUnlockWidget::OnUnlockButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Unlock button clicked!"));

	// Make sure we have a valid revolver component
	if (!RevolverComponent)
	{
		CacheRevolverComponent(); // Try to find it again
	}

	if (RevolverComponent)
	{
		// Unlock the Hellfire mode
		RevolverComponent->UnlockHellfireMode();

		// Remove this widget from the screen
		//RemoveFromParent();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("HellfireUnlockWidget: Cannot unlock - no valid Revolver Component!"));
	}
}

