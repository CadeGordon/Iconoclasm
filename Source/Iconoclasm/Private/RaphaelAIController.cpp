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

    // Initialize ActivationTrigger
    ActivationTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("ActivationTrigger"));
    ActivationTrigger->InitSphereRadius(1000.0f); // Adjust radius as needed
    ActivationTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ActivationTrigger->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
    ActivationTrigger->SetupAttachment(GetRootComponent());

    
}

void ARaphaelAIController::SummonBeamOnPlayer()
{
    // Get the player's location
    AActor* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (Player)
    {
        FVector PlayerLocation = Player->GetActorLocation();

        // Start with a telegraph before the actual beam spawns
        SpawnBeamTelegraph(PlayerLocation);
    }

    // Set a timer to summon the next beam after the delay
    GetWorld()->GetTimerManager().SetTimer(DelayTimerHandle, this, &ARaphaelAIController::StartBeamSummonWithDelay, DelayBetweenBeams, false);
}

void ARaphaelAIController::BeginPlay()
{
    Super::BeginPlay();

    // Enable ticking for the AI controller
    PrimaryActorTick.bCanEverTick = true;

    if (ActivationTrigger)
    {
        ActivationTrigger->OnComponentBeginOverlap.AddDynamic(this, &ARaphaelAIController::OnPlayerEnterTrigger);
    }
    
}



void ARaphaelAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bIsActivated)
        return; // Skip behavior if not activated

    // Continue with existing behavior
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (!PlayerCharacter) return;

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn) return;

    // Lock rotation to face the player
    FVector BossLocation = ControlledPawn->GetActorLocation();
    FVector PlayerLocation = PlayerCharacter->GetActorLocation();
    PlayerLocation.Z = BossLocation.Z;

    FVector DirectionToPlayer = (PlayerLocation - BossLocation).GetSafeNormal();
    FRotator LookAtRotation = FRotationMatrix::MakeFromX(DirectionToPlayer).Rotator();
    ControlledPawn->SetActorRotation(FRotator(0.0f, LookAtRotation.Yaw, 0.0f));

    if (!IsAbilityActive)
    {
        int32 RandomIndex = FMath::RandRange(0, 6);
        EAbilityType SelectedAbility = static_cast<EAbilityType>(RandomIndex);
        PerformAbility(SelectedAbility);
    }

    // Optionally draw a debug sphere for visualization
    if (ActivationTrigger)
    {
        FVector SphereLocation = ActivationTrigger->GetComponentLocation();
        float SphereRadius = ActivationTrigger->GetScaledSphereRadius();

        DrawDebugSphere(GetWorld(), SphereLocation, SphereRadius, 32, FColor::Blue, false, -1.0f, 0, 2.0f);
    }

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

        // Bind overlap event for the beam collider
        BeamCollider->OnComponentBeginOverlap.AddDynamic(this, &ARaphaelAIController::OnBeamOverlap);

        // FIXED: Use a weak pointer and proper cleanup in the lambda
        TWeakObjectPtr<UCapsuleComponent> WeakBeamCollider = BeamCollider;
        TWeakObjectPtr<ARaphaelAIController> WeakThis = this;

        // Set a timer to destroy the beam collider after the beam duration
        GetWorld()->GetTimerManager().SetTimer(BeamTimerHandle, [WeakThis, WeakBeamCollider]()
            {
                // Check if both objects are still valid before accessing them
                if (WeakThis.IsValid() && WeakBeamCollider.IsValid())
                {
                    // Unbind the overlap event before destroying to prevent dangling delegates
                    WeakBeamCollider->OnComponentBeginOverlap.RemoveDynamic(WeakThis.Get(), &ARaphaelAIController::OnBeamOverlap);
                    WeakBeamCollider->DestroyComponent();
                    UE_LOG(LogTemp, Warning, TEXT("Beam Collider Destroyed"));
                }
            }, BeamDuration, false);
    }
}

UFUNCTION()
void ARaphaelAIController::OnBeamOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // Check if the overlapped actor is the player
    if (OtherActor && OtherActor != this && OtherActor == UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
    {
        // Apply damage to the player when the beam overlaps
        float DamageAmount = 0.0f; // Set the damage amount (adjust as needed)
        UGameplayStatics::ApplyDamage(
            OtherActor,                // The target actor (the player)
            DamageAmount,              // The damage amount
            GetPawn()->GetController(),// The instigator (the AI controller)
            GetPawn(),                 // The damage causer (the boss)
            UDamageType::StaticClass() // The damage type
        );

        // Optional: Log the damage for debugging purposes
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Player hit by beam!"));
    }
}

