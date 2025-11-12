// Fill out your copyright notice in the Description page of Project Settings.


#include "GruntAIController.h"
#include "Kismet/GameplayStatics.h"
#include "GruntEnemyCharacter.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

AGruntAIController::AGruntAIController()
{
    bCanAttack = true; // Enemy can attack initially
    AttackCooldown = 2.0f; // Default attack cooldown duration (in seconds)

}

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
    if (bCanAttack && PlayerPawn)
    {
        // Get the damage amount from the enemy pawn
        AGruntEnemyCharacter* GruntPawn = Cast<AGruntEnemyCharacter>(GetPawn());

        float DamageAmount = 20.0f; // Default fallback

        if (GruntPawn)
        {
            DamageAmount = GruntPawn->DamageAmount;
        }

        // Deal damage to the player
        UGameplayStatics::ApplyDamage(
            PlayerPawn,
            DamageAmount,
            GetPawn()->GetController(),
            GetPawn(),
            UDamageType::StaticClass()
        );

        // Debug message
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red,
            FString::Printf(TEXT("Player hit - %f Damage dealt!"), DamageAmount));

        // Start the cooldown
        bCanAttack = false;
        GetWorld()->GetTimerManager().SetTimer(AttackCooldownTimerHandle,
            this, &AGruntAIController::ResetAttack, AttackCooldown);
    }
}

void AGruntAIController::ResetAttack()
{
    bCanAttack = true;
}



