// Fill out your copyright notice in the Description page of Project Settings.



#include "WallRunComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

// Sets default values for this component's properties
UWallRunComponent::UWallRunComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	WallRunSpeed = 10.0f;
	
	DescentRate = 200.0f;

	// ...
}


// Called when the game starts
void UWallRunComponent::BeginPlay()
{
	Super::BeginPlay();
	OwningCharacter = Cast<ACharacter>(GetOwner());

	// ...
	
}


// Called every frame
void UWallRunComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Constantly check if the character can start wall running
	if (WallRunCooldownActive)
	{
		// Do not allow wall running if cooldown is active
		return;
	}

	// Constantly check if the character can start wall running
	FVector OutWallNormal, OutWallRunDirection;
	bool DetectedWall = DetectWall(OutWallNormal, OutWallRunDirection);

	// If a wall is detected and the character is not already wall running, start wall running
	if (DetectedWall && !IsWallRunning)
	{
		StartWallRun();
	}
	else if (IsWallRunning)
	{
		// If the character is wall running, continue checking if still near the wall
		if (DetectedWall)
		{
			// Continue wall running
			WallNormal = OutWallNormal;
			WallRunDirection = OutWallRunDirection;
			FVector WallRunVelocity = WallRunDirection * WallRunSpeed;
			OwningCharacter->LaunchCharacter(WallRunVelocity, false, false);
		}
		else
		{
			// If no wall is detected, stop wall running
			StopWallRun();
		}
	}
}

void UWallRunComponent::StartWallRun()
{
	FVector OutWallNormal, OutWallRunDirection;
	if (DetectWall(OutWallNormal, OutWallRunDirection))
	{
		WallNormal = OutWallNormal;
		WallRunDirection = OutWallRunDirection;

		// Capture the player's initial velocity when the wall run starts
		InitialVelocity = OwningCharacter->GetCharacterMovement()->Velocity;

		// Set a timer to stop wall running after the specified duration
		GetWorld()->GetTimerManager().SetTimer(WallRunTimerHandle, this, &UWallRunComponent::EndWallRun, WallRunDuration, false);

		// Call WallRun function every tick
		OwningCharacter->GetCharacterMovement()->StopMovementImmediately(); // Stop other movement
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UWallRunComponent::WallRun);
	}
}

void UWallRunComponent::StopWallRun()
{
	// Clear timer handle and reset movement mode
	GetWorld()->GetTimerManager().ClearTimer(WallRunTimerHandle);
	OwningCharacter->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);

	// Set the cooldown timer
	WallRunCooldownActive = true;
	GetWorld()->GetTimerManager().SetTimer(WallRunCooldownTimerHandle, this, &UWallRunComponent::ResetWallRunCooldown, WallRunCooldownDuration, false);
}

void UWallRunComponent::WallRun()
{
	// Calculate wall run movement using the initial speed
	FVector HorizontalInitialVelocity = InitialVelocity.ProjectOnTo(WallRunDirection);
	FVector WallRunVelocity = WallRunDirection * WallRunSpeed;

	// Maintain the player's initial speed while wall running
	FVector FinalWallRunVelocity = HorizontalInitialVelocity + WallRunVelocity;

	// Calculate downward velocity
	FVector DownwardVelocity = FVector(0.0f, 0.0f, -DescentRate);

	// Combine wall run, downward velocities, and initial velocity
	FinalWallRunVelocity += DownwardVelocity;

	// Apply the final velocity to the character
	OwningCharacter->LaunchCharacter(FinalWallRunVelocity, false, false);

	// Draw debug line to visualize wall running (optional)
	DrawDebugLine(GetWorld(), OwningCharacter->GetActorLocation(), OwningCharacter->GetActorLocation() + WallRunDirection * 100.0f, FColor::Green, false, 0.1f);

	// Continue wall run if still on the wall
	FVector OutWallNormal, OutWallRunDirection;
	if (DetectWall(OutWallNormal, OutWallRunDirection))
	{
		// Call WallRun function again for the next tick
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UWallRunComponent::WallRun);
	}
	else
	{
		// Stop wall run if no wall is detected
		StopWallRun();
	}
}



void UWallRunComponent::ResetWallRunCooldown()
{
	WallRunCooldownActive = false;
}

bool UWallRunComponent::DetectWall(FVector& OutWallNormal, FVector& OutWallDirection)
{
	// Raycast from the character's position to detect the wall
	FVector Start = OwningCharacter->GetActorLocation();
	FVector ForwardVector = OwningCharacter->GetActorForwardVector();
	FVector RightVector = OwningCharacter->GetActorRightVector();

	FVector EndRight = Start + RightVector * 100.0f;
	FVector EndLeft = Start - RightVector * 100.0f;

	FHitResult HitResultRight;
	FHitResult HitResultLeft;

	// Perform line traces on both sides to detect a wall
	bool bHitRight = GetWorld()->LineTraceSingleByChannel(HitResultRight, Start, EndRight, ECC_Visibility);
	bool bHitLeft = GetWorld()->LineTraceSingleByChannel(HitResultLeft, Start, EndLeft, ECC_Visibility);

	if (bHitRight && HitResultRight.bBlockingHit)
	{
		// Wall detected on the right side
		OutWallNormal = HitResultRight.Normal;

		// Calculate wall run direction
		OutWallDirection = FVector::CrossProduct(OutWallNormal, FVector::UpVector).GetSafeNormal();

		// Verify the direction: if it’s in the reverse direction of the forward vector, invert it
		if (FVector::DotProduct(OutWallDirection, ForwardVector) < 0)
		{
			OutWallDirection *= -1;
		}

		UE_LOG(LogTemp, Warning, TEXT("Wall detected on the right side. Wall normal: %s"), *OutWallNormal.ToString());

		return true;
	}
	else if (bHitLeft && HitResultLeft.bBlockingHit)
	{
		// Wall detected on the left side
		OutWallNormal = HitResultLeft.Normal;

		// Calculate wall run direction
		OutWallDirection = FVector::CrossProduct(OutWallNormal, FVector::UpVector).GetSafeNormal();

		// Verify the direction: if it’s in the reverse direction of the forward vector, invert it
		if (FVector::DotProduct(OutWallDirection, ForwardVector) < 0)
		{
			OutWallDirection *= -1;
		}

		UE_LOG(LogTemp, Warning, TEXT("Wall detected on the left side. Wall normal: %s"), *OutWallNormal.ToString());

		return true;
	}

	return false;
}

void UWallRunComponent::EndWallRun()
{
	StopWallRun();
	
}