void ARaphaelAIController::StartBeamSummonWithDelay()
{
    // Start summoning the beam on the player after the delay
    SummonBeamOnPlayer();
}

void ARaphaelAIController::SpawnBeamTelegraph(FVector location)
{
    // Create a telegraph indicator (visual aid) at the player's location
    DrawDebugCylinder(
        GetWorld(),
        location - FVector(0, 0, BeamHeight / 2), // Bottom of telegraph
        location + FVector(0, 0, BeamHeight / 2), // Top of telegraph
        BeamRadius,
        12,
        FColor::Yellow,
        false,
        TelegraphDuration,
        0,
        2.0f // Thickness of the telegraph
    );

    // Set a timer to spawn the actual beam collider after the telegraph duration
    GetWorld()->GetTimerManager().SetTimer(
        TelegraphTimerHandle,
        [this, location]()
        {
            SpawnBeamColliderAtLocation(location);
        },
        TelegraphDuration,
        false
    );
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

    // Get the boss location
    FVector BossLocation = BossCharacter->GetActorLocation();

    // Get player location and adjust the target to be near the ground
    FVector PlayerLocation = PlayerCharacter->GetActorLocation();

    // Offset the target location to hit the ground near the player
    PlayerLocation.Z -= 100.0f; // Target below the player's feet, adjust this value as needed

    // Calculate the direction for the line trace (from boss to the adjusted target)
    FVector TraceDirection = (PlayerLocation - BossLocation).GetSafeNormal();

    // Define the start and end points for the trace
    FVector TraceStart = BossLocation + FVector(0, 0, 50.0f); // Start slightly above the boss
    FVector TraceEnd = PlayerLocation; // End at the adjusted player location

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
        // If no hit, trace to the adjusted ground near the player
        SpawnExplosionAtLocation(TraceEnd);

        // Visualize the trace path
        DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Green, false, 2.0f, 0, 5.0f);
        UE_LOG(LogTemp, Warning, TEXT("Throw missed, exploding at target ground."));
    }
}

