// Fill out your copyright notice in the Description page of Project Settings.


#include "RaphaelBossCharacter.h"
#include "RaphaelAIController.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "IconoclasmProjectile.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"


// Sets default values
ARaphaelBossCharacter::ARaphaelBossCharacter()
{
	// Set AIController class to use RaphaelAIController
	AIControllerClass = ARaphaelAIController::StaticClass();

 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Initialize the sphere collider for projectile spawn location
	ProjectileSpawnPoint = CreateDefaultSubobject<USphereComponent>(TEXT("ProjectileSpawnPoint"));
	ProjectileSpawnPoint->SetupAttachment(RootComponent);
	ProjectileSpawnPoint->SetRelativeLocation(FVector(100.0f, 0.0f, 50.0f)); // Adjust location if needed

	// Create and initialize the sphere collider
	PushBackSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PushBackSphere"));
	PushBackSphere->SetupAttachment(RootComponent);
	PushBackSphere->SetSphereRadius(300.0f);  // Adjust the radius as needed
	PushBackSphere->OnComponentBeginOverlap.AddDynamic(this, &ARaphaelBossCharacter::OnPushBackSphereOverlapBegin);

}

void ARaphaelBossCharacter::SpawnProjectile(FVector LaunchDirection)
{
	if (!ProjectileClass) return;

	// Get the location and rotation for the projectile spawn
	FVector SpawnLocation = ProjectileSpawnPoint->GetComponentLocation();
	FRotator SpawnRotation = LaunchDirection.Rotation();

	// Spawn the projectile
	FActorSpawnParameters SpawnParams;
	GetWorld()->SpawnActor<AIconoclasmProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
}

void ARaphaelBossCharacter::PushBackPlayer(ACharacter* PlayerCharacter)
{
	if (PlayerCharacter)
	{
		FVector PushDirection = PlayerCharacter->GetActorLocation() - GetActorLocation();
		PushDirection.Normalize();

		// Apply an impulse to the player's movement component to push them back
		FVector PushForce = PushDirection * 100000.0f;  // Adjust the strength of the push
		PlayerCharacter->GetCharacterMovement()->AddImpulse(PushForce, true);
	}
}

// Called when the game starts or when spawned
void ARaphaelBossCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Overlap event
void ARaphaelBossCharacter::OnPushBackSphereOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	// Check if the overlapping actor is the player character
	ACharacter* PlayerCharacter = Cast<ACharacter>(OtherActor);
	if (PlayerCharacter && PlayerCharacter == UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
	{
		// Push the player back
		PushBackPlayer(PlayerCharacter);
	}
}

// Called every frame
void ARaphaelBossCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ARaphaelBossCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}



