// Fill out your copyright notice in the Description page of Project Settings.


#include "RaphaelAIController.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "RaphaelBossCharacter.h"
#include "Components/CapsuleComponent.h"
#include "SpearOfHeavenProjectile.h"
#include "Components/SphereComponent.h"
#include "HomingProjectile.h"
#include "DrawDebugHelpers.h" // For debugging hit traces


ARaphaelAIController::ARaphaelAIController()
{
    // Set default values
    CurrentShotCount = 0;
    CurrentShotCount = 0;
    ProjectilesPerBurst = 5; // Number of shots per burst
    ShotInterval = 0.2f;     // Interval between each shot in the burst
    BurstInterval = 2.0f;    // Time between bursts

    // Initialize the Judgement Gaze Collider
    JudgementGazeCollider = CreateDefaultSubobject<UCapsuleComponent>(TEXT("JudgementGazeCollider"));
    JudgementGazeCollider->InitCapsuleSize(50.f, 200.f);  // Adjust as needed
    JudgementGazeCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    JudgementGazeCollider->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

    // Initialize the Judgement Gaze Spawn Point
    JudgementGazeSpawnPoint = CreateDefaultSubobject<USphereComponent>(TEXT("JudgementGazeSpawnPoint"));
    JudgementGazeSpawnPoint->InitSphereRadius(10.f);
    JudgementGazeSpawnPoint->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Attach the JudgementGazeCollider to the JudgementGazeSpawnPoint
    JudgementGazeCollider->SetupAttachment(JudgementGazeSpawnPoint);
}

void ARaphaelAIController::SummonBeamOnPlayer()
{
    // Get the player character
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (PlayerCharacter)
    {
        // Get the player's location
        FVector PlayerLocation = PlayerCharacter->GetActorLocation();

        // Spawn the beam collider at the player's location
        SpawnBeamColliderAtLocation(PlayerLocation);
    }

    // Set a timer to summon the next beam after the delay
    GetWorld()->GetTimerManager().SetTimer(DelayTimerHandle, this, &ARaphaelAIController::StartBeamSummonWithDelay, DelayBetweenBeams, false);
}

void ARaphaelAIController::BeginPlay()
{
    Super::BeginPlay();

    // Enable ticking for the AI controller
    PrimaryActorTick.bCanEverTick = true;

    // Start the first burst
    StartBurst();

    // Start the beam summon with delay
    StartBeamSummonWithDelay();

    // Set up a timer to trigger the throw ability every 10 seconds (for example)
    GetWorld()->GetTimerManager().SetTimer(ThrowChargeTimerHandle, this, &ARaphaelAIController::StartThrowAbility, 10.0f, true);

    StartJudgementGaze();

    StartHeavenRain();
    
}



void ARaphaelAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Get reference to the player character
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (!PlayerCharacter) return;

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn) return;

    // Get the current location of the boss and player
    FVector BossLocation = ControlledPawn->GetActorLocation();
    FVector PlayerLocation = PlayerCharacter->GetActorLocation();

    // Calculate the direction towards the player
    FVector DirectionToPlayer = (PlayerLocation - BossLocation).GetSafeNormal();

    // Calculate the rotation to face the player
    FRotator LookAtRotation = FRotationMatrix::MakeFromX(DirectionToPlayer).Rotator();

    // Set the rotation of the boss to face the player
    ControlledPawn->SetActorRotation(LookAtRotation);
}

void ARaphaelAIController::SpawnBeamColliderAtLocation(FVector Location)
{
    // Create a temporary capsule component for the beam (as the beam's collider)
    UCapsuleComponent* BeamCollider = NewObject<UCapsuleComponent>(this, UCapsuleComponent::StaticClass());
    if (BeamCollider)
    {
        // Initialize the size of the beam collider (capsule shape)
        BeamCollider->InitCapsuleSize(BeamRadius, BeamHeight);

        // Enable collision and set collision responses
        BeamCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        BeamCollider->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);

        // Set the world location for the beam (on top of the player)
        BeamCollider->SetWorldLocation(Location);

        // Register the component to make it active in the world
        BeamCollider->RegisterComponent();

        // Draw a debug cylinder to visualize the beam
        FVector BottomOfBeam = Location - FVector(0, 0, BeamHeight / 2); // Bottom of the beam
        FVector TopOfBeam = Location + FVector(0, 0, BeamHeight / 2);    // Top of the beam

        // Draw the debug cylinder to represent the beam
        DrawDebugCylinder(GetWorld(), BottomOfBeam, TopOfBeam, BeamRadius, 12, FColor::Red, false, BeamDuration, 0, 2);


        // Log for debugging
        UE_LOG(LogTemp, Warning, TEXT("Beam Collider Spawned at Player's Location"));

        // Set a timer to destroy the beam collider after the beam duration
        GetWorld()->GetTimerManager().SetTimer(BeamTimerHandle, [=]()
            {
                BeamCollider->DestroyComponent();
                UE_LOG(LogTemp, Warning, TEXT("Beam Collider Destroyed"));
            }, BeamDuration, false);
    }
}

