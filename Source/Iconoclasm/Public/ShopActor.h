// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopActor.generated.h"

class UBoxComponent;
class UWidgetComponent;

UCLASS()
class ICONOCLASM_API AShopActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AShopActor();

protected:
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UBoxComponent* InteractionBox;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UWidgetComponent* ShopWidgetComponent;

    virtual void BeginPlay() override;

public:
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    void ShowShopMenu(AActor* PlayerActor);
    void HideShopMenu();

};
