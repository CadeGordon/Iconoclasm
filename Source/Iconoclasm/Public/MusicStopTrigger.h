// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MusicStopTrigger.generated.h"

class UBoxComponent;

UCLASS()
class ICONOCLASM_API AMusicStopTrigger : public AActor
{
	GENERATED_BODY()
	
public:
	AMusicStopTrigger();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* TriggerBox;

	UFUNCTION()
	void OnTriggerEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

public:
	// If true, stops music immediately. If false, fades out over FadeOutTime
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music Stop")
	bool bStopImmediately = false;

	// How long to fade out if not stopping immediately
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music Stop")
	float FadeOutTime = 2.0f;

	// If true, trigger can only be used once
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music Stop")
	bool bOneTimeUse = true;

private:
	bool bHasBeenTriggered = false;
};
