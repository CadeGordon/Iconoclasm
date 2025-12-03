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
    PreferredDistance = 1200.0f;
    MinDistance = 800.0f;
    FlySpeed = 900.0f;
    ChangeDirectionInterval = 1.0f;  // Increased from 0.4f for smoother movement
    TimeSinceLastDirectionChange = 0.0f;
    bIsChasingPlayer = false;

    // Shooting parameters
    ShootRange = 1500.0f;
    TimeBetweenShots = 2.0f;
    TimeSinceLastShot = 0.0f;

    // Evasive maneuver parameters
    EvasiveManeuverChance = 0.2f;  // Reduced from 0.3f
    CurrentEvasiveDirection = FVector::ZeroVector;
    EvasiveManeuverDuration = 0.5f;  // Increased from 0.3f for smoother transitions
    EvasiveManeuverTimer = 0.0f;
    bIsEvading = false;
    StrafeSpeed = 700.0f;

    // Smoothing parameters
    CurrentVelocity = FVector::ZeroVector;
    AccelerationRate = 3.0f;  // How fast to change velocity
    TargetDirection = FVector::ZeroVector;
}

void AFlyingAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    APawn* ControlledPawn = GetPawn();
    if (!IsValid(ControlledPawn))
    {
        return;
    }

    TimeSinceLastDirectionChange += DeltaTime;
    TimeSinceLastShot += DeltaTime;

    // Handle evasive maneuver timing
    if (bIsEvading)
    {
        EvasiveManeuverTimer += DeltaTime;
        if (EvasiveManeuverTimer >= EvasiveManeuverDuration)
        {
            bIsEvading = false;
            EvasiveManeuverTimer = 0.0f;
        }
    }

    // Less frequent direction changes for smoother movement
    if (TimeSinceLastDirectionChange >= ChangeDirectionInterval)
    {
        ChangeFlyDirection();
        TimeSinceLastDirectionChange = 0.0f;

        // Random chance to trigger evasive maneuver
        if (FMath::FRand() < EvasiveManeuverChance && !bIsEvading)
        {
            TriggerEvasiveManeuver();
        }
    }

    if (IsValid(PlayerPawn))
    {
        float DistanceToPlayer = FVector::Dist(PlayerPawn->GetActorLocation(), ControlledPawn->GetActorLocation());

        // Shoot at player if within range and cooldown has passed
        if (DistanceToPlayer <= ShootRange && TimeSinceLastShot >= TimeBetweenShots)
        {
            ShootProjectile();
            TimeSinceLastShot = 0.0f;

            // Trigger evasive maneuver after shooting
            if (FMath::FRand() < 0.4f)  // Reduced from 0.6f
            {
                TriggerEvasiveManeuver();
            }
        }

        // Keep away from player behavior
        if (DistanceToPlayer < MinDistance)
        {
            FleeFromPlayer(DeltaTime);
        }
        else if (DistanceToPlayer > PreferredDistance)
        {
            MaintainDistance(DeltaTime);
        }
        else
        {
            FlyAround(DeltaTime);
        }
    }
    else
    {
        FlyAround(DeltaTime);
    }

    // Apply smoothed movement
    ApplySmoothMovement(DeltaTime);
}

void AFlyingAIController::BeginPlay()
{
    Super::BeginPlay();

    PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    ChangeFlyDirection();
    TimeSinceLastDirectionChange = 0.0f;
    CurrentVelocity = FVector::ZeroVector;
}

