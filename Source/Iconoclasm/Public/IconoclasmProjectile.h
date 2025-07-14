// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IconoclasmProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS(config=Game)
class AIconoclasmProjectile : public AActor
{
	GENERATED_BODY()

	/** Sphere collision component */
	UPROPERTY(VisibleDefaultsOnly, Category=Projectile)
	USphereComponent* CollisionComp;

	/** Projectile movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement;

public:
	AIconoclasmProjectile();

	// Tracking properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet Hell")
	float TrackingStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet Hell")
	float MaxTrackingDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet Hell")
	bool bCanTrackPlayer;

	// Reference to the target player
	UPROPERTY(BlueprintReadOnly, Category = "Bullet Hell")
	class ACharacter* TargetPlayer;

	/** called when projectile hits something */
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** called when projectile hits something */
	UFUNCTION()
	void AltOnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Returns CollisionComp subobject **/
	USphereComponent* GetCollisionComp() const { return CollisionComp; }
	/** Returns ProjectileMovement subobject **/
	UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }

	/** Function to initialize the projectile's velocity in the shoot direction */
	void FireInDirection(const FVector& ShootDirection);

	// Override BeginPlay and Tick
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// Tracking functions
	UFUNCTION(BlueprintCallable, Category = "Bullet Hell")
	void FindTargetPlayer();

	UFUNCTION(BlueprintCallable, Category = "Bullet Hell")
	void UpdatePlayerTracking(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Bullet Hell")
	void SetTrackingEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Bullet Hell")
	void SetTrackingStrength(float NewStrength);
};