void ARaphaelAIController::SpawnExplosionAtLocation(FVector Location)
{
    // Spawn the explosion particle effect
    UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, Location);

    // Apply radial knockback to nearby actors
    TArray<AActor*> OverlappingActors;
    float KnockbackForce = 1500.0f; // Adjust force as needed
    float LaunchHeight = 1000.0f;  // Adjust height as needed

    // Define the radius for the knockback and damage
    float KnockbackRadius = 500.0f;
    float DamageAmount = 0.0f; // Set the damage amount (adjust as needed)

    // Draw a debug sphere to visualize the explosion radius
    DrawDebugSphere(
        GetWorld(),
        Location,           // Center of the sphere
        KnockbackRadius,    // Radius of the sphere
        24,                 // Number of segments
        FColor::Orange,     // Sphere color
        false,              // Persistent (false means it will disappear after some time)
        5.0f,               // Lifetime (5 seconds)
        0,                  // Depth priority
        2.0f                // Thickness of the lines
    );

    // Collect actors within the explosion radius
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), OverlappingActors);

    for (AActor* Actor : OverlappingActors)
    {
        if (Actor)
        {
            FVector DistanceVector = Actor->GetActorLocation() - Location;

            // Only affect actors within the radius
            if (DistanceVector.Size() <= KnockbackRadius)
            {
                // Apply radial impulse for physics-enabled actors
                if (UPrimitiveComponent* PrimitiveComponent = Actor->FindComponentByClass<UPrimitiveComponent>())
                {
                    PrimitiveComponent->AddRadialImpulse(
                        Location,
                        KnockbackRadius,
                        KnockbackForce,
                        ERadialImpulseFalloff::RIF_Linear,
                        true
                    );
                }

                // Launch the player character upward
                if (ACharacter* OverlappingCharacter = Cast<ACharacter>(Actor))
                {
                    FVector LaunchVelocity = FVector(0.0f, 0.0f, LaunchHeight);
                    OverlappingCharacter->LaunchCharacter(LaunchVelocity, true, true);

                    // Apply damage to the character caught in the explosion radius
                    UGameplayStatics::ApplyDamage(
                        Actor,               // The target actor
                        DamageAmount,        // The amount of damage
                        GetPawn()->GetController(), // The instigator (the AI controller)
                        GetPawn(),           // The damage causer (the boss)
                        UDamageType::StaticClass() // The damage type
                    );
                }
            }
        }
    }

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

        // Radius for the sphere trace
        float TraceRadius = 50.0f; // Adjust the radius as needed

        // Sweep from the boss's location toward the trailing player position using a sphere
        FHitResult HitResult;
        FCollisionQueryParams TraceParams;
        TraceParams.AddIgnoredActor(ControlledPawn); // Ignore the boss itself in the trace

        // Perform the sphere trace
        bool bHit = GetWorld()->SweepSingleByChannel(
            HitResult,
            BossLocation,
            TargetLocation,
            FQuat::Identity, // No rotation
            ECC_Pawn, // Use ECC_Pawn to detect characters and pawns
            FCollisionShape::MakeSphere(TraceRadius),
            TraceParams
        );

        // Debug output for the trace result
        if (bHit)
        {
            UE_LOG(LogTemp, Warning, TEXT("Hit actor: %s"), *HitResult.GetActor()->GetName());

            // Handle logic if player is hit
            if (HitResult.GetActor() == PlayerCharacter)
            {
                UE_LOG(LogTemp, Warning, TEXT("JudgementGaze hit the player!"));

                // Apply damage to the player
                float DamageAmount = 0.0f; // Adjust the damage amount as needed
                UGameplayStatics::ApplyDamage(
                    PlayerCharacter,          // The target actor (the player)
                    DamageAmount,             // The damage amount
                    ControlledPawn->GetController(), // The instigator (the AI controller)
                    ControlledPawn,           // The damage causer (the boss)
                    UDamageType::StaticClass() // The damage type
                );

                // Optional: Log the damage for debugging purposes
                GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Player damaged by Judgement Gaze!"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No hit detected!"));
        }

        // Visualize the trace for debugging
        DrawDebugSphere(GetWorld(), TargetLocation, TraceRadius, 24, FColor::Red, false, 0.1f, 0, 2.0f);
        DrawDebugLine(GetWorld(), BossLocation, TargetLocation, FColor::Red, false, 0.1f, 0, 2.0f);
    }
}

