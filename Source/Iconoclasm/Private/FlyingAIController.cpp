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
    StopChasingDistance = 1300.0f;   // Distance to stop chasing and switch to random flying
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
        FVector NewLocation = GetPawn()->GetActorLocation();
        FVector AvoidanceDirection = AvoidNearbyEnemies(); // Get avoidance vector from nearby enemies
        FVector MovementDirection = RandomFlyDirection + AvoidanceDirection; // Add the avoidance to the flying direction

        MovementDirection.Normalize();
        NewLocation += (MovementDirection * FlySpeed * GetWorld()->DeltaTimeSeconds);

        // Set a minimum Z value (altitude) to prevent the enemy from flying too close to the ground
        float MinAltitude = 500.0f;  // Adjust this value as necessary

        // Ensure the new location's Z value is above the minimum altitude
        if (NewLocation.Z < MinAltitude)
        {
            NewLocation.Z = MinAltitude;  // Keep the enemy at least this high
        }

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

FVector AFlyingAIController::AvoidNearbyEnemies()
{
    FVector Avoidance = FVector::ZeroVector;

    // Define the radius to check for nearby enemies
    float AvoidanceRadius = 300.0f;

    // Find all pawns (enemies) in the world
    TArray<AActor*> AllEnemies;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFlyingEnemyCharacter::StaticClass(), AllEnemies);

    for (AActor* OtherEnemy : AllEnemies)
    {
        // Skip if the other enemy is this one
        if (OtherEnemy == GetPawn()) continue;

        // Calculate the distance to the other enemy
        float DistanceToEnemy = FVector::Dist(GetPawn()->GetActorLocation(), OtherEnemy->GetActorLocation());

        if (DistanceToEnemy < AvoidanceRadius)
        {
            // Calculate a direction away from the other enemy
            FVector AwayFromEnemy = GetPawn()->GetActorLocation() - OtherEnemy->GetActorLocation();
            AwayFromEnemy.Normalize();

            // Scale the avoidance based on the proximity (closer enemies generate stronger avoidance)
            Avoidance += AwayFromEnemy / DistanceToEnemy;
        }
    }

    // Normalize and return the avoidance vector
    if (!Avoidance.IsNearlyZero())
    {
        Avoidance.Normalize();
    }

    return Avoidance * 0.5f; // You can adjust the multiplier to control the strength of avoidance
}


