// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelNameTriggerBox.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"

ALevelNameTriggerBox::ALevelNameTriggerBox()
{
    OnActorBeginOverlap.AddDynamic(this, &ALevelNameTriggerBox::OnOverlapBegin);
    bHasTriggered = false;
}

void ALevelNameTriggerBox::BeginPlay()
{
    Super::BeginPlay();
}

void ALevelNameTriggerBox::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
    if (bTriggerOnce && bHasTriggered)
        return;

    // Check if the overlapping actor is the player character
    ACharacter* Character = Cast<ACharacter>(OtherActor);
    if (!Character || !Character->IsPlayerControlled())
        return;

    if (!WidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetClass is not set on TypewriterTrigger!"));
        return;
    }

    bHasTriggered = true;

    // Create and add widget to viewport
    WidgetInstance = CreateWidget<ULevelNameWidget>(GetWorld(), WidgetClass);
    if (WidgetInstance)
    {
        WidgetInstance->AddToViewport(10); // High Z-order to appear on top
        WidgetInstance->StartTypewriter(TextToDisplay, TypingSpeed);

        // Set timer to remove widget
        GetWorld()->GetTimerManager().SetTimer(
            RemoveWidgetTimer,
            this,
            &ALevelNameTriggerBox::RemoveWidget,
            DisplayDuration,
            false
        );
    }
}

void ALevelNameTriggerBox::RemoveWidget()
{
    if (WidgetInstance)
    {
        WidgetInstance->RemoveFromParent();
        WidgetInstance = nullptr;
    }
}