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

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// Existing
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	void FireInDirection(const FVector& ShootDirection);

	// Existing reflect behavior (melee punch)
	void ReflectProjectile(const FVector& ReflectionDirection, AActor* NewInstigator);

	// Existing
	UFUNCTION()
	void AltOnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	void SetTrackingEnabled(bool bEnabled);
	void SetTrackingStrength(float NewStrength);

	// =========================
	// NEW: Grapple + Hold + Throw
	// =========================

	// Start being pulled by grapple
	void StartGrapplePull(ACharacter* PullingCharacter);

	// Stop being pulled (cancel)
	void StopGrapplePull();

	// True if we can currently be grabbed by grapple
	bool IsGrapplable() const;

	// Attach/child to player (held)
	void AttachToPlayer(ACharacter* NewHolder, const FName& SocketName, const FVector& RelativeOffset);

	// Throw from player (explodes on impact using reflected explosion logic)
	void ThrowFromPlayer(const FVector& ThrowDirection, float ThrowSpeed, AActor* NewInstigator);

	// Is it currently held?
	bool IsHeldByPlayer() const { return bIsHeldByPlayer; }

	// Get current holder
	ACharacter* GetHoldingCharacter() const { return HoldingCharacter.Get(); }

	


	// Bullet-hell tracking
	void FindTargetPlayer();
	void UpdatePlayerTracking(float DeltaTime);

	UPROPERTY()
	ACharacter* TargetPlayer;

	UPROPERTY(EditAnywhere, Category = "Tracking")
	float TrackingStrength;

	UPROPERTY(EditAnywhere, Category = "Tracking")
	float MaxTrackingDistance;

	UPROPERTY(EditAnywhere, Category = "Tracking")
	bool bCanTrackPlayer;

	// Reflection/explosion
	UPROPERTY()
	bool bIsReflected;

	UPROPERTY(EditAnywhere, Category = "Reflection")
	float ReflectedExplosionRadius;

	UPROPERTY(EditAnywhere, Category = "Reflection")
	float ReflectedExplosionDamage;

	UPROPERTY()
	APawn* OriginalInstigator;

	// =========================
	// NEW: Grapple state
	// =========================
	UPROPERTY()
	bool bIsBeingGrappled;

	UPROPERTY()
	bool bIsHeldByPlayer;

	UPROPERTY()
	TWeakObjectPtr<ACharacter> HoldingCharacter;

	UPROPERTY(EditAnywhere, Category = "Grapple")
	float GrapplePullSpeed;

	// When held, we usually want to disable collision, then re-enable when thrown
	void SetHeldCollision(bool bHeld);

	// Small safety: ignore player collision briefly after throw to prevent instant self-hit
	void ReenablePawnCollision();

	FTimerHandle TimerHandle_ReenablePawnCollision;
};

