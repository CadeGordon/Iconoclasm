// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DashHUD.generated.h"

/**
 * 
 */
UCLASS()
class ICONOCLASM_API UDashHUD : public UUserWidget
{
	GENERATED_BODY()
	
public:
    // Updates the progress bar with the given value (0.0f to 1.0f)
    UFUNCTION(BlueprintCallable, Category = "GrappleHUD")
    void UpdateDashProgress(float Progress);

public:
    // Reference to the progress bar in the UI
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* DashProgressBar;
};
