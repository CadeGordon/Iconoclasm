// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GruntEnemyCharacter.generated.h"

UCLASS()
class ICONOCLASM_API AGruntEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGruntEnemyCharacter();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DamageAmount = 20.0f;

	
protected:
	// Called when the game starts or when spawned
	//virtual void BeginPlay() override;
	UPROPERTY(EditAnywhere, BLueprintReadWrite, Category = "Health")
	float Health;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	// Custom function to check if AI can jump (not an override)
	bool CanAIJump() const;
};
