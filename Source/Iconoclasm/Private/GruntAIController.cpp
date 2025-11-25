// Fill out your copyright notice in the Description page of Project Settings.


#include "GruntAIController.h"
#include "Kismet/GameplayStatics.h"
#include "GruntEnemyCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "NavigationSystem.h"

AGruntAIController::AGruntAIController()
{
    bCanAttack = true;
    AttackCooldown = 2.0f;

    bCanJump = true;
    JumpCooldown = 3.0f; // Cooldown between jumps
}

void AGruntAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (GetPawn() && PlayerPawn)
    {
        MoveToPlayer();
        TryJumpToPlayer();
    }
}

void AGruntAIController::BeginPlay()
{
    Super::BeginPlay();

    PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

    if (GetPawn())
    {
        MoveToPlayer();
    }
}

void AGruntAIController::MoveToPlayer()
{
    if (PlayerPawn)
    {
        FVector TargetLocation = PlayerPawn->GetActorLocation();
        TargetLocation.Z = GetPawn()->GetActorLocation().Z;

        MoveToLocation(TargetLocation, 5.0f);

        float AttackRange = 150.0f;
        if (FVector::Dist(PlayerPawn->GetActorLocation(), GetPawn()->GetActorLocation()) <= AttackRange)
        {
            AttackPlayer();
        }
    }
}

void AGruntAIController::TryJumpToPlayer()
{
    if (!bCanJump || !PlayerPawn) return;

    AGruntEnemyCharacter* GruntCharacter = Cast<AGruntEnemyCharacter>(GetPawn());
    ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerPawn);

    if (!GruntCharacter || !PlayerCharacter) return;

    // Check if player is in the air
    UCharacterMovementComponent* PlayerMovement = PlayerCharacter->GetCharacterMovement();
    if (!PlayerMovement || !PlayerMovement->IsFalling()) return;

    // Check if grunt is on the ground
    UCharacterMovementComponent* GruntMovement = GruntCharacter->GetCharacterMovement();
    if (!GruntMovement || GruntMovement->IsFalling()) return;

    // Check if the character can jump using our custom function
    if (!GruntCharacter->CanAIJump()) return;

    FVector GruntLocation = GruntCharacter->GetActorLocation();
    FVector PlayerLocation = PlayerCharacter->GetActorLocation();

    // Calculate horizontal distance
    FVector HorizontalDiff = PlayerLocation - GruntLocation;
    HorizontalDiff.Z = 0.0f;
    float HorizontalDistance = HorizontalDiff.Size();

    // Calculate vertical distance
    float VerticalDistance = PlayerLocation.Z - GruntLocation.Z;

    // Only jump if player is within reasonable range and above us
    float MaxJumpRange = 600.0f; // Horizontal range
    float MinVerticalDiff = 100.0f; // Player must be at least this high above us

    if (HorizontalDistance <= MaxJumpRange && VerticalDistance >= MinVerticalDiff)
    {
        float LaunchSpeed;
        FVector LaunchVelocity;

        if (CalculateJumpVelocity(GruntLocation, PlayerLocation, LaunchSpeed, LaunchVelocity))
        {
            // Make the character jump first to leave the ground
            GruntCharacter->Jump();

            // Then apply the calculated velocity on the next frame
            FTimerHandle LaunchTimerHandle;
            GetWorld()->GetTimerManager().SetTimer(LaunchTimerHandle, [GruntMovement, LaunchVelocity]()
                {
                    if (GruntMovement)
                    {
                        GruntMovement->Velocity = LaunchVelocity;
                    }
                }, 0.05f, false); // Small delay to ensure jump has started

            // Debug visualization
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green,
                FString::Printf(TEXT("Grunt jumping! Speed: %f"), LaunchSpeed));

            // Start cooldown
            bCanJump = false;
            GetWorld()->GetTimerManager().SetTimer(JumpCooldownTimerHandle,
                this, &AGruntAIController::ResetJump, JumpCooldown);
        }
    }
}

bool AGruntAIController::CalculateJumpVelocity(const FVector& StartLocation, const FVector& TargetLocation,
    float& OutLaunchSpeed, FVector& OutLaunchVelocity)
{
    ACharacter* GruntCharacter = Cast<ACharacter>(GetPawn());
    if (!GruntCharacter) return false;

    UCharacterMovementComponent* Movement = GruntCharacter->GetCharacterMovement();
    if (!Movement) return false;

    // Get gravity
    float Gravity = FMath::Abs(Movement->GetGravityZ());

    // Calculate differences
    FVector Difference = TargetLocation - StartLocation;
    FVector HorizontalDiff = Difference;
    HorizontalDiff.Z = 0.0f;
    float HorizontalDistance = HorizontalDiff.Size();
    float VerticalDistance = Difference.Z;

    if (HorizontalDistance < 1.0f) return false;

    // Use projectile motion equations
    // We want to reach the target, so we calculate the required velocity

    // Time to reach target (we'll use 45-degree angle as optimal for max range)
    // But we need to adjust for the vertical distance

    // Calculate launch angle (aim slightly above target to account for arc)
    float OptimalAngle = 45.0f; // degrees
    float AngleRad = FMath::DegreesToRadians(OptimalAngle);

    // Calculate required initial velocity
    // Using: Range = (v^2 * sin(2*angle)) / g
    // Solving for v: v = sqrt((Range * g) / sin(2*angle))

    float LaunchSpeed = FMath::Sqrt((HorizontalDistance * Gravity) / FMath::Sin(2.0f * AngleRad));

    // Adjust for vertical difference
    // Add extra speed if target is higher
    if (VerticalDistance > 0.0f)
    {
        float ExtraSpeed = FMath::Sqrt(2.0f * Gravity * VerticalDistance);
        LaunchSpeed += ExtraSpeed * 0.5f; // Add half the extra needed
    }

    // Clamp the launch speed to reasonable values
    LaunchSpeed = FMath::Clamp(LaunchSpeed, 300.0f, 1500.0f);

    // Calculate velocity direction
    FVector HorizontalDirection = HorizontalDiff.GetSafeNormal();

    // Calculate vertical component
    float VerticalComponent = LaunchSpeed * FMath::Sin(AngleRad);
    float HorizontalComponent = LaunchSpeed * FMath::Cos(AngleRad);

    // Construct final velocity
    OutLaunchVelocity = HorizontalDirection * HorizontalComponent;
    OutLaunchVelocity.Z = VerticalComponent;

    OutLaunchSpeed = LaunchSpeed;

    return true;
}

void AGruntAIController::ResetJump()
{
    bCanJump = true;
}

void AGruntAIController::AttackPlayer()
{
    if (bCanAttack && PlayerPawn)
    {
        AGruntEnemyCharacter* GruntPawn = Cast<AGruntEnemyCharacter>(GetPawn());
        float DamageAmount = 20.0f;

        if (GruntPawn)
        {
            DamageAmount = GruntPawn->DamageAmount;
        }

        UGameplayStatics::ApplyDamage(
            PlayerPawn,
            DamageAmount,
            GetPawn()->GetController(),
            GetPawn(),
            UDamageType::StaticClass()
        );

        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red,
            FString::Printf(TEXT("Player hit - %f Damage dealt!"), DamageAmount));

        bCanAttack = false;
        GetWorld()->GetTimerManager().SetTimer(AttackCooldownTimerHandle,
            this, &AGruntAIController::ResetAttack, AttackCooldown);
    }
}

void AGruntAIController::ResetAttack()
{
    bCanAttack = true;
}


