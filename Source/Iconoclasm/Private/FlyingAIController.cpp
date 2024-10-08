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
    // Initialize variables
    PlayerChaseDistance = 1500.0f;  // Distance to start chasing the player
    StopChasingDistance = 1000.0f;   // Distance to stop chasing and switch to random flying
    FlySpeed = 600.0f;              // Speed of flying
    ChangeDirectionInterval = 1.5f; // How often to change direction when flying randomly
    TimeSinceLastDirectionChange = 0.0f;
    bIsChasingPlayer = false;
}

void AFlyingAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    TimeSinceLastDirectionChange += DeltaTime;
    TimeSinceLastShot += DeltaTime;

    if (TimeSinceLastDirectionChange >= ChangeDirectionInterval)
    {
        ChangeFlyDirection();
        TimeSinceLastDirectionChange = 0.0f;
    }

    if (PlayerPawn)
    {
        // Calculate distance to the player
        float DistanceToPlayer = FVector::Dist(PlayerPawn->GetActorLocation(), GetPawn()->GetActorLocation());

        if (DistanceToPlayer <= ShootRange && TimeSinceLastShot >= TimeBetweenShots)
        {
            ShootProjectile(); // Fire projectile if within range and cooldown has passed
            TimeSinceLastShot = 0.0f;
        }

        if (DistanceToPlayer > StopChasingDistance)
        {
            MoveToPlayer();  // Move towards the player if out of range
        }
        else
        {
            FlyAround();  // Fly around if close to the player
        }
    }
    else
    {
        FlyAround(); // Default flying behavior
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

void AFlyingAIController::MoveToPlayer()
{
    if (PlayerPawn && GetPawn())
    {
        FVector DirectionToPlayer = (PlayerPawn->GetActorLocation() - GetPawn()->GetActorLocation()).GetSafeNormal();
        FVector NewLocation = GetPawn()->GetActorLocation() + (DirectionToPlayer * FlySpeed * GetWorld()->DeltaTimeSeconds);
        GetPawn()->SetActorLocation(NewLocation);
    }
}

void AFlyingAIController::AttackPlayer()
{
    // Implement logic for attacking the player
    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Player hit by flying enemy"));
}

void AFlyingAIController::ShootProjectile()
{
    AFlyingEnemyCharacter* FlyingEnemy = Cast<AFlyingEnemyCharacter>(GetPawn());
    if (FlyingEnemy)
    {
        FlyingEnemy->ShootAtPlayer(PlayerPawn);  // Call the character to handle shooting
    }
}


