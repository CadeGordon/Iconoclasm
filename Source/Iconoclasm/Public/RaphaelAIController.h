// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "RaphaelAIController.generated.h"


class AActor;
class ARaphaelBossCharacter;

/**
 * 
 */
UCLASS()
class ICONOCLASM_API ARaphaelAIController : public AAIController
{
	GENERATED_BODY()
	
public:
    // Constructor
    ARaphaelAIController();

    // Function to summon the beam on the player
    void SummonBeamOnPlayer();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

private:

    // Cylinder Collider to follow player
    UPROPERTY()
    class UCapsuleComponent* JudgementGazeCollider;

    // Helper function to handle the beam attack effects
    void SpawnBeamColliderAtLocation(FVector Location);

    // Function to trigger the next beam after the delay
    void StartBeamSummonWithDelay();

    // Number of line traces per Heaven's Rain move
    int32 HeavenRainTraceCount = 20;

    // Radius within which the traces should spawn
    float HeavenRainRadius = 3000.0f;

    // Interval between each line trace in seconds
    float HeavenRainInterval = 0.5f;

    // Timer handle for managing Heaven's Rain interval
    FTimerHandle HeavenRainTimerHandle;

    // Counter to track the number of traces spawned in the current Heaven's Rain ability
    int32 CurrentRainTraceCount = 0;


    // Reference to the player character
    APawn* PlayerPawn;

    // Number of projectiles in a burst
    int32 ProjectilesPerBurst = 5;

    // Delay between shots in a burst
    float ShotInterval = 0.4f;

    // Delay between bursts
    float BurstInterval = 2.0f;

    // Keeps track of the current number of shots fired in the current burst
    int32 CurrentShotCount;

    UPROPERTY(EditAnywhere, Category = "Beam Attack")
    float BeamRadius = 200.0f; // The radius of the beam attack area

    UPROPERTY(EditAnywhere, Category = "Beam Attack")
    float BeamHeight = 1000.0f; // The height of the beam attack

    UPROPERTY(EditAnywhere, Category = "Beam Attack")
    float BeamDuration = 3.0f; // How long the beam stays active

    UPROPERTY(EditAnywhere, Category = "Beam Attack")
    float DelayBetweenBeams = 5.0f; // Delay between each beam summon

    UPROPERTY(EditDefaultsOnly, Category = "Abilities")
    bool IsJudgementGazeActive = false; // Flag to track if Judgement Gaze is active

    UPROPERTY(EditAnywhere, Category = "Judgement Gaze")
    class USphereComponent* JudgementGazeSpawnPoint;  // Spawn point for the beam

    // The delay between each trace check to trail behind the player
    float TraceDelay = 0.1f;

    // Duration of the JudgementGaze
    float JudgementGazeDuration = 5.0f;

    // Array to store player positions
    TArray<FVector> PlayerPositionHistory;

    

    // Distance behind the player
    float TrailOffsetDistance = 200.0f;

    // Timer handles for the JudgementGaze attack
    FTimerHandle JudgementGazeTimerHandle;
    FTimerHandle JudgementGazeDurationTimerHandle;

    FTimerHandle BeamTimerHandle;
    FTimerHandle DelayTimerHandle;  // Timer handle for delay between beams

    // Timers for shooting projectiles and starting a new burst
    FTimerHandle ShotTimerHandle;
    FTimerHandle BurstTimerHandle;

    // Timer handles for charge-up and explosion delay
    FTimerHandle ThrowChargeTimerHandle;

    // Explosion variables
    UPROPERTY(EditAnywhere, Category = "Explosion")
    UParticleSystem* ExplosionEffect; // The explosion particle effect

    UPROPERTY(EditAnywhere, Category = "Explosion")
    float ExplosionDamage = 100.0f;   // Amount of damage the explosion deals

    UPROPERTY(EditAnywhere, Category = "Explosion")
    float ExplosionRadius = 300.0f;   // Radius of the explosion

    // Function to handle shooting a projectile
    void ShootProjectile();

    // Function to start a burst of projectiles
    void StartBurst();

    // Function to reset for the next burst
    void ResetBurst();

    // Function to start the throw ability (with charge-up)
    void StartThrowAbility();

    // Function to execute the throw after charge-up
    void ExecuteThrow();

    // Function to spawn the explosion at the hit location
    void SpawnExplosionAtLocation(FVector Location);

    // Start the JudgmentGaze ability
    void StartJudgementGaze();
    void StopJudgementGaze();

    // Update the location of the JudgmentGaze collider behind the player
    void PerformJudgementGaze();

    // Function to perform one Heaven's Rain line trace
    void SpawnHeavenRainTrace();

    // Function to start the Heaven's Rain ability
    void StartHeavenRain();

    // Function to reset after Heaven's Rain is complete
    void EndHeavenRain();

   

};
