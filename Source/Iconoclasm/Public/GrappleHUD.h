// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GrappleHUD.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API UGrappleHUD : public UUserWidget
{
	GENERATED_BODY()

	public:
    // Updates the progress bar with the given value (0.0f to 1.0f)
    UFUNCTION(BlueprintCallable, Category = "GrappleHUD")
    void UpdateProgressBar(float Progress);

protected:
    // Reference to the progress bar in the UI
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* GrappleProgressBar;
	
};
