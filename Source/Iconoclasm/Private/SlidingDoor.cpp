// Fill out your copyright notice in the Description page of Project Settings.


#include "SlidingDoor.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"

ASlidingDoor::ASlidingDoor()
{
    PrimaryActorTick.bCanEverTick = true;

    DoorRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorRoot"));
    RootComponent = DoorRoot;

    DoorFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorFrame"));
    DoorFrame->SetupAttachment(DoorRoot);

    LeftDoor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDoor"));
    LeftDoor->SetupAttachment(DoorRoot);
    LeftDoor->SetRelativeLocation(FVector(0.0f, -50.0f, 0.0f));

    RightDoor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDoor"));
    RightDoor->SetupAttachment(DoorRoot);
    RightDoor->SetRelativeLocation(FVector(0.0f, 50.0f, 0.0f));

    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    TriggerBox->SetupAttachment(DoorRoot);
    TriggerBox->SetBoxExtent(FVector(200.0f, 200.0f, 100.0f));
    TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
}

void ASlidingDoor::BeginPlay()
{
    Super::BeginPlay();

    LeftDoorStartPos = LeftDoor->GetRelativeLocation();
    RightDoorStartPos = RightDoor->GetRelativeLocation();

    UE_LOG(LogTemp, Warning, TEXT("Door BeginPlay - Left Start: %s, Right Start: %s"),
        *LeftDoorStartPos.ToString(), *RightDoorStartPos.ToString());

    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ASlidingDoor::OnTriggerBeginOverlap);
    TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ASlidingDoor::OnTriggerEndOverlap);

    FOnTimelineFloat TimelineProgress;
    TimelineProgress.BindUFunction(this, FName("UpdateDoorPosition"));

    if (OpenCurve)
    {
        DoorTimeline.AddInterpFloat(OpenCurve, TimelineProgress);
        UE_LOG(LogTemp, Warning, TEXT("Door using custom curve"));
    }
    else
    {
        UCurveFloat* DefaultCurve = NewObject<UCurveFloat>();
        DefaultCurve->FloatCurve.AddKey(0.0f, 0.0f);
        DefaultCurve->FloatCurve.AddKey(1.0f, 1.0f);
        DoorTimeline.AddInterpFloat(DefaultCurve, TimelineProgress);
        UE_LOG(LogTemp, Warning, TEXT("Door using default linear curve"));
    }

    FOnTimelineEvent TimelineFinished;
    TimelineFinished.BindUFunction(this, FName("OnDoorTimelineFinished"));
    DoorTimeline.SetTimelineFinishedFunc(TimelineFinished);

    DoorTimeline.SetPlayRate(OpenSpeed);
    DoorTimeline.SetLooping(false);

    // Set initial lock state
    if (bStartsLocked)
    {
        LockDoor();
    }
}

void ASlidingDoor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    DoorTimeline.TickTimeline(DeltaTime);
}

void ASlidingDoor::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    UE_LOG(LogTemp, Warning, TEXT("Door Overlap Begin - Actor: %s"), *OtherActor->GetName());

    if (OtherActor && OtherActor->IsA(ACharacter::StaticClass()))
    {
        PlayersInTrigger++;
        UE_LOG(LogTemp, Warning, TEXT("Player detected! Players in trigger: %d, Locked: %s"),
            PlayersInTrigger, bIsLocked ? TEXT("YES") : TEXT("NO"));

        if (PlayersInTrigger > 0 && !bIsOpen && !bIsLocked)
        {
            UE_LOG(LogTemp, Warning, TEXT("Opening door!"));
            OpenDoor();
        }
        else if (bIsLocked)
        {
            UE_LOG(LogTemp, Warning, TEXT("Door is locked! Cannot open."));
        }
    }
}

void ASlidingDoor::OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    UE_LOG(LogTemp, Warning, TEXT("Door Overlap End - Actor: %s"), *OtherActor->GetName());

    if (OtherActor && OtherActor->IsA(ACharacter::StaticClass()))
    {
        PlayersInTrigger--;
        UE_LOG(LogTemp, Warning, TEXT("Player left! Players in trigger: %d"), PlayersInTrigger);

        if (PlayersInTrigger <= 0 && bIsOpen && !bIsLocked)
        {
            PlayersInTrigger = 0;
            UE_LOG(LogTemp, Warning, TEXT("Closing door!"));
            CloseDoor();
        }
    }
}

void ASlidingDoor::UpdateDoorPosition(float Value)
{
    FVector LeftTargetPos = LeftDoorStartPos + FVector(0.0f, -OpenDistance * Value, 0.0f);
    FVector RightTargetPos = RightDoorStartPos + FVector(0.0f, OpenDistance * Value, 0.0f);

    LeftDoor->SetRelativeLocation(LeftTargetPos);
    RightDoor->SetRelativeLocation(RightTargetPos);

    static int DebugCounter = 0;
    if (DebugCounter++ % 10 == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("UpdateDoorPosition - Value: %f, Left: %s, Right: %s"),
            Value, *LeftTargetPos.ToString(), *RightTargetPos.ToString());
    }
}

void ASlidingDoor::OnDoorTimelineFinished()
{
    UE_LOG(LogTemp, Warning, TEXT("Door timeline finished!"));
}

void ASlidingDoor::OpenDoor()
{
    if (bIsLocked)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot open door - it is locked!"));
        return;
    }

    bIsOpen = true;
    DoorTimeline.PlayFromStart();
    UE_LOG(LogTemp, Warning, TEXT("OpenDoor called - Timeline playing"));
}

void ASlidingDoor::CloseDoor()
{
    bIsOpen = false;
    DoorTimeline.Reverse();
    UE_LOG(LogTemp, Warning, TEXT("CloseDoor called - Timeline reversing"));
}

void ASlidingDoor::LockDoor()
{
    bIsLocked = true;
    UE_LOG(LogTemp, Warning, TEXT("Door is now LOCKED"));
}

void ASlidingDoor::UnlockDoor()
{
    bIsLocked = false;
    UE_LOG(LogTemp, Warning, TEXT("Door is now UNLOCKED"));

    // If player is in trigger, open the door
    if (PlayersInTrigger > 0 && !bIsOpen)
    {
        OpenDoor();
    }
}



