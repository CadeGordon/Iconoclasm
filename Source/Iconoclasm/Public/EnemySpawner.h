// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"

class AGruntEnemyCharacter;

UCLASS()
class ICONOCLASM_API AEnemySpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnemySpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, Category = "Spawner")
    TArray<TSubclassOf<AGruntEnemyCharacter>> EnemyTypes; // Reference to the enemy class to spawn


    UPROPERTY(EditAnywhere, Category = "Spawner")
    int32 NumberOfEnemies = 3; // Number of enemies to spawn

    UPROPERTY(VisibleAnywhere, Category = "Components")
    class UBoxComponent* SpawnTrigger; // Trigger that will detect the player

    UPROPERTY(EditAnywhere, Category = "Spawner")
    TArray<AActor*> SpawnPoints;  // Array of spawn point actors

    UPROPERTY(EditAnywhere, Category = "Spawner")
    TArray<int32> EnemyCounts; // Array of counts for each enemy class


    // Function to handle spawning enemies
    UFUNCTION()
    void OnTriggerEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


private:
    void SpawnEnemies();

};
