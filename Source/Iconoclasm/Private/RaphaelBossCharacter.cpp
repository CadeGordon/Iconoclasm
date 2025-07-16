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
#include "RaphHealthBar.h"
#include "Components/ProgressBar.h"
#include "Blueprint/UserWidget.h"
#include "HealthComponent.h"
#include "RaphaelAIController.h"


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

	// Create the detection sphere
	PlayerDetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PlayerDetectionSphere"));
	PlayerDetectionSphere->SetupAttachment(RootComponent);
	PlayerDetectionSphere->SetSphereRadius(1000.0f);  // Adjust the radius as needed
	PlayerDetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ARaphaelBossCharacter::OnPlayerEnterBossArea);

	// Initialize Health Component
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));


	// Initialize default Halo Arc spawn points (5 points in an arc around the boss)
	HaloArcSpawnPoints.SetNum(5);

	// Set up default positions - you can adjust these values or move them in the editor
	HaloArcSpawnPoints[0] = FVector(300.0f, -400.0f, 100.0f);  // Left side
	HaloArcSpawnPoints[1] = FVector(500.0f, -200.0f, 100.0f);  // Left-center
	HaloArcSpawnPoints[2] = FVector(600.0f, 0.0f, 100.0f);     // Center
	HaloArcSpawnPoints[3] = FVector(500.0f, 200.0f, 100.0f);   // Right-center
	HaloArcSpawnPoints[4] = FVector(300.0f, 400.0f, 100.0f);   // Right side


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

	// Bind to HealthComponent events
	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.AddDynamic(this, &ARaphaelBossCharacter::OnBossHealthChanged);
		HealthComponent->OnDeath.AddDynamic(this, &ARaphaelBossCharacter::OnBossDeath);
	}

	// Create but hide Health Bar UI at first
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC && BossHealthWidgetClass)
	{
		BossHealthWidget = CreateWidget<URaphHealthBar>(PC, BossHealthWidgetClass);
	}
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


void ARaphaelBossCharacter::OnPlayerEnterBossArea(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACharacter* PlayerCharacter = Cast<ACharacter>(OtherActor);
	if (PlayerCharacter && PlayerCharacter == UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
	{
		if (BossHealthWidget && !BossHealthWidget->IsInViewport())
		{
			BossHealthWidget->AddToViewport();
			UpdateBossHealthUI(HealthComponent->GetCurrentHealth());
		}
	}
}


void ARaphaelBossCharacter::OnBossHealthChanged(float NewHealth)
{
	if (BossHealthWidget && HealthComponent)
	{
		float HealthPercentage = NewHealth / HealthComponent->MaxHealth;
		BossHealthWidget->UpdateHealthBar(HealthPercentage);
	}
}

void ARaphaelBossCharacter::OnBossDeath()
{
	if (BossHealthWidget)
	{
		BossHealthWidget->RemoveFromParent();
		BossHealthWidget = nullptr;
	}

	// Stop all abilities in the AI controller
	ARaphaelAIController* BossAI = Cast<ARaphaelAIController>(GetController());
	if (BossAI)
	{
		BossAI->StopAllAbilities();
	}

}

void ARaphaelBossCharacter::UpdateBossHealthUI(float CurrentHealth)
{
	if (BossHealthWidget && HealthComponent)
	{
		float HealthPercentage = CurrentHealth / HealthComponent->MaxHealth;
		BossHealthWidget->UpdateHealthBar(HealthPercentage);
	}
}

void ARaphaelBossCharacter::SetInvulnerable(bool bShouldBeInvulnerable)
{
	bIsInvulnerable = bShouldBeInvulnerable;

	if (bIsInvulnerable)
	{
		UE_LOG(LogTemp, Warning, TEXT("Boss is now invulnerable"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Boss is now vulnerable to damage"));
	}
}

float ARaphaelBossCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	class AController* EventInstigator, AActor* DamageCauser)
{
	// If the boss is invulnerable, don't take any damage
	if (bIsInvulnerable)
	{
		UE_LOG(LogTemp, Warning, TEXT("Boss is invulnerable - damage blocked!"));
		return 0.0f; // No damage taken
	}

	// If not invulnerable, proceed with normal damage handling
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void ARaphaelBossCharacter::SpawnHaloArcProjectilesFromPoints()
{

	if (!ProjectileClass) return;

	// Get the player's current location for targeting
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!PlayerCharacter) return;

	FVector PlayerLocation = PlayerCharacter->GetActorLocation();
	FVector BossLocation = GetActorLocation();

	// Spawn projectiles from each configured point
	for (int32 i = 0; i < HaloArcSpawnPoints.Num(); i++)
	{
		// Calculate world position of spawn point relative to boss
		FVector WorldSpawnLocation = BossLocation + HaloArcSpawnPoints[i];

		// Calculate direction from spawn point to player
		FVector DirectionToPlayer = (PlayerLocation - WorldSpawnLocation).GetSafeNormal();
		FRotator SpawnRotation = DirectionToPlayer.Rotation();

		// Spawn the projectile
		FActorSpawnParameters SpawnParams;
		AActor* SpawnedProjectile = GetWorld()->SpawnActor<AIconoclasmProjectile>(
			ProjectileClass,
			WorldSpawnLocation,
			SpawnRotation,
			SpawnParams
		);

		// Optional: Draw debug sphere to visualize spawn points
		DrawDebugSphere(GetWorld(), WorldSpawnLocation, 20.0f, 12, FColor::Purple, false, 2.0f, 0, 2.0f);

		// Optional: Draw debug line showing projectile direction
		DrawDebugLine(GetWorld(), WorldSpawnLocation, WorldSpawnLocation + (DirectionToPlayer * 500.0f),
			FColor::Purple, false, 2.0f, 0, 2.0f);
	}

	UE_LOG(LogTemp, Warning, TEXT("Halo Arc projectiles spawned from %d points"), HaloArcSpawnPoints.Num());
}
