// Fill out your copyright notice in the Description page of Project Settings.


#include "GruntAIController.h"
#include "Kismet/GameplayStatics.h"
#include "GruntEnemyCharacter.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

void AGruntAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!IsPlayerInRange)
    {
        Wander();              // Wander around randomly
        CheckPlayerProximity(); // Check if the player is close
    }
    else
    {
        MoveToPlayer(); // Move toward the player if in range
    }

    // Draw debug sphere for the detection radius
    if (PlayerPawn)
    {
        FVector Center = GetPawn()->GetActorLocation();
        float DetectionRadius = 500.0f; // Same as in CheckPlayerProximity
        DrawDebugSphere(GetWorld(), Center, DetectionRadius, 12, FColor::Green, false, -1, 0, 2.0f);
    }
}

void AGruntAIController::BeginPlay()
{
    Super::BeginPlay();

    // Get a reference to the player
    PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    IsPlayerInRange = false;

    // Initialize the wander location
    WanderLocation = FVector::ZeroVector;
}

void AGruntAIController::MoveToPlayer()
{
    if (PlayerPawn)
    {
        // Move the AI towards the player
        MoveToActor(PlayerPawn);

        // Check if the AI is close enough to attack the player
        float AttackRange = 150.0f;
        if (FVector::Dist(PlayerPawn->GetActorLocation(), GetPawn()->GetActorLocation()) <= AttackRange)
        {
            AttackPlayer();
        }
    }
}

void AGruntAIController::AttackPlayer()
{
    // Debug message to indicate the player was hit
    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Player hit"));
}

void AGruntAIController::Wander()
{
    // If the AI has reached its wander location, generate a new one
    if (FVector::Dist(GetPawn()->GetActorLocation(), WanderLocation) < 100.0f)
    {
        float WanderRadius = 1000.0f;
        FVector RandomPoint = FMath::VRand() * WanderRadius;
        WanderLocation = GetPawn()->GetActorLocation() + RandomPoint;

        // Debugging: Print the new wander location
        UE_LOG(LogTemp, Warning, TEXT("New Wander Location: %s"), *WanderLocation.ToString());

        // Move to the new random location
        MoveToLocation(WanderLocation);
    }
}

void AGruntAIController::CheckPlayerProximity()
{
    if (PlayerPawn)
    {
        float DistanceToPlayer = FVector::Dist(PlayerPawn->GetActorLocation(), GetPawn()->GetActorLocation());
        float DetectionRadius = 500.0f;

        // Check if the player is within detection range
        if (DistanceToPlayer <= DetectionRadius)
        {
            IsPlayerInRange = true;
        }
    }
}
