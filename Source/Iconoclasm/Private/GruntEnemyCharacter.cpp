// Fill out your copyright notice in the Description page of Project Settings.


#include "GruntEnemyCharacter.h"
#include "GruntAIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

// Sets default values
AGruntEnemyCharacter::AGruntEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = AGruntAIController::StaticClass();
	// Ensure the AI controller is assigned when the character is spawned
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Set the "Enemy" tag
	Tags.Add(FName("Enemy"));

    

}


//// Called when the game starts or when spawned
//void AGruntEnemyCharacter::BeginPlay()
//{
//	Super::BeginPlay();
//	
//}

// Called every frame
void AGruntEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Get the player character
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!PlayerCharacter) return;

	// Get direction to player
	FVector DirectionToPlayer = PlayerCharacter->GetActorLocation() - GetActorLocation();
	DirectionToPlayer.Z = 0.0f; // Ignore vertical difference

	if (!DirectionToPlayer.IsNearlyZero())
	{
		FRotator TargetRotation = DirectionToPlayer.Rotation();
		FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 5.0f); // 5.0f = rotation speed
		SetActorRotation(NewRotation);
	}

}

// Called to bind functionality to input
void AGruntEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}



