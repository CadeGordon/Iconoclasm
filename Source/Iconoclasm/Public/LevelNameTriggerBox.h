// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "LevelNameWidget.h"
#include "LevelNameTriggerBox.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API ALevelNameTriggerBox : public ATriggerBox
{
	GENERATED_BODY()
	
public:
    ALevelNameTriggerBox();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor);

    // The text to display
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Typewriter")
    FText TextToDisplay;

    // Speed of typing (seconds per character)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Typewriter")
    float TypingSpeed = 0.05f;

    // Reference to the widget class
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Typewriter")
    TSubclassOf<ULevelNameWidget> WidgetClass;

    // How long to display the widget
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Typewriter")
    float DisplayDuration = 5.0f;

    // Should trigger only once?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Typewriter")
    bool bTriggerOnce = true;

private:
    ULevelNameWidget* WidgetInstance;
    bool bHasTriggered;
    FTimerHandle RemoveWidgetTimer;

    void RemoveWidget();
};
