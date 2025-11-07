// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HealthPack.generated.h"

UCLASS()
class ICONOCLASM_API AHealthPack : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHealthPack();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
    UPROPERTY(EditAnywhere, Category = "Health")
    float HealAmount = 25.0f; // Adjust as needed

    UPROPERTY(EditAnywhere, Category = "Components")
    class USphereComponent* CollisionComponent;

    UPROPERTY(EditAnywhere, Category = "Components")
    class UStaticMeshComponent* MeshComponent;

    UPROPERTY(EditAnywhere, Category = "Movement")
    float PullSpeed = 500.0f; // Speed at which it moves toward the player

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);

    FTimerHandle DestroyTimerHandle;

    void DestroyHealthPack();

};
