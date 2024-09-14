// Fill out your copyright notice in the Description page of Project Settings.


#include "GruntEnemyCharacter.h"
#include "GruntAIController.h"

// Sets default values
AGruntEnemyCharacter::AGruntEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = AGruntAIController::StaticClass();

}

// Called when the game starts or when spawned
void AGruntEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGruntEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AGruntEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

