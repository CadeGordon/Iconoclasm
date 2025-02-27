// Fill out your copyright notice in the Description page of Project Settings.


#include "Checkpoint.h"
#include "Components/BoxComponent.h"
#include "IconoclasmCharacter.h"
#include "Kismet/GameplayStatics.h"

 

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
	}
}
