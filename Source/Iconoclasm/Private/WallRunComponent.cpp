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

	WallRunSpeed = 1000.0f;
	
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
	// Unlock character rotation so camera can look independently
	OwningCharacter->bUseControllerRotationYaw = false;
	OwningCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;

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
		//OwningCharacter->GetCharacterMovement()->StopMovementImmediately(); // Stop other movement
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UWallRunComponent::WallRun);
	}
}

void UWallRunComponent::StopWallRun()
{
	UCharacterMovementComponent* MovementComp = OwningCharacter->GetCharacterMovement();

	// Save current wall run velocity before switching movement mode
	FVector ExitVelocity = MovementComp->Velocity;

	// Reset movement mode (back to walking)
	MovementComp->SetMovementMode(EMovementMode::MOVE_Walking);

	// Reapply momentum after mode change (Unreal zeroes it out otherwise)
	MovementComp->Velocity = ExitVelocity;

	// Set the cooldown timer
	WallRunCooldownActive = true;
	GetWorld()->GetTimerManager().SetTimer(WallRunCooldownTimerHandle, this, &UWallRunComponent::ResetWallRunCooldown, WallRunCooldownDuration, false);

	// Re-lock camera and character rotation
	OwningCharacter->bUseControllerRotationYaw = true;
	OwningCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;

	// Optional: Snap character to camera rotation
	FRotator ControlRot = OwningCharacter->GetControlRotation();
	FRotator NewYaw = FRotator(0.f, ControlRot.Yaw, 0.f);
	OwningCharacter->SetActorRotation(NewYaw);
}

void UWallRunComponent::WallRun()
{
	UCharacterMovementComponent* MovementComp = OwningCharacter->GetCharacterMovement();
	FVector CurrentVelocity = MovementComp->Velocity;

	// Project current velocity onto wall run direction
	float ForwardSpeed = FVector::DotProduct(CurrentVelocity, WallRunDirection);

	// If the forward speed is below desired wall run speed, boost it
	if (ForwardSpeed < WallRunSpeed)
	{
		float SpeedBoost = WallRunSpeed - ForwardSpeed;
		CurrentVelocity += WallRunDirection * SpeedBoost;
	}

	// Apply descent rate (Z component)
	CurrentVelocity.Z = -DescentRate;

	// Set the velocity directly instead of using LaunchCharacter
	MovementComp->Velocity = CurrentVelocity;

	DrawDebugLine(GetWorld(), OwningCharacter->GetActorLocation(), OwningCharacter->GetActorLocation() + WallRunDirection * 100.0f, FColor::Green, false, 0.1f);

	FVector OutWallNormal, OutWallRunDirection;
	if (DetectWall(OutWallNormal, OutWallRunDirection))
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UWallRunComponent::WallRun);
	}
	else
	{
		StopWallRun();
	}
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

