// Fill out your copyright notice in the Description page of Project Settings.


#include "MusicStopTrigger.h"
#include "Components/BoxComponent.h"
#include "CombatMusicManager.h"
#include "IconoclasmCharacter.h"

AMusicStopTrigger::AMusicStopTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create trigger box
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;

	// Set up collision
	TriggerBox->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Bind overlap event
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AMusicStopTrigger::OnTriggerEnter);
}

void AMusicStopTrigger::BeginPlay()
{
	Super::BeginPlay();
}

void AMusicStopTrigger::OnTriggerEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Check if it's the player
	if (!OtherActor || !OtherActor->IsA(AIconoclasmCharacter::StaticClass()))
	{
		return;
	}

	// Check if already triggered
	if (bOneTimeUse && bHasBeenTriggered)
	{
		return;
	}

	bHasBeenTriggered = true;
	UE_LOG(LogTemp, Warning, TEXT("Music stop trigger activated by player"));

	// Get the music manager
	ACombatMusicManager* MusicManager = ACombatMusicManager::GetInstance(GetWorld());
	if (MusicManager)
	{
		MusicManager->StopAllMusic(bStopImmediately, FadeOutTime);
		UE_LOG(LogTemp, Warning, TEXT("Stopping all music via trigger"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Music Manager not found!"));
	}

	// Disable collision if one-time use
	if (bOneTimeUse)
	{
		TriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

