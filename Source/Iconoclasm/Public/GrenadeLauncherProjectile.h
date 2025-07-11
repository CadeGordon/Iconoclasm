// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrenadeLauncherProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class ICONOCLASM_API AGrenadeLauncherProjectile : public AActor
{
	GENERATED_BODY()

public:
	/** Sphere collision component */
	UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
	USphereComponent* GrenadeCollisionComp;

	/** Damage radius sphere component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile)
	USphereComponent* GrenadeDamageRadiusComp;

	/** Projectile movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* GrenadeProjectileMovement;
	
public:	
	// Sets default values for this actor's properties
	AGrenadeLauncherProjectile();

protected:
	

public:	
	/** called when projectile hits something */
	UFUNCTION()
	void GrenadeOnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	///** called when projectile hits something */
	UFUNCTION()
	void GrenadeAltOnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Returns CollisionComp subobject **/
	USphereComponent* GetCollisionComp() const { return GrenadeCollisionComp; }
	/** Returns ProjectileMovement subobject **/
	UProjectileMovementComponent* GetProjectileMovement() const { return GrenadeProjectileMovement; }

	/** Function to initialize the projectile's velocity in the shoot di
	rection */
	void GrenadeFireInDirection(const FVector& ShootDirection);

	UPROPERTY(BlueprintReadWrite, Category = "Projectile")
	bool bIsAltFire = false;

	// Add this function declaration as well:
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SetAltFireMode(bool bAltFire);

	

};
