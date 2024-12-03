// Fill out your copyright notice in the Description page of Project Settings.


#include "RangedEnemyAIController.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "RangedEnemyCharacter.h"

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

    // Keep distance from the player
    if (PlayerPawn)
    {
        FVector EnemyLocation = GetPawn()->GetActorLocation();
        FVector PlayerLocation = PlayerPawn->GetActorLocation();
        float DistanceToPlayer = FVector::Dist(EnemyLocation, PlayerLocation);

        if (DistanceToPlayer < MinimumDistanceFromPlayer)
        {
            // Move away from the player if too close
            FVector DirectionAwayFromPlayer = (EnemyLocation - PlayerLocation).GetSafeNormal();
            FVector NewLocation = EnemyLocation + DirectionAwayFromPlayer * MoveRadius;

            MoveToLocation(NewLocation);
        }
    }
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

        // Apply damage to the player when shot at
        float DamageAmount = 20.0f; // The amount of damage to deal to the player
        UGameplayStatics::ApplyDamage(
            PlayerPawn,                // The target actor (the player)
            DamageAmount,              // The damage amount
            GetPawn()->GetController(),// The instigator (the AI controller)
            GetPawn(),                 // The damage causer (the ranged enemy)
            UDamageType::StaticClass() // The damage type
        );

        // Optional: Log the damage for debugging purposes
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Ranged enemy shot player!"));
    }

    // Continue with repositioning or other logic
    Reposition();

    // Set timer to shoot again after cooldown
    GetWorld()->GetTimerManager().SetTimer(ShootingTimerHandle, this, &ARangedEnemyAIController::FireAtPlayer, ShootingCooldown, false);
}

void ARangedEnemyAIController::Reposition()
{
    if (ARangedEnemyCharacter* EnemyCharacter = Cast<ARangedEnemyCharacter>(GetPawn()))
    {
        // Get current location and player's location
        FVector CurrentLocation = EnemyCharacter->GetActorLocation();
        ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
        if (!PlayerCharacter) return;

        FVector PlayerLocation = PlayerCharacter->GetActorLocation();

        // Calculate a direction away from the player
        FVector AwayDirection = (CurrentLocation - PlayerLocation).GetSafeNormal();
        FVector NewLocation = CurrentLocation + AwayDirection * MoveRadius;

        // Move the AI character to the new location
        MoveToLocation(NewLocation);
        UE_LOG(LogTemp, Warning, TEXT("Enemy repositions"));
    }
}