void AFlyingAIController::TriggerEvasiveManeuver()
{
    bIsEvading = true;
    EvasiveManeuverTimer = 0.0f;

    APawn* ControlledPawn = GetPawn();
    if (!IsValid(ControlledPawn))
    {
        return;
    }

    // Generate a random perpendicular direction for dodging
    if (IsValid(PlayerPawn))
    {
        FVector ToPlayer = (PlayerPawn->GetActorLocation() - ControlledPawn->GetActorLocation()).GetSafeNormal();
        FVector RightVector = FVector::CrossProduct(ToPlayer, FVector::UpVector).GetSafeNormal();

        // Randomly dodge left or right, with some vertical component
        float HorizontalDirection = FMath::RandBool() ? 1.0f : -1.0f;
        float VerticalBias = FMath::FRandRange(-0.2f, 0.4f);

        CurrentEvasiveDirection = (RightVector * HorizontalDirection) + (FVector::UpVector * VerticalBias);
        CurrentEvasiveDirection.Normalize();
    }
    else
    {
        CurrentEvasiveDirection = FMath::VRand();
        CurrentEvasiveDirection.Z = FMath::FRandRange(-0.2f, 0.5f);
        CurrentEvasiveDirection.Normalize();
    }
}

void AFlyingAIController::FlyAround(float DeltaTime)
{
    APawn* ControlledPawn = GetPawn();
    if (!IsValid(ControlledPawn))
    {
        return;
    }

    FVector AvoidanceDirection = AvoidNearbyEnemies();

    // Combine random flight with evasive maneuvers
    if (bIsEvading)
    {
        TargetDirection = (RandomFlyDirection * 0.3f) + (CurrentEvasiveDirection * 0.7f) + AvoidanceDirection;
    }
    else
    {
        TargetDirection = RandomFlyDirection + AvoidanceDirection;
    }

    TargetDirection.Normalize();
}

void AFlyingAIController::ChangeFlyDirection()
{
    APawn* ControlledPawn = GetPawn();
    float CurrentAltitude = IsValid(ControlledPawn) ? ControlledPawn->GetActorLocation().Z : 500.0f;
    float MinAltitude = 500.0f;

    RandomFlyDirection = FMath::VRand();

    if (CurrentAltitude < MinAltitude + 200.0f)
    {
        RandomFlyDirection.Z = FMath::FRandRange(0.4f, 0.8f);
    }
    else
    {
        RandomFlyDirection.Z = FMath::FRandRange(-0.2f, 0.6f);
    }

    RandomFlyDirection.Normalize();
}

void AFlyingAIController::FleeFromPlayer(float DeltaTime)
{
    APawn* ControlledPawn = GetPawn();
    if (!IsValid(PlayerPawn) || !IsValid(ControlledPawn))
    {
        return;
    }

    // Move directly away from player
    FVector AwayFromPlayer = (ControlledPawn->GetActorLocation() - PlayerPawn->GetActorLocation()).GetSafeNormal();
    FVector AvoidanceDirection = AvoidNearbyEnemies();

    // Add strafing while fleeing for erratic movement
    FVector RightVector = FVector::CrossProduct(AwayFromPlayer, FVector::UpVector).GetSafeNormal();
    float StrafeOffset = FMath::Sin(GetWorld()->GetTimeSeconds() * 2.5f) * 0.25f;  // Slower sine wave

    if (bIsEvading)
    {
        TargetDirection = (AwayFromPlayer * 0.6f) + (CurrentEvasiveDirection * 0.3f) + (RightVector * StrafeOffset * 0.1f) + AvoidanceDirection;
    }
    else
    {
        TargetDirection = (AwayFromPlayer * 0.7f) + (RightVector * StrafeOffset) + (AvoidanceDirection * 0.3f);
    }

    TargetDirection.Normalize();
}