//void ARaphaelAIController::SpawnHeavenRainTrace()
//{
//    if (APawn* BossPawn = GetPawn())
//    {
//        float MaxDamage = 100.0f;
//        FVector BossLocation = BossPawn->GetActorLocation();
//        // Generate a random point within the specified radius
//        float RandomRadius = FMath::FRandRange(0.0f, HeavenRainRadius);
//        float RandomAngle = FMath::FRandRange(0.0f, 360.0f);
//        FVector Offset = FVector(RandomRadius * FMath::Cos(FMath::DegreesToRadians(RandomAngle)),
//            RandomRadius * FMath::Sin(FMath::DegreesToRadians(RandomAngle)),
//            0.0f);
//        FVector TraceStart = BossLocation + Offset + FVector(0.0f, 0.0f, 5000.0f);
//        FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, 7000.0f);
//        // Perform the line trace
//        FHitResult Hit;
//        FCollisionQueryParams QueryParams;
//        QueryParams.AddIgnoredActor(BossPawn);
//        bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
//        // Draw debug cylinder
//        float CylinderRadius = 200.0f;
//        float CylinderHeight = (TraceStart - TraceEnd).Size();
//        DrawDebugCylinder(GetWorld(),
//            TraceStart,
//            TraceEnd,
//            CylinderRadius,
//            32,
//            FColor::Blue,
//            false,
//            2.0f,
//            0,
//            2.0f);
//        if (bHit)
//        {
//            FVector CylinderCenter = (TraceStart + TraceEnd) * 0.5f;
//            // Get all pawns in the world (includes player and other characters)
//            TArray<AActor*> AllPawns;
//            UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), AllPawns);
//
//            // Check each pawn to see if it's within the cylinder
//            for (AActor* Actor : AllPawns)
//            {
//                if (Actor == BossPawn || !Actor)
//                    continue;
//
//                FVector ActorLocation = Actor->GetActorLocation();
//
//                // Check if actor is within cylinder height range
//                float ActorZ = ActorLocation.Z;
//                float CylinderTop = FMath::Max(TraceStart.Z, TraceEnd.Z);
//                float CylinderBottom = FMath::Min(TraceStart.Z, TraceEnd.Z);
//
//                if (ActorZ >= CylinderBottom && ActorZ <= CylinderTop)
//                {
//                    // Check if actor is within cylinder radius (2D distance from cylinder axis)
//                    FVector ActorLocation2D = FVector(ActorLocation.X, ActorLocation.Y, 0.0f);
//                    FVector CylinderCenter2D = FVector(CylinderCenter.X, CylinderCenter.Y, 0.0f);
//                    float Distance2D = FVector::Dist(ActorLocation2D, CylinderCenter2D);
//
//                    if (Distance2D <= CylinderRadius)
//                    {
//                        // Actor is within cylinder, apply damage
//                        UGameplayStatics::ApplyDamage(
//                            Actor,
//                            MaxDamage,
//                            BossPawn->GetController(),
//                            this,
//                            UDamageType::StaticClass()
//                        );
//
//                        UE_LOG(LogTemp, Warning, TEXT("Heaven's Rain damage applied to: %s, Damage: %f"),
//                            *Actor->GetName(), MaxDamage);
//                    }
//                }
//            }
//        }
//        // Increment the trace count
//        CurrentRainTraceCount++;
//        // Stop the ability if the required number of traces is reached
//        if (CurrentRainTraceCount >= HeavenRainTraceCount)
//        {
//            EndHeavenRain();
//        }
//    }
//}
//
//void ARaphaelAIController::StartHeavenRain()
//{
//    CurrentRainTraceCount = 0;
//
//    // Start the timer to trigger each trace with an interval
//    GetWorld()->GetTimerManager().SetTimer(HeavenRainTimerHandle, this, &ARaphaelAIController::SpawnHeavenRainTrace, HeavenRainInterval, true);
//
//    
//}
//
//
//
//void ARaphaelAIController::EndHeavenRain()
//{
//
//    // Clear the timer to stop further traces
//    GetWorld()->GetTimerManager().ClearTimer(HeavenRainTimerHandle);
//
//    UE_LOG(LogTemp, Warning, TEXT("Heaven's Rain ability ended."));
//}

void ARaphaelAIController::PerformAbility(EAbilityType AbilityType)
{
    IsAbilityActive = true;



    switch (AbilityType)
    {
    case EAbilityType::JacobsLadder:
        SummonBeamOnPlayer();
        GetWorld()->GetTimerManager().SetTimer(AbilityTimerHandle, this, &ARaphaelAIController::ResetAbility, JacobLadderDuration, false);
        break;

    case EAbilityType::BurstShoot:
        StartBurst();
        GetWorld()->GetTimerManager().SetTimer(AbilityTimerHandle, this, &ARaphaelAIController::ResetAbility, BurstDuration, false);
        break;

    case EAbilityType::ThrowAbility:
        StartThrowAbility();
        GetWorld()->GetTimerManager().SetTimer(AbilityTimerHandle, this, &ARaphaelAIController::ResetAbility, ThrowDuration, false);
        break;

    case EAbilityType::JudgementGaze:
        StartJudgementGaze();
        GetWorld()->GetTimerManager().SetTimer(AbilityTimerHandle, this, &ARaphaelAIController::ResetAbility, GazeDuration, false);
        break;

   /* case EAbilityType::HeavenRain:
        StartHeavenRain();
        GetWorld()->GetTimerManager().SetTimer(AbilityTimerHandle, this, &ARaphaelAIController::ResetAbility, RainDuration, false);
        break;*/

    case EAbilityType::HaloArc:
        StartHaloArc();
        break;

    case EAbilityType::DeathRing:
        StartDeathRing();
        break;

    default:
        IsAbilityActive = false;
        break;
    }
}

void ARaphaelAIController::ResetAbility()
{
    IsAbilityActive = false;
    UE_LOG(LogTemp, Warning, TEXT("Ability Reset!"));
}

