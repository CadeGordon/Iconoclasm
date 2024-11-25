// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthComponent.h"

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

	// Check for death
	if (CurrentHealth <= 0.0f)
	{
		bIsDead = true;
		OnDeath.Broadcast();
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




