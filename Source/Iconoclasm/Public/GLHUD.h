// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GLHUD.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API UGLHUD : public UUserWidget
{
	GENERATED_BODY()
	
public:
    // Updates the displayed color for the grenade launcher mode
    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateImageColor(FLinearColor NewColor);

protected:
    // Reference to the image widget in the UI
    UPROPERTY(meta = (BindWidget))
    class UImage* ModeImage;


    // Reference to the image widget in the UI
    UPROPERTY(meta = (BindWidget))
    class UImage* Crosshair;

    
};
