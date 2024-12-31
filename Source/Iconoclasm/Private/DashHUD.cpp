// Fill out your copyright notice in the Description page of Project Settings.


#include "DashHUD.h"
#include "Components/ProgressBar.h"

void UDashHUD::UpdateDashProgress(float Progress)
{
    if (DashProgressBar)
    {
        DashProgressBar->SetPercent(Progress);
    }
}
