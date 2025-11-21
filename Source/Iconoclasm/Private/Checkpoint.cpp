// Fill out your copyright notice in the Description page of Project Settings.


#include "Checkpoint.h"
#include "Components/BoxComponent.h"
#include "IconoclasmCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "RangedEnemyCharacter.h"
#include "FlyingEnemyCharacter.h"
#include "GruntEnemyCharacter.h"
#include "RaphaelBossCharacter.h"
#include "CombatMusicManager.h"

ACheckpoint::ACheckpoint()
{
	PrimaryActorTick.bCanEverTick = false;
	CheckpointTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("CheckpointTrigger"));
	RootComponent = CheckpointTrigger;
	CheckpointTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CheckpointTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	CheckpointTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	CheckpointTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CheckpointTrigger->OnComponentBeginOverlap.AddDynamic(this, &ACheckpoint::OnCheckpointOverlap);
}

void ACheckpoint::BeginPlay()
{
	Super::BeginPlay();
}

void ACheckpoint::OnCheckpointOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AIconoclasmCharacter* PlayerCharacter = Cast<AIconoclasmCharacter>(OtherActor);
	if (PlayerCharacter)
	{
		PlayerCharacter->SetCheckpointLocation(GetActorLocation());
		DestroyEnemyCharacters();
	}
}

void ACheckpoint::DestroyEnemyCharacters()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 TotalEnemiesDestroyed = 0;

	// Destroy RangedEnemyCharacters (except Raphael boss)
	TArray<AActor*> RangedEnemies;
	UGameplayStatics::GetAllActorsOfClass(World, ARangedEnemyCharacter::StaticClass(), RangedEnemies);
	for (AActor* Enemy : RangedEnemies)
	{
		ARaphaelBossCharacter* RaphaelBoss = Cast<ARaphaelBossCharacter>(Enemy);
		if (!RaphaelBoss)
		{
			Enemy->Destroy();
			TotalEnemiesDestroyed++;
		}
	}

	// Destroy FlyEnemyCharacters (except Raphael boss)
	TArray<AActor*> FlyEnemies;
	UGameplayStatics::GetAllActorsOfClass(World, AFlyingEnemyCharacter::StaticClass(), FlyEnemies);
	for (AActor* Enemy : FlyEnemies)
	{
		ARaphaelBossCharacter* RaphaelBoss = Cast<ARaphaelBossCharacter>(Enemy);
		if (!RaphaelBoss)
		{
			Enemy->Destroy();
			TotalEnemiesDestroyed++;
		}
	}

	// Destroy GruntEnemyCharacters (except Raphael boss)
	TArray<AActor*> GruntEnemies;
	UGameplayStatics::GetAllActorsOfClass(World, AGruntEnemyCharacter::StaticClass(), GruntEnemies);
	for (AActor* Enemy : GruntEnemies)
	{
		ARaphaelBossCharacter* RaphaelBoss = Cast<ARaphaelBossCharacter>(Enemy);
		if (!RaphaelBoss)
		{
			Enemy->Destroy();
			TotalEnemiesDestroyed++;
		}
	}

	// Notify the music manager that enemies were destroyed
	if (TotalEnemiesDestroyed > 0)
	{
		ACombatMusicManager* MusicManager = ACombatMusicManager::GetInstance(World);
		if (MusicManager)
		{
			// Notify for each enemy destroyed
			for (int32 i = 0; i < TotalEnemiesDestroyed; i++)
			{
				MusicManager->OnEnemyKilled();
			}
			UE_LOG(LogTemp, Warning, TEXT("Checkpoint destroyed %d enemies and notified music manager"), TotalEnemiesDestroyed);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Music Manager not found when clearing checkpoint enemies!"));
		}
	}
}
