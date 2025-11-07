// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "LevelNameWidget.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API ULevelNameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    // Function to start the typewriter effect
    UFUNCTION(BlueprintCallable, Category = "Typewriter")
    void StartTypewriter(const FText& InText, float InSpeed = 0.05f);

    // Function to stop the effect
    UFUNCTION(BlueprintCallable, Category = "Typewriter")
    void StopTypewriter();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Reference to the text block (bind this in the widget blueprint)
    UPROPERTY(meta = (BindWidget))
    UTextBlock* TypewriterText;

    // Optional: Sound to play per character
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Typewriter")
    USoundBase* TypeSound;

private:
    FString FullText;
    FString CurrentText;
    int32 CurrentCharIndex;
    float TypeSpeed;
    float TimeSinceLastChar;
    bool bIsTyping;
	
};
