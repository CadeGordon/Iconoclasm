// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RangedEnemyCharacter.generated.h"

UCLASS()
class ICONOCLASM_API ARangedEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARangedEnemyCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Sphere collider to determine projectile spawn point
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	class USphereComponent* ProjectileSpawnSphere;

	// Weapon firing logic
	void FireWeapon(const FVector& Direction);

	

private:
	// Projectile class to spawn (set this in the blueprint or dynamically)
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<class AIconoclasmProjectile> ProjectileClass;

	// Shooting cooldown handled here or in the controller
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float ShootingCooldown = 2.0f;

};
