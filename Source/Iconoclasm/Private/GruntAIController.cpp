// Fill out your copyright notice in the Description page of Project Settings.


#include "GruntAIController.h"
#include "Kismet/GameplayStatics.h"
#include "GruntEnemyCharacter.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

void AGruntAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Ensure the controller has a pawn
    if (GetPawn() && PlayerPawn)
    {
        // Always move the AI towards the player
        MoveToPlayer();
    }
}

void AGruntAIController::BeginPlay()
{
    Super::BeginPlay();

    // Get the player pawn
    PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

    // Start moving to the player if possessing a pawn
    if (GetPawn())
    {
        MoveToPlayer();
    }
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
    if (PlayerPawn)
    {
        // Amount of damage to deal
        float DamageAmount = 20.0f;

        // Deal damage to the player
        UGameplayStatics::ApplyDamage(
            PlayerPawn,                // Target actor (the player)
            DamageAmount,              // Damage amount
            GetPawn()->GetController(),// Instigator (the AI controller)
            GetPawn(),                 // Damage causer (the grunt enemy)
            UDamageType::StaticClass() // Damage type
        );

        // Debug message to indicate the player was hit
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Player hit - Damage dealt!"));
    }
}



