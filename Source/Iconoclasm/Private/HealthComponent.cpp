// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthComponent.h"
#include "IconoclasmCharacter.h"
#include "HealthPack.h"
#include "ScoreComponent.h"
#include "ScoreWidget.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// Set default values
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
	bIsDead = false;
	BloodEffectClass = nullptr;

	// ...
}


// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// Set current health to max at the start
	CurrentHealth = MaxHealth;

	// Bind to the actor's OnTakeAnyDamage event
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
	}
	
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.0f || bIsDead)
	{
		return;
	}
	// Apply damage
	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);
	// Broadcast health change
	OnHealthChanged.Broadcast(CurrentHealth);

	// --- Award points for hitting an enemy ---
	if (InstigatedBy && InstigatedBy->GetPawn())
	{
		AActor* InstigatorActor = InstigatedBy->GetPawn();
		if (AIconoclasmCharacter* PlayerCharacter = Cast<AIconoclasmCharacter>(InstigatorActor))
		{
			if (UScoreComponent* ScoreComp = PlayerCharacter->FindComponentByClass<UScoreComponent>())
			{
				// Small score for hitting an enemy
				int32 HitPoints = 25; // adjust as needed
				ScoreComp->AddScore(HitPoints);

				// Determine bonus message
				FString HitMessage = TEXT("+Hit");
				FLinearColor HitColor = FLinearColor::Gray;

				if (PlayerCharacter->bLastAttackWasSlam)
				{
					HitPoints += 50; // extra points for slam hit
					HitMessage = TEXT("+SlamHit");
					HitColor = FLinearColor::Red;
				}

				// Show message in UI
				if (ScoreComp->ScoreWidgetInstance)
				{
					ScoreComp->ScoreWidgetInstance->AddKillMessage(HitMessage, HitColor);
				}

				UE_LOG(LogTemp, Log, TEXT("Hit registered, awarded %d points: %s"), HitPoints, *HitMessage);
			}
		}
	}

	// Check for death
	if (CurrentHealth <= 0.0f)
	{
		bIsDead = true;
		OnDeath.Broadcast();

		AIconoclasmCharacter* Player = Cast<AIconoclasmCharacter>(GetOwner());
		if (Player)
		{
			// Player specific logic show death screen instead of destroying
			Player->ShowDeathScreen();
		}
		else
		{
			// This is an enemy that died award score to player
			if (InstigatedBy && InstigatedBy->GetPawn())
			{
				AActor* KillerActor = InstigatedBy->GetPawn();

				// Check if the killer was the player
				if (AIconoclasmCharacter* KillerCharacter = Cast<AIconoclasmCharacter>(KillerActor))
				{
					// If the last attack was a slam, mark this kill as a SlamKill
					if (KillerCharacter->bLastAttackWasSlam)
					{
						KillerCharacter->bLastKillWasSlam = true;
					}
				}

				// Get the player's score component
				if (UScoreComponent* ScoreComp = KillerActor->FindComponentByClass<UScoreComponent>())
				{
					// Get the enemy type from the actor's class name
					FString EnemyType = GetOwner()->GetClass()->GetName();
					// Remove the  prefix that Unreal adds to actor class names
					if (EnemyType.StartsWith(TEXT("A")))
					{
						EnemyType = EnemyType.RightChop(1);
					}

					ScoreComp->AddScoreForEnemy(EnemyType);
					ScoreComp->AddKill();
				}
			}

			// Spawn blood effect at actor's location
			if (BloodEffectClass && GetWorld())
			{
				FActorSpawnParameters SpawnParams;
				GetWorld()->SpawnActor<AActor>(
					BloodEffectClass,
					GetOwner()->GetActorLocation(),
					FRotator::ZeroRotator,
					SpawnParams
				);
			}

			// Spawn health pack at actor's location
			if (GetWorld() && HealthPackClass)
			{
				FActorSpawnParameters SpawnParams;
				GetWorld()->SpawnActor<AHealthPack>(
					HealthPackClass,
					GetOwner()->GetActorLocation(),
					FRotator::ZeroRotator,
					SpawnParams
				);
			}

			// Destroy any other actor that has a health component
			AActor* Owner = GetOwner();
			if (Owner)
			{
				Owner->Destroy();
			}
		}
	}

}

float UHealthComponent::GetCurrentHealth() const
{
	return CurrentHealth;
}

bool UHealthComponent::IsDead() const
{
	return bIsDead;
}

void UHealthComponent::Heal(float Amount)
{
	if (Amount > 0.0f && !bIsDead)
	{
		CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, MaxHealth);
		OnHealthChanged.Broadcast(CurrentHealth);
	}
}

void UHealthComponent::TakeDamage(float Damage)
{
	HandleTakeAnyDamage(GetOwner(), Damage, nullptr, nullptr, nullptr);
}


void UHealthComponent::SetCurrentHealth(float NewHealth)
{
	CurrentHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth);
	bIsDead = (CurrentHealth <= 0.0f);
}