void ARaphaelAIController::OnPlayerEnterTrigger(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

    if (OtherActor == PlayerCharacter && !bIsActivated && !bIsDelaying)
    {
        bIsDelaying = true; // We're now in the delay period, but not activated yet

        // Start a timer for activation delay
        GetWorld()->GetTimerManager().SetTimer(DelayTimerHandle, this, &ARaphaelAIController::ActivateBoss, ActivationDelay, false);

        // Optional: Trigger animation or dialogue here
        UE_LOG(LogTemp, Warning, TEXT("Player entered trigger zone. Activating boss in %f seconds..."), ActivationDelay);
    }
}

void ARaphaelAIController::ActivateBoss()
{
    bIsActivated = true; // Now we're fully activated
    bIsDelaying = false; // No longer in delay period

    // Make the boss vulnerable to damage now that it's activated
    if (ARaphaelBossCharacter* BossCharacter = Cast<ARaphaelBossCharacter>(GetPawn()))
    {
        BossCharacter->SetInvulnerable(false);
    }

    UE_LOG(LogTemp, Warning, TEXT("Boss activated. Starting behavior and making vulnerable to damage."));

    // Enable AI behavior
    StartBurst();
    StartBeamSummonWithDelay();
    StartJudgementGaze();
    //StartHeavenRain();
    StartHaloArc();
    GetWorld()->GetTimerManager().SetTimer(ThrowChargeTimerHandle, this, &ARaphaelAIController::StartThrowAbility, 10.0f, true);
}



void ARaphaelAIController::StopAllAbilities()
{
    // Clear all timers
    UWorld* World = GetWorld();
    if (World)
    {
        // Stop JacobsLadder ability
        World->GetTimerManager().ClearTimer(DelayTimerHandle);
        World->GetTimerManager().ClearTimer(TelegraphTimerHandle);
        World->GetTimerManager().ClearTimer(BeamTimerHandle);

        // Stop BurstShoot ability
        World->GetTimerManager().ClearTimer(ShotTimerHandle);
        World->GetTimerManager().ClearTimer(BurstTimerHandle);

        // Stop ThrowAbility
        World->GetTimerManager().ClearTimer(ThrowChargeTimerHandle);

        // Stop JudgementGaze ability
        World->GetTimerManager().ClearTimer(JudgementGazeTimerHandle);
        World->GetTimerManager().ClearTimer(JudgementGazeDurationTimerHandle);
        PlayerPositionHistory.Empty();

        // Stop HeavenRain ability
        World->GetTimerManager().ClearTimer(HeavenRainTimerHandle);

        // Stop main ability timer
        World->GetTimerManager().ClearTimer(AbilityTimerHandle);

        // Stop Halo Arc ability
        World->GetTimerManager().ClearTimer(HaloArcTimerHandle);
        World->GetTimerManager().ClearTimer(HaloArcDurationTimerHandle);

        // Stop Death Ring ability
        World->GetTimerManager().ClearTimer(DeathRingTimerHandle);
        World->GetTimerManager().ClearTimer(DeathRingUpdateTimerHandle);
    }

    // Reset state flags
    IsAbilityActive = false;
    bIsActivated = false;
    bIsDelaying = false;
    bDeathRingActive = false;  // Add this line

    // Reset Halo Arc count
    CurrentHaloArcCount = 0;

    // Clean up Death Ring colliders
    DestroyDeathRingColliders();  // Add this line

    UE_LOG(LogTemp, Warning, TEXT("All boss abilities stopped due to death"));
}

void ARaphaelAIController::StartHaloArc()
{
    CurrentHaloArcCount = 0;

    // Start spawning projectiles immediately, then continue with intervals
    SpawnHaloArcProjectiles();

    // Set timer for subsequent projectile spawns
    GetWorld()->GetTimerManager().SetTimer(
        HaloArcTimerHandle,
        this,
        &ARaphaelAIController::SpawnHaloArcProjectiles,
        HaloArcProjectileInterval,
        true
    );

    // Set timer to end the ability after the specified duration
    GetWorld()->GetTimerManager().SetTimer(
        HaloArcDurationTimerHandle,
        this,
        &ARaphaelAIController::EndHaloArc,
        HaloArcDuration,
        false
    );

    UE_LOG(LogTemp, Warning, TEXT("Halo Arc ability started"));
}

void ARaphaelAIController::SpawnHaloArcProjectiles()
{
    // Get the boss character
    ARaphaelBossCharacter* BossCharacter = Cast<ARaphaelBossCharacter>(GetPawn());
    if (!BossCharacter) return;

    // Call the boss character's function to spawn projectiles from the configured points
    BossCharacter->SpawnHaloArcProjectilesFromPoints();

    // Increment the count
    CurrentHaloArcCount++;

    // Check if we've reached the maximum number of projectile waves
    if (CurrentHaloArcCount >= HaloArcProjectileCount)
    {
        EndHaloArc();
    }
}

