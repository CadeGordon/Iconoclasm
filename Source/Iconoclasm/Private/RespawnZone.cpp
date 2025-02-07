// Fill out your copyright notice in the Description page of Project Settings.


#include "RespawnZone.h"
#include "Components/BoxComponent.h"
#include "IconoclasmCharacter.h"
#include "GameFramework/Actor.h"
#include "HealthComponent.h"

// Sets default values
ARespawnZone::ARespawnZone()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create a box collider
	RespawnTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("RespawnTrigger"));
	RootComponent = RespawnTrigger;
	RespawnTrigger->SetBoxExtent(FVector(100.0f, 100.0f, 50.0f)); // Default size, can be edited in the editor

	// Enable overlap event
	RespawnTrigger->OnComponentBeginOverlap.AddDynamic(this, &ARespawnZone::OnOverlapBegin);

}

// Called when the game starts or when spawned
void ARespawnZone::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARespawnZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ARespawnZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AIconoclasmCharacter* PlayerCharacter = Cast<AIconoclasmCharacter>(OtherActor);
	if (PlayerCharacter)
	{
		// Log respawn event
		UE_LOG(LogTemp, Warning, TEXT("Player entered Respawn Zone! Respawning..."));

		// Move player to last known safe location
		if (PlayerCharacter->LastSafeLocation != FVector::ZeroVector)
		{
			PlayerCharacter->SetActorLocation(PlayerCharacter->LastSafeLocation);
		}

		// Apply damage using HealthComponent
		UHealthComponent* HealthComp = PlayerCharacter->FindComponentByClass<UHealthComponent>();
		if (HealthComp)
		{
			HealthComp->TakeDamage(DamageOnRespawn);
		}
	}
}

