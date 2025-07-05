// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShotgunProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class ICONOCLASM_API AShotgunProjectile : public AActor
{
	GENERATED_BODY()


	public:
	/** Sphere collision component */
	UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
	USphereComponent* ShotgunCollisionComp;

	/** Damage radius sphere component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile)
	USphereComponent* DamageRadiusComp;

	/** Projectile movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ShotgunProjectileMovement;
	
public:	
	// Sets default values for this actor's properties
	AShotgunProjectile();

	/** called when projectile hits something */
	UFUNCTION()
	void ShotgunOnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	///** called when projectile hits something */
	//UFUNCTION()
	//void AltOnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Returns CollisionComp subobject **/
	USphereComponent* GetCollisionComp() const { return ShotgunCollisionComp; }
	/** Returns ProjectileMovement subobject **/
	UProjectileMovementComponent* GetProjectileMovement() const { return ShotgunProjectileMovement; }

	/** Function to initialize the projectile's velocity in the shoot di
	rection */
	void ShotgunFireInDirection(const FVector& ShootDirection);

	/** Function to apply damage to enemies in radius */
	UFUNCTION(BlueprintCallable, Category = Damage)
	void ApplyDamageInRadius(const FVector& Origin);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Bouncing", meta = (AllowPrivateAccess = "true"))
	int32 BounceCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Bouncing", meta = (AllowPrivateAccess = "true"))
	int32 MaxBounces;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Bouncing", meta = (AllowPrivateAccess = "true"))
	float SpeedMultiplierPerBounce;

	/** Damage amount to deal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Damage)
	float DamageAmount = 50.0f;

	/** Damage radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Damage)
	float DamageRadius = 100.0f;

	/** Whether to apply damage on hit or on each bounce */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Damage)
	bool bDamageOnEachBounce = true;

	/** Reference to the player character who fired this projectile */
	UPROPERTY(BlueprintReadWrite, Category = Projectile)
	ACharacter* PlayerCharacter;

	/** Set the player character reference */
	UFUNCTION(BlueprintCallable, Category = Projectile)
	void SetPlayerCharacter(ACharacter* InPlayerCharacter);
};