void ARaphaelAIController::EndHaloArc()
{
    // Clear the timers
    GetWorld()->GetTimerManager().ClearTimer(HaloArcTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(HaloArcDurationTimerHandle);

    // Reset the count
    CurrentHaloArcCount = 0;

    // Reset the ability state so the AI can pick a new ability
    IsAbilityActive = false;

    UE_LOG(LogTemp, Warning, TEXT("Halo Arc ability ended"));
}

void ARaphaelAIController::StartDeathRing()
{
    if (bDeathRingActive) return; // Don't start if already active

    bDeathRingActive = true;
    DeathRingStartTime = GetWorld()->GetTimeSeconds();

    // Create the ring colliders
    CreateDeathRingColliders();

    // Start updating the rings every frame
    GetWorld()->GetTimerManager().SetTimer(
        DeathRingUpdateTimerHandle,
        this,
        &ARaphaelAIController::UpdateDeathRings,
        0.02f, // Update every 0.02 seconds for smooth movement
        true
    );

    // End the ability after the duration
    GetWorld()->GetTimerManager().SetTimer(
        DeathRingTimerHandle,
        this,
        &ARaphaelAIController::EndDeathRing,
        DeathRingDuration,
        false
    );

    UE_LOG(LogTemp, Warning, TEXT("Death Ring ability started"));
}

void ARaphaelAIController::UpdateDeathRings()
{
    if (!bDeathRingActive || DeathRingColliders.Num() == 0) return;

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn) return;

    float CurrentTime = GetWorld()->GetTimeSeconds();
    float ElapsedTime = CurrentTime - DeathRingStartTime;
    float Progress = FMath::Clamp(ElapsedTime / DeathRingDuration, 0.0f, 1.0f);

    FVector BossLocation = ControlledPawn->GetActorLocation();

    // Update each ring
    for (int32 i = 0; i < DeathRingColliders.Num(); i++)
    {
        if (DeathRingColliders[i] && IsValid(DeathRingColliders[i]))
        {
            // Calculate the current radius for this ring
            float InitialRadius = DeathRingStartRadius + (i * DeathRingSpacing);
            float CurrentRadius = FMath::Lerp(InitialRadius, DeathRingEndRadius, Progress);

            // Update the capsule size
            DeathRingColliders[i]->SetCapsuleSize(CurrentRadius, DeathRingHeight / 2.0f);

            // Update position to stay centered on boss
            DeathRingColliders[i]->SetWorldLocation(BossLocation);

            // Draw debug visualization
            DrawDebugCylinder(
                GetWorld(),
                BossLocation - FVector(0, 0, DeathRingHeight / 2),
                BossLocation + FVector(0, 0, DeathRingHeight / 2),
                CurrentRadius,
                32,
                FColor::Purple,
                false,
                0.1f,
                0,
                3.0f
            );

            // Draw inner boundary for thickness visualization
            float InnerRadius = FMath::Max(CurrentRadius - DeathRingThickness, 0.0f);
            DrawDebugCylinder(
                GetWorld(),
                BossLocation - FVector(0, 0, DeathRingHeight / 2),
                BossLocation + FVector(0, 0, DeathRingHeight / 2),
                InnerRadius,
                32,
                FColor::Red,
                false,
                0.1f,
                0,
                2.0f
            );
        }
    }
}

void ARaphaelAIController::EndDeathRing()
{
    bDeathRingActive = false;

    // Clear the update timer
    GetWorld()->GetTimerManager().ClearTimer(DeathRingUpdateTimerHandle);

    // Clear damage cooldowns
    DeathRingDamageCooldowns.Empty();

    // Destroy all ring colliders
    DestroyDeathRingColliders();

    // Reset ability state
    IsAbilityActive = false;

    UE_LOG(LogTemp, Warning, TEXT("Death Ring ability ended"));
}

