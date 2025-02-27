// Fill out your copyright notice in the Description page of Project Settings.


#include "RangedEnemyCharacter.h"
#include "RangedEnemyAIController.h"
#include "IconoclasmProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

// Sets default values
ARangedEnemyCharacter::ARangedEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set AI Controller Class to use our custom AI controller
	AIControllerClass = ARangedEnemyAIController::StaticClass();

	// Setup the sphere component for projectile spawning
	ProjectileSpawnSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ProjectileSpawnSphere"));
	ProjectileSpawnSphere->SetupAttachment(RootComponent);
	ProjectileSpawnSphere->SetSphereRadius(10.0f);  // Adjust size as needed

	// Adjust character movement settings, so they can move when repositioning
	GetCharacterMovement()->MaxWalkSpeed = 300.0f;  // Custom speed for AI movement

	// Ensure the AI controller is assigned when the character is spawned
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Set the "Enemy" tag
	Tags.Add(FName("Enemy"));

}

// Called when the game starts or when spawned
void ARangedEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARangedEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ARangedEnemyCharacter::FireWeapon(const FVector& Direction)
{
	// Spawn the projectile, set its direction, and any other firing logic
	// For example:
	if (ProjectileClass)
	{
		FVector SpawnLocation = GetActorLocation() + Direction * 100.0f; // Adjust the spawn distance if needed
		FRotator SpawnRotation = Direction.Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();

		AIconoclasmProjectile* Projectile = GetWorld()->SpawnActor<AIconoclasmProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
		if (Projectile)
		{
			Projectile->FireInDirection(Direction);
		}
	}
}



