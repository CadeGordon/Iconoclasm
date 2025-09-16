// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EndLevelTrigger.generated.h"

class UBoxComponent;
class UScoreComponent;
class UUserWidget;

UCLASS()
class ICONOCLASM_API AEndLevelTrigger : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEndLevelTrigger();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trigger")
    UBoxComponent* BoxCollider;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, 
                        AActor* OtherActor, 
                        UPrimitiveComponent* OtherComp, 
                        int32 OtherBodyIndex, 
                        bool bFromSweep, 
                        const FHitResult & SweepResult);

    // Widget to display end level score
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI")
    TSubclassOf<UUserWidget> EndLevelWidgetClass;

    UPROPERTY()
    UUserWidget* EndLevelWidget;

    // Store the final score without decay
    int32 FinalScore;

};
