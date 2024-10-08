// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FlyingEnemyCharacter.generated.h"

class UFloatingPawnMovement;

UCLASS()
class ICONOCLASM_API AFlyingEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AFlyingEnemyCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<class AIconoclasmProjectile> ProjectileClass;

	/** Sphere collider acting as the muzzle location */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	class USphereComponent* MuzzleSphere;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Floating pawn movement component to allow flying
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	UFloatingPawnMovement* MovementComponent;

	// Set the AI controller class to be used by this character
	UPROPERTY(EditAnywhere, Category = "AI")
	TSubclassOf<class AFlyingAIController> FlyAIControllerClass;

	// Function to handle movement mode or any setup needed for flying
	void InitializeFlyingSettings();

	/** Function to shoot a projectile at the player */
	void ShootAtPlayer(APawn* Target);

};
