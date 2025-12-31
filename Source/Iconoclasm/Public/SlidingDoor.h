// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Components/TimelineComponent.h"
#include "SlidingDoor.generated.h"

UCLASS()
class ICONOCLASM_API ASlidingDoor : public AActor
{
	GENERATED_BODY()
	
public:
    ASlidingDoor();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* DoorRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* DoorFrame;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* LeftDoor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* RightDoor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* TriggerBox;

    // Door settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Settings")
    float OpenDistance = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Settings")
    float OpenSpeed = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Settings")
    UCurveFloat* OpenCurve;

    // Lock system
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Settings")
    bool bStartsLocked = false;

    UPROPERTY(BlueprintReadOnly, Category = "Door Settings")
    bool bIsLocked = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Settings")
    FLinearColor LockedDoorColor = FLinearColor::Red;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Settings")
    FLinearColor UnlockedDoorColor = FLinearColor::White;

    // Internal curve for fallback
    UPROPERTY()
    UCurveFloat* DefaultCurve;

    // Functions
    UFUNCTION(BlueprintCallable, Category = "Door")
    void OpenDoor();

    UFUNCTION(BlueprintCallable, Category = "Door")
    void CloseDoor();

    UFUNCTION(BlueprintCallable, Category = "Door")
    void LockDoor();

    UFUNCTION(BlueprintCallable, Category = "Door")
    void UnlockDoor();

    UFUNCTION(BlueprintCallable, Category = "Door")
    bool IsLocked() const { return bIsLocked; }

private:
    UFUNCTION()
    void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    UFUNCTION()
    void UpdateDoorPosition(float Value);

    UFUNCTION()
    void OnDoorTimelineFinished();

   

    FTimeline DoorTimeline;
    FVector LeftDoorStartPos;
    FVector RightDoorStartPos;

    bool bIsOpen = false;
    int32 PlayersInTrigger = 0;
};
