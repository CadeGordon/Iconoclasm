// Fill out your copyright notice in the Description page of Project Settings.


#include "FlyingEnemyCharacter.h"
#include "FlyingAIController.h"
#include "GameFramework/CharacterMovementComponent.h"  // Include this line
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/Controller.h"
#include "IconoclasmProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AFlyingEnemyCharacter::AFlyingEnemyCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Assign the inherited capsule component to the property
	CapsuleRoot = GetCapsuleComponent();

	// Initialize movement component for flying
	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));

	// Set default AIController class, if not set in Blueprint
	AIControllerClass = AFlyingAIController::StaticClass();

	// Ensure the AI controller is assigned when the character is spawned
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Create and set up the sphere collider for the projectile muzzle
	MuzzleSphere = CreateDefaultSubobject<USphereComponent>(TEXT("MuzzleSphere"));
	MuzzleSphere->SetupAttachment(RootComponent);

	// You can set the size of the sphere if needed
	MuzzleSphere->SetSphereRadius(10.0f);
	MuzzleSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Only used for positioning

    // Set the "Enemy" tag
    Tags.Add(FName("Enemy"));
}

// Called when the game starts or when spawned
void AFlyingEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Initialize flying settings for the character
	InitializeFlyingSettings();
}

// Called every frame
void AFlyingEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Get the player character
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!PlayerCharacter) return;

	// Get the direction to the player
	FVector DirectionToPlayer = PlayerCharacter->GetActorLocation() - GetActorLocation();

	// Optional: ignore Z axis if you only want yaw rotation (like a turret)
	// DirectionToPlayer.Z = 0.0f;

	if (!DirectionToPlayer.IsNearlyZero())
	{
		FRotator TargetRotation = DirectionToPlayer.Rotation();

		// Optional: restrict to yaw only
		// TargetRotation.Pitch = 0.0f;
		// TargetRotation.Roll = 0.0f;

		FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 3.0f); // 3.0f is rotation speed
		SetActorRotation(NewRotation);
	}

}

// Called to bind functionality to input
void AFlyingEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);



}

void AFlyingEnemyCharacter::InitializeFlyingSettings()
{
	if (MovementComponent)
    {
        // Set flying movement specifics like speed and acceleration
        MovementComponent->MaxSpeed = 600.0f; // Flying speed
        MovementComponent->Acceleration = 200.0f;
    }

    // Optionally, you could also configure the pawn's collision and gravity behavior
    SetCanBeDamaged(true);

    // Ensure gravity isn't affecting the flying enemy (this is important for flying characters)
    //GetMesh()->SetEnableGravity(false);
}

void AFlyingEnemyCharacter::ShootAtPlayer(APawn* Target)
{
    // Add comprehensive null checks using modern Unreal API
    if (!ProjectileClass)
    {
        UE_LOG(LogTemp, Error, TEXT("ProjectileClass is not set on %s"), *GetName());
        return;
    }

    if (!IsValid(Target))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid target for shooting"));
        return;
    }

    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        UE_LOG(LogTemp, Error, TEXT("World is null when trying to shoot"));
        return;
    }

    if (!IsValid(MuzzleSphere))
    {
        UE_LOG(LogTemp, Error, TEXT("MuzzleSphere is null on %s"), *GetName());
        return;
    }

    FVector MuzzlePosition = MuzzleSphere->GetComponentLocation();
    FVector TargetLocation = Target->GetActorLocation();
    FVector DirectionToTarget = TargetLocation - MuzzlePosition;

    // Check if direction is valid
    if (DirectionToTarget.IsNearlyZero())
    {
        UE_LOG(LogTemp, Warning, TEXT("Target is too close to muzzle position"));
        return;
    }

    FRotator MuzzleRotation = DirectionToTarget.Rotation();

    // Spawn the projectile
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = GetInstigator();
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AIconoclasmProjectile* Projectile = World->SpawnActor<AIconoclasmProjectile>(
        ProjectileClass,
        MuzzlePosition,
        MuzzleRotation,
        SpawnParams
    );

    if (IsValid(Projectile))
    {
        FVector LaunchDirection = MuzzleRotation.Vector();
        Projectile->FireInDirection(LaunchDirection);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn projectile"));
    }
}



