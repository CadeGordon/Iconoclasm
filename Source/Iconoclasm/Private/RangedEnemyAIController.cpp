// Fill out your copyright notice in the Description page of Project Settings.


#include "RangedEnemyAIController.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "RangedEnemyCharacter.h"

ARangedEnemyAIController::ARangedEnemyAIController()
{
    ShotsFired = 0; // Initialize shot counter
    MaxShotsBeforeReposition = 3; // Set shots before repositioning


}

void ARangedEnemyAIController::BeginPlay()
{
    Super::BeginPlay();

    // Get reference to the player pawn
    PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

    // Start the behavior loop
    FireAtPlayer();
}

void ARangedEnemyAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

   
}

void ARangedEnemyAIController::FireAtPlayer()
{
    ARangedEnemyCharacter* RangedEnemy = Cast<ARangedEnemyCharacter>(GetPawn());
    if (RangedEnemy && PlayerPawn)
    {
        // Get the player location and add an offset for the height to aim at the center
        FVector PlayerLocation = PlayerPawn->GetActorLocation();
        float PlayerHeightOffset = PlayerPawn->GetDefaultHalfHeight(); // Use half the height of the player character
        FVector AimTarget = PlayerLocation + FVector(0, 0, PlayerHeightOffset);

        // Get the direction to the target
        FVector FireDirection = (AimTarget - RangedEnemy->GetActorLocation()).GetSafeNormal();

        // Make the enemy shoot in the calculated direction
        RangedEnemy->FireWeapon(FireDirection);

        // Optional: Log the shot for debugging purposes
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Ranged enemy shot player!"));
    }

    // Increment the shot counter
    ShotsFired++;

    // Check if it's time to reposition
    if (ShotsFired >= MaxShotsBeforeReposition)
    {
        ShotsFired = 0; // Reset shot counter
        Reposition();
    }

    // Set timer to shoot again after cooldown
    GetWorld()->GetTimerManager().SetTimer(ShootingTimerHandle, this, &ARangedEnemyAIController::FireAtPlayer, ShootingCooldown, false);
}

void ARangedEnemyAIController::Reposition()
{
    if (ARangedEnemyCharacter* EnemyCharacter = Cast<ARangedEnemyCharacter>(GetPawn()))
    {
        // Get current location
        FVector CurrentLocation = EnemyCharacter->GetActorLocation();

        // Generate a random direction
        FVector RandomDirection = FMath::VRand(); // Random unit vector
        RandomDirection.Z = 0; // Keep movement on the horizontal plane
        RandomDirection.Normalize();

        // Choose a random distance
        float RandomDistance = FMath::RandRange(MinRepositionDistance, MaxRepositionDistance);

        // Calculate the new location
        FVector NewLocation = CurrentLocation + RandomDirection * RandomDistance;

        // Move the AI character to the new location
        MoveToLocation(NewLocation);
        UE_LOG(LogTemp, Warning, TEXT("Enemy repositions to %s"), *NewLocation.ToString());
    }
}