void AFlyingAIController::MaintainDistance(float DeltaTime)
{
    APawn* ControlledPawn = GetPawn();
    if (!IsValid(PlayerPawn) || !IsValid(ControlledPawn))
    {
        return;
    }

    // Move slightly toward player but mostly strafe around them
    FVector ToPlayer = (PlayerPawn->GetActorLocation() - ControlledPawn->GetActorLocation()).GetSafeNormal();
    FVector AvoidanceDirection = AvoidNearbyEnemies();

    // Circle around the player
    FVector RightVector = FVector::CrossProduct(ToPlayer, FVector::UpVector).GetSafeNormal();

    // Use a more consistent circle direction based on time
    float CircleSpeed = 1.5f;
    float CircleDirection = FMath::Sin(GetWorld()->GetTimeSeconds() * CircleSpeed);

    if (bIsEvading)
    {
        TargetDirection = (ToPlayer * 0.2f) + (CurrentEvasiveDirection * 0.5f) + (RightVector * CircleDirection * 0.3f) + AvoidanceDirection;
    }
    else
    {
        TargetDirection = (ToPlayer * 0.25f) + (RightVector * CircleDirection * 0.5f) + (RandomFlyDirection * 0.25f) + AvoidanceDirection;
    }

    TargetDirection.Normalize();
}

void AFlyingAIController::ApplySmoothMovement(float DeltaTime)
{
    APawn* ControlledPawn = GetPawn();
    if (!IsValid(ControlledPawn))
    {
        return;
    }

    // Smoothly interpolate current velocity toward target direction
    FVector TargetVelocity = TargetDirection * FlySpeed;

    // Apply speed boost during evasive maneuvers
    if (bIsEvading)
    {
        TargetVelocity *= 1.3f;  // Reduced from 1.5f
    }

    // Lerp toward target velocity for smooth acceleration/deceleration
    CurrentVelocity = FMath::VInterpTo(CurrentVelocity, TargetVelocity, DeltaTime, AccelerationRate);

    // Apply movement
    FVector NewLocation = ControlledPawn->GetActorLocation() + (CurrentVelocity * DeltaTime);

    // Smooth altitude correction
    float MinAltitude = 500.0f;
    if (NewLocation.Z < MinAltitude)
    {
        float AltitudeDeficit = MinAltitude - NewLocation.Z;
        float UpwardCorrection = FMath::Min(AltitudeDeficit * 0.3f, FlySpeed * DeltaTime);
        NewLocation.Z += UpwardCorrection;
    }

    ControlledPawn->SetActorLocation(NewLocation);
}

void AFlyingAIController::AttackPlayer()
{
    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Player hit by flying enemy"));
}

void AFlyingAIController::ShootProjectile()
{
    APawn* ControlledPawn = GetPawn();
    if (!IsValid(ControlledPawn))
    {
        return;
    }

    if (!IsValid(PlayerPawn))
    {
        return;
    }

    AFlyingEnemyCharacter* FlyingEnemy = Cast<AFlyingEnemyCharacter>(ControlledPawn);
    if (IsValid(FlyingEnemy))
    {
        FlyingEnemy->ShootAtPlayer(PlayerPawn);
    }
}

FVector AFlyingAIController::AvoidNearbyEnemies()
{
    FVector Avoidance = FVector::ZeroVector;

    APawn* ControlledPawn = GetPawn();
    if (!IsValid(ControlledPawn))
    {
        return Avoidance;
    }

    float AvoidanceRadius = 700.0f;

    TArray<AActor*> AllEnemies;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFlyingEnemyCharacter::StaticClass(), AllEnemies);

    for (AActor* OtherEnemy : AllEnemies)
    {
        if (!IsValid(OtherEnemy) || OtherEnemy == ControlledPawn)
        {
            continue;
        }

        float DistanceToEnemy = FVector::Dist(ControlledPawn->GetActorLocation(), OtherEnemy->GetActorLocation());

        if (DistanceToEnemy < AvoidanceRadius)
        {
            FVector AwayFromEnemy = ControlledPawn->GetActorLocation() - OtherEnemy->GetActorLocation();
            AwayFromEnemy.Normalize();
            Avoidance += AwayFromEnemy / DistanceToEnemy;
        }
    }

    if (!Avoidance.IsNearlyZero())
    {
        Avoidance.Normalize();
    }

    return Avoidance * 2.0f;  // Reduced from 2.5f
}