void ARaphaelAIController::StartBeamSummonWithDelay()
{
    // Start summoning the beam on the player after the delay
    SummonBeamOnPlayer();
}

void ARaphaelAIController::ShootProjectile()
{
    if (ARaphaelBossCharacter* BossCharacter = Cast<ARaphaelBossCharacter>(GetPawn()))
    {
        // Get the reference to the player character
        ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
        if (!PlayerCharacter) return;

        // Get the boss and player's current locations
        FVector BossLocation = BossCharacter->GetActorLocation();
        FVector PlayerLocation = PlayerCharacter->GetActorLocation();

        // Calculate the direction from the boss to the player
        FVector DirectionToPlayer = (PlayerLocation - BossLocation).GetSafeNormal();

        // Pass the direction to the projectile spawning function
        BossCharacter->SpawnProjectile(DirectionToPlayer);

        // Increment the current shot count
        CurrentShotCount++;

        // If there are more shots to fire in the burst, set a timer to fire the next shot
        if (CurrentShotCount < ProjectilesPerBurst)
        {
            GetWorld()->GetTimerManager().SetTimer(ShotTimerHandle, this, &ARaphaelAIController::ShootProjectile, ShotInterval, false);
        }
    }
}

void ARaphaelAIController::StartBurst()
{
    // Reset shot count
    CurrentShotCount = 0;

    // Start shooting projectiles
    ShootProjectile();

    // Set timer for the next burst, considering the time to complete the current burst
    float NextBurstDelay = (ProjectilesPerBurst * ShotInterval) + BurstInterval;
    GetWorld()->GetTimerManager().SetTimer(BurstTimerHandle, this, &ARaphaelAIController::StartBurst, NextBurstDelay, false);
}

void ARaphaelAIController::ResetBurst()
{
    // Clear the timers
    GetWorld()->GetTimerManager().ClearTimer(ShotTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(BurstTimerHandle);

    // Reset the shot count
    CurrentShotCount = 0;

    // Start the burst again
    StartBurst();
}

void ARaphaelAIController::StartThrowAbility()
{
    // Start the 2-second charge before executing the throw
    GetWorld()->GetTimerManager().SetTimer(ThrowChargeTimerHandle, this, &ARaphaelAIController::ExecuteThrow, 2.0f, false);

    // Log or play charge-up animation/sound during this time if desired
    UE_LOG(LogTemp, Warning, TEXT("Boss is charging the throw..."));
}

void ARaphaelAIController::ExecuteThrow()
{
    // Get the boss character and the player character
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    ARaphaelBossCharacter* BossCharacter = Cast<ARaphaelBossCharacter>(GetPawn());

    if (!PlayerCharacter || !BossCharacter) return;

    // Get boss location and player location for the trace
    FVector BossLocation = BossCharacter->GetActorLocation();
    FVector PlayerLocation = PlayerCharacter->GetActorLocation();

    // Calculate the direction for the line trace (from boss to player)
    FVector TraceDirection = (PlayerLocation - BossLocation).GetSafeNormal();

    // Define the start and end points for the trace
    FVector TraceStart = BossLocation + FVector(0, 0, 50.0f); // Start slightly above the boss
    FVector TraceEnd = TraceStart + (TraceDirection * 5000.0f); // Trace for a long distance

    // Setup for line trace
    FHitResult HitResult;
    FCollisionQueryParams TraceParams;
    TraceParams.AddIgnoredActor(BossCharacter); // Ignore the boss in the trace

    // Perform the hitscan (line trace)
    bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

    if (bHit)
    {
        // If we hit something, spawn an explosion at the hit location
        SpawnExplosionAtLocation(HitResult.Location);

        // Optionally, draw a debug line to visualize the hitscan
        DrawDebugLine(GetWorld(), TraceStart, HitResult.Location, FColor::Red, false, 2.0f, 0, 5.0f);

       // UE_LOG(LogTemp, Warning, TEXT("Throw hit: %s"), *HitResult.Actor->GetName());
    }
    else
    {
        // If no hit, trace to the max distance
        DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Green, false, 2.0f, 0, 5.0f);
        UE_LOG(LogTemp, Warning, TEXT("Throw missed, no hit."));
    }
}

void ARaphaelAIController::SpawnExplosionAtLocation(FVector Location)
{
    // Here we spawn an explosion effect (you can add your own explosion particle system or logic)
    UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, Location);

    // Optionally, apply damage or effects to nearby actors
    TArray<AActor*> IgnoredActors;
    //UGameplayStatics::ApplyRadialDamage(GetWorld(), ExplosionDamage, Location, ExplosionRadius, DamageTypeClass, IgnoredActors, this, GetOwner()->GetInstigatorController(), true);

    UE_LOG(LogTemp, Warning, TEXT("Explosion spawned at location: %s"), *Location.ToString());
}

