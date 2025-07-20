// Fill out your copyright notice in the Description page of Project Settings.



#include "WallRunComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "IconoclasmCharacter.h"

// Sets default values for this component's properties
UWallRunComponent::UWallRunComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	WallRunSpeed = 1800.0f;
	
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

	// Make sure we have a valid character and movement component
	if (!OwningCharacter)
	{
		return;
	}

	UCharacterMovementComponent* MovementComp = OwningCharacter->GetCharacterMovement();
	if (!MovementComp)
	{
		return;
	}

	// Constantly check if the character can start wall running
	FVector OutWallNormal, OutWallRunDirection;
	bool DetectedWall = DetectWall(OutWallNormal, OutWallRunDirection);
	bool bIsFalling = MovementComp->IsFalling();

	// If a wall is detected and the character is not already wall running, start wall running
	if (DetectedWall && bIsFalling && !IsWallRunning)
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
			// Apply the fixed wall run velocity every frame
			WallRun();
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
	UCharacterMovementComponent* MovementComp = OwningCharacter->GetCharacterMovement();

	// Get current horizontal speed
	FVector CurrentVel = MovementComp->Velocity;
	CurrentVel.Z = 0;
	float CurrentHorizontalSpeed = CurrentVel.Size();

	// Calculate momentum-based wall run speed
	float MomentumWallRunSpeed = FMath::Max(
		CurrentHorizontalSpeed * MomentumMultiplier,
		BaseWallRunSpeed
	);

	// Cap the speed to prevent excessive wall running
	MomentumWallRunSpeed = FMath::Min(MomentumWallRunSpeed, 4000.0f);

	// Create velocity using the wall run direction and momentum-based speed
	FVector NewVelocity = WallRunDirection * MomentumWallRunSpeed;

	// Apply the constant descent rate
	NewVelocity.Z = -DescentRate;

	// Apply the velocity
	MovementComp->Velocity = NewVelocity;

	// Visual debug
	DrawDebugLine(GetWorld(), OwningCharacter->GetActorLocation(),
		OwningCharacter->GetActorLocation() + WallRunDirection * 100.0f,
		FColor::Green, false, 0.1f);
}

void UWallRunComponent::StopWallRun()
{
	IsWallRunning = false;

	UCharacterMovementComponent* MovementComp = OwningCharacter->GetCharacterMovement();

	// Save current wall run velocity to preserve momentum
	FVector ExitVelocity = MovementComp->Velocity;

	// Reset movement mode
	MovementComp->SetMovementMode(EMovementMode::MOVE_Walking);

	// Preserve momentum with slight reduction
	ExitVelocity *= 0.9f; // Small momentum loss on exit
	MovementComp->Velocity = ExitVelocity;

	// Set cooldown and restore control
	WallRunCooldownActive = true;
	GetWorld()->GetTimerManager().SetTimer(WallRunCooldownTimerHandle, this, &UWallRunComponent::ResetWallRunCooldown, WallRunCooldownDuration, false);

	OwningCharacter->bUseControllerRotationYaw = true;
	OwningCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;

	// Smooth rotation transition
	FRotator ControlRot = OwningCharacter->GetControlRotation();
	FRotator NewYaw = FRotator(0.f, ControlRot.Yaw, 0.f);
	OwningCharacter->SetActorRotation(NewYaw);

	if (GetWorld()->GetTimerManager().IsTimerActive(WallRunTimerHandle))
	{
		GetWorld()->GetTimerManager().ClearTimer(WallRunTimerHandle);
	}
}

void UWallRunComponent::WallRun()
{
	UCharacterMovementComponent* MovementComp = OwningCharacter->GetCharacterMovement();

	// Create a fixed velocity using the wall run direction and speed
	FVector NewVelocity = WallRunDirection * WallRunSpeed;

	// Apply the constant descent rate
	NewVelocity.Z = -DescentRate;

	// Apply the fixed velocity directly
	MovementComp->Velocity = NewVelocity;

	// Visual debug - shows the direction of wall running
	DrawDebugLine(GetWorld(), OwningCharacter->GetActorLocation(),
		OwningCharacter->GetActorLocation() + WallRunDirection * 100.0f,
		FColor::Green, false, 0.1f);
}



void UWallRunComponent::ResetWallRunCooldown()
{
	WallRunCooldownActive = false;
}

bool UWallRunComponent::DetectWall(FVector& OutWallNormal, FVector& OutWallDirection)
{
	FVector Start = OwningCharacter->GetActorLocation();
	FVector RightVector = OwningCharacter->GetActorRightVector();

	FVector EndRight = Start + RightVector * 100.0f;
	FVector EndLeft = Start - RightVector * 100.0f;

	FHitResult HitResultRight, HitResultLeft;

	bool bHitRight = GetWorld()->LineTraceSingleByChannel(HitResultRight, Start, EndRight, ECC_Visibility);
	bool bHitLeft = GetWorld()->LineTraceSingleByChannel(HitResultLeft, Start, EndLeft, ECC_Visibility);

	if (bHitRight && HitResultRight.bBlockingHit)
	{
		OutWallNormal = HitResultRight.Normal;

		// Try both directions and pick the one facing forward
		FVector WallDir1 = FVector::CrossProduct(FVector::UpVector, OutWallNormal).GetSafeNormal();
		FVector WallDir2 = -WallDir1;
		FVector Forward = OwningCharacter->GetActorForwardVector();

		OutWallDirection = (FVector::DotProduct(Forward, WallDir1) > FVector::DotProduct(Forward, WallDir2)) ? WallDir1 : WallDir2;

		return true;
	}
	else if (bHitLeft && HitResultLeft.bBlockingHit)
	{
		OutWallNormal = HitResultLeft.Normal;

		// Try both directions and pick the one facing forward
		FVector WallDir1 = FVector::CrossProduct(FVector::UpVector, OutWallNormal).GetSafeNormal();
		FVector WallDir2 = -WallDir1;
		FVector Forward = OwningCharacter->GetActorForwardVector();

		OutWallDirection = (FVector::DotProduct(Forward, WallDir1) > FVector::DotProduct(Forward, WallDir2)) ? WallDir1 : WallDir2;

		return true;
	}

	return false;
}

void UWallRunComponent::EndWallRun()
{
	StopWallRun();
	
}

void UWallRunComponent::ResetWallJumpCooldown()
{
	bWallJumped = false;
}

