// Fill out your copyright notice in the Description page of Project Settings.


#include "GrappleHUD.h"
#include "Components/ProgressBar.h"

void UGrappleHUD::UpdateProgressBar(float Progress)
{
    if (GrappleProgressBar)
    {
        GrappleProgressBar->SetPercent(Progress);
    }
}