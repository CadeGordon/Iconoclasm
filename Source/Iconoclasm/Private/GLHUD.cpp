// Fill out your copyright notice in the Description page of Project Settings.


#include "GLHUD.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"

void UGLHUD::UpdateImageColor(FLinearColor NewColor)
{
    if (ModeImage)
    {
        ModeImage->SetColorAndOpacity(NewColor);
    }
}


