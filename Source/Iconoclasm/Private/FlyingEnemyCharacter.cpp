// Fill out your copyright notice in the Description page of Project Settings.


#include "FlyingEnemyCharacter.h"
#include "FlyingAIController.h"
#include "GameFramework/CharacterMovementComponent.h"  // Include this line
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/Controller.h"

// Sets default values
AFlyingEnemyCharacter::AFlyingEnemyCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Initialize movement component for flying
	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));

	// Set default AIController class, if not set in Blueprint
	AIControllerClass = AFlyingAIController::StaticClass();

	// Ensure the AI controller is assigned when the character is spawned
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

// Called when the game starts or when spawned
void AFlyingEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Initialize flying settings for the character
	InitializeFlyingSettings();
}

// Called every frame
void AFlyingEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AFlyingEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AFlyingEnemyCharacter::InitializeFlyingSettings()
{
	if (MovementComponent)
    {
        // Set flying movement specifics like speed and acceleration
        MovementComponent->MaxSpeed = 600.0f; // Flying speed
        MovementComponent->Acceleration = 200.0f;
    }

    // Optionally, you could also configure the pawn's collision and gravity behavior
    SetCanBeDamaged(true);

    // Ensure gravity isn't affecting the flying enemy (this is important for flying characters)
    GetMesh()->SetEnableGravity(false);
}



