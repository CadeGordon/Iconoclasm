// Fill out your copyright notice in the Description page of Project Settings.


#include "FlyingAIController.h"
#include "FlyingEnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Math/UnrealMathUtility.h"

AFlyingAIController::AFlyingAIController()
{
	
}

void AFlyingAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    // Update the timer for changing directions
    TimeSinceLastDirectionChange += DeltaTime;

    // Change direction every few seconds
    if (TimeSinceLastDirectionChange >= ChangeDirectionInterval)
    {
        ChangeFlyDirection();
        TimeSinceLastDirectionChange = 0.0f;
    }

    // Fly around
    FlyAround();

    // Optionally, attack the player if nearby
    if (PlayerPawn && FVector::Dist(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation()) <= 500.0f)
    {
        AttackPlayer();
    }
}

void AFlyingAIController::BeginPlay()
{
	Super::BeginPlay();

    // Find the player pawn
    PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

    // Start flying in a random direction
    ChangeFlyDirection();
    TimeSinceLastDirectionChange = 0.0f;

	
}

void AFlyingAIController::FlyAround()
{
    if (GetPawn())
    {
        FVector NewLocation = GetPawn()->GetActorLocation() + (RandomFlyDirection * FlySpeed * GetWorld()->DeltaTimeSeconds);
        GetPawn()->SetActorLocation(NewLocation);
    }
}

void AFlyingAIController::ChangeFlyDirection()
{
    // Generate a random direction
    RandomFlyDirection = FMath::VRand();
    RandomFlyDirection.Z = FMath::FRandRange(0.3f, 1.0f); // Favor flying upward a bit more
    RandomFlyDirection.Normalize();

    UE_LOG(LogTemp, Warning, TEXT("Changing fly direction to: %s"), *RandomFlyDirection.ToString());
}

void AFlyingAIController::AttackPlayer()
{
    // Implement logic for attacking the player
    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Player hit by flying enemy"));
}


