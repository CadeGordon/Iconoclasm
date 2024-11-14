// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RaphaelBossCharacter.generated.h"

UCLASS()
class ICONOCLASM_API ARaphaelBossCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARaphaelBossCharacter();

	// Function to spawn the projectile
	void SpawnProjectile(FVector LaunchDirection);

	// Function to push the player back
	void PushBackPlayer(ACharacter* PlayerCharacter);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Overlap event
	UFUNCTION()
	void OnPushBackSphereOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	// Projectile class to spawn
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<AActor> ProjectileClass;

	// The sphere collider component to use as the projectile's spawn location
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* ProjectileSpawnPoint;

	// Distance within which the player will be pushed back
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Push Back")
	float PushBackDistance = 300.0f; // You can adjust this value in the editor

	// Force applied during push back
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Push Back")
	float PushBackForce = 800.0f; // You can adjust this value in the editor

	// Sphere component to determine push back radius
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Push Back")
	USphereComponent* PushBackSphere;

	// Array of actors set in the editor to serve as healing sources
	UPROPERTY(EditAnywhere, Category = "Healing")
	TArray<AActor*> HealingLightSources;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	
};