void ARaphaelAIController::StartJudgementGaze()
{
    // Clear any existing player position history
    PlayerPositionHistory.Empty();

    // Begin tracking the player's position with a delay
    GetWorld()->GetTimerManager().SetTimer(JudgementGazeTimerHandle, this, &ARaphaelAIController::PerformJudgementGaze, TraceDelay, true);

    // Stop JudgementGaze after the specified duration
    GetWorld()->GetTimerManager().SetTimer(JudgementGazeDurationTimerHandle, this, &ARaphaelAIController::StopJudgementGaze, JudgementGazeDuration, false);

    UE_LOG(LogTemp, Warning, TEXT("JudgementGaze started"));
}

void ARaphaelAIController::StopJudgementGaze()
{
    // Clear timers and reset the JudgementGaze ability
    GetWorld()->GetTimerManager().ClearTimer(JudgementGazeTimerHandle);
    PlayerPositionHistory.Empty();

    UE_LOG(LogTemp, Warning, TEXT("JudgementGaze ended"));
}

void ARaphaelAIController::PerformJudgementGaze()
{
    // Get player reference
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (!PlayerCharacter) return;

    // Store the player's current position each tick
    FVector PlayerLocation = PlayerCharacter->GetActorLocation();
    PlayerPositionHistory.Add(PlayerLocation);

    // Ensure there's at least one previous position to trail behind
    if (PlayerPositionHistory.Num() > 1)
    {
        // Get the trailing position (older position in history)
        FVector TargetLocation = PlayerPositionHistory[0];

        // Remove oldest position to maintain a trailing effect
        PlayerPositionHistory.RemoveAt(0);

        // Boss's current location
        APawn* ControlledPawn = GetPawn();
        if (!ControlledPawn) return;

        FVector BossLocation = ControlledPawn->GetActorLocation();

        // Line trace from the boss's location toward the trailing player position
        FHitResult HitResult;
        GetWorld()->LineTraceSingleByChannel(HitResult, BossLocation, TargetLocation, ECC_Visibility);

        // Visualize the trace for debugging
        DrawDebugLine(GetWorld(), BossLocation, TargetLocation, FColor::Red, false, 0.1f, 0, 2.0f);

        // Handle logic if player is hit
        if (HitResult.GetActor() == PlayerCharacter)
        {
            UE_LOG(LogTemp, Warning, TEXT("JudgementGaze hit the player!"));
            // Add damage or effects here as needed
        }
    }
}

void ARaphaelAIController::SpawnHeavenRainTrace()
{
    // Get the controlled pawn’s location (center of the trace area)
    if (APawn* BossPawn = GetPawn())
    {
        FVector BossLocation = BossPawn->GetActorLocation();

        // Generate a random point within the specified radius
        float RandomRadius = FMath::FRandRange(0.0f, HeavenRainRadius);
        float RandomAngle = FMath::FRandRange(0.0f, 360.0f);

        FVector Offset = FVector(RandomRadius * FMath::Cos(FMath::DegreesToRadians(RandomAngle)),
            RandomRadius * FMath::Sin(FMath::DegreesToRadians(RandomAngle)),
            0.0f);

        FVector TraceStart = BossLocation + Offset;
        FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, 10000.0f); // Trace downward

        // Perform the line trace
        FHitResult Hit;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(BossPawn);

        bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

        // Draw debug line (optional)
        DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Blue, false, 2.0f, 0, 2.0f);

        // Apply effects if the trace hits something
        if (bHit)
        {
            // Handle what happens on hit here
            // e.g., apply damage, spawn effects, etc.
            //UE_LOG(LogTemp, Warning, TEXT("Heaven's Rain hit %s"), *Hit.Actor->GetName());
        }

        // Increment the trace count
        CurrentRainTraceCount++;

        // Stop the ability if the required number of traces is reached
        if (CurrentRainTraceCount >= HeavenRainTraceCount)
        {
            EndHeavenRain();
        }
    }
}

void ARaphaelAIController::StartHeavenRain()
{
    CurrentRainTraceCount = 0;

    // Start the timer to trigger each trace with an interval
    GetWorld()->GetTimerManager().SetTimer(HeavenRainTimerHandle, this, &ARaphaelAIController::SpawnHeavenRainTrace, HeavenRainInterval, true);

    
}

void ARaphaelAIController::EndHeavenRain()
{

    // Clear the timer to stop further traces
    GetWorld()->GetTimerManager().ClearTimer(HeavenRainTimerHandle);

    UE_LOG(LogTemp, Warning, TEXT("Heaven's Rain ability ended."));
}






