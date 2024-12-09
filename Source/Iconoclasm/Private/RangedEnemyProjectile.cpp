// Fill out your copyright notice in the Description page of Project Settings.


#include "RangedEnemyProjectile.h"

// Sets default values
ARangedEnemyProjectile::ARangedEnemyProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ARangedEnemyProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARangedEnemyProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

