// Fill out your copyright notice in the Description page of Project Settings.


#include "GruntEnemyCharacter.h"
#include "GruntAIController.h"

// Sets default values
AGruntEnemyCharacter::AGruntEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = AGruntAIController::StaticClass();
	// Ensure the AI controller is assigned when the character is spawned
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // Set default health value
    Health = 100.0f;

    

}

float AGruntEnemyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // Call parent implementation (optional)
    Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    // Subtract the damage from health
    Health -= DamageAmount;

    // Log damage for debugging
    UE_LOG(LogTemp, Warning, TEXT("Grunt took %f damage. Remaining health: %f"), DamageAmount, Health);

    // Check if health is depleted
    if (Health <= 0.0f)
    {
        Die();
    }

    return DamageAmount;
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

}

// Called to bind functionality to input
void AGruntEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AGruntEnemyCharacter::Die()
{
    // Log death for debugging
    UE_LOG(LogTemp, Warning, TEXT("GruntEnemyCharacter has died."));

    // Handle death (e.g., play animation, destroy actor, etc.)
    Destroy();
}