void ARaphaelAIController::CreateDeathRingColliders()
{
    // Clean up any existing colliders first
    DestroyDeathRingColliders();

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn) return;

    UE_LOG(LogTemp, Warning, TEXT("Creating Death Ring Colliders"));

    // Create 3 ring colliders
    for (int32 i = 0; i < 3; i++)
    {
        UCapsuleComponent* RingCollider = NewObject<UCapsuleComponent>(this, UCapsuleComponent::StaticClass());
        if (RingCollider)
        {
            // Calculate initial radius for this ring
            float InitialRadius = DeathRingStartRadius + (i * DeathRingSpacing);

            // Initialize the capsule size (radius, half-height)
            RingCollider->InitCapsuleSize(InitialRadius, DeathRingHeight / 2.0f);

            // Set collision properties - THIS IS CRUCIAL
            RingCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            RingCollider->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
            RingCollider->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Overlap);

            // IMPORTANT: Also set collision object type
            RingCollider->SetCollisionObjectType(ECC_WorldDynamic);

            // Set world location to boss position
            FVector BossLocation = ControlledPawn->GetActorLocation();
            RingCollider->SetWorldLocation(BossLocation);

            // Register the component
            RingCollider->RegisterComponent();

            // Bind overlap event
            RingCollider->OnComponentBeginOverlap.AddDynamic(this, &ARaphaelAIController::OnDeathRingOverlap);

            // Add to our array
            DeathRingColliders.Add(RingCollider);

            UE_LOG(LogTemp, Warning, TEXT("Created Death Ring %d with radius %f"), i, InitialRadius);
        }
    }
}

void ARaphaelAIController::DestroyDeathRingColliders()
{
    for (UCapsuleComponent* RingCollider : DeathRingColliders)
    {
        if (RingCollider && IsValid(RingCollider))
        {
            // Unbind the overlap event
            RingCollider->OnComponentBeginOverlap.RemoveDynamic(this, &ARaphaelAIController::OnDeathRingOverlap);

            // Destroy the component
            RingCollider->DestroyComponent();
        }
    }

    // Clear the array
    DeathRingColliders.Empty();
}

void ARaphaelAIController::OnDeathRingOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    UE_LOG(LogTemp, Warning, TEXT("Death Ring Overlap Event Triggered with actor: %s"),
        OtherActor ? *OtherActor->GetName() : TEXT("NULL"));

    // Check if the overlapped actor is the player
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (OtherActor && OtherActor == PlayerCharacter)
    {
        // Get the ring collider that was hit
        UCapsuleComponent* HitRing = Cast<UCapsuleComponent>(OverlappedComponent);
        if (!HitRing)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to cast to UCapsuleComponent"));
            return;
        }

        // Check damage cooldown
        float CurrentTime = GetWorld()->GetTimeSeconds();
        float LastDamageTime = DeathRingDamageCooldowns.FindRef(PlayerCharacter);

        if (CurrentTime - LastDamageTime < DeathRingDamageCooldown)
        {
            return; // Still in cooldown
        }

        // Check if player is actually inside the ring (not in the safe center)
        FVector PlayerLocation = PlayerCharacter->GetActorLocation();
        FVector RingCenter = HitRing->GetComponentLocation();
        float DistanceFromCenter = FVector::Dist2D(PlayerLocation, RingCenter);

        float RingRadius = HitRing->GetScaledCapsuleRadius();
        float InnerRadius = FMath::Max(RingRadius - DeathRingThickness, 0.0f);

        UE_LOG(LogTemp, Warning, TEXT("Player distance from center: %f, Inner radius: %f, Outer radius: %f"),
            DistanceFromCenter, InnerRadius, RingRadius);

        // Player is in danger zone if they're between inner and outer radius
        bool bPlayerInDangerZone = (DistanceFromCenter > InnerRadius && DistanceFromCenter < RingRadius);

        if (bPlayerInDangerZone)
        {
            // Apply damage to the player
            UGameplayStatics::ApplyDamage(
                OtherActor,
                DeathRingDamage,
                GetPawn()->GetController(),
                GetPawn(),
                UDamageType::StaticClass()
            );

            // Update cooldown
            DeathRingDamageCooldowns.Add(PlayerCharacter, CurrentTime);

            // Visual feedback
            GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red,
                FString::Printf(TEXT("Death Ring Hit! Damage: %f"), DeathRingDamage));

            UE_LOG(LogTemp, Warning, TEXT("Player damaged by Death Ring for %f damage"), DeathRingDamage);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Player in safe zone - no damage applied"));
        }
    }
}





