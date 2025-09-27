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
		return;
	}

	if (!OwningCharacter)
	{
		return;
	}

	UCharacterMovementComponent* MovementComp = OwningCharacter->GetCharacterMovement();
	if (!MovementComp)
	{
		return;
	}

	FVector OutWallNormal, OutWallRunDirection;
	bool DetectedWall = DetectWall(OutWallNormal, OutWallRunDirection);
	bool bIsFalling = MovementComp->IsFalling();
	bool bIsGrounded = MovementComp->IsMovingOnGround(); // Add this check

	// If a wall is detected and the character is not already wall running, start wall running
	if (DetectedWall && bIsFalling && !IsWallRunning)
	{
		StartWallRun();
	}
	else if (IsWallRunning)
	{
		// Stop wall running if character has reached the ground
		if (bIsGrounded)
		{
			StopWallRun();
			return;
		}

		// If the character is wall running, continue checking if still near the wall
		if (DetectedWall && bIsFalling) // Also check bIsFalling here
		{
			// Continue wall running
			WallNormal = OutWallNormal;
			WallRunDirection = OutWallRunDirection;
			WallRun();
		}
		else
		{
			// If no wall is detected or not falling, stop wall running
			StopWallRun();
		}
	}
}

void UWallRunComponent::StartWallRun()
{
	// Unlock character rotation so camera can look independently
	OwningCharacter->bUseControllerRotationYaw = false;
	OwningCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;

	IsWallRunning = true;

	FVector OutWallNormal, OutWallRunDirection;
	if (DetectWall(OutWallNormal, OutWallRunDirection))
	{
		WallNormal = OutWallNormal;
		WallRunDirection = OutWallRunDirection;

		// Capture the player's initial velocity when the wall run starts
		InitialVelocity = OwningCharacter->GetCharacterMovement()->Velocity;

		// Set a timer to stop wall running after the specified duration
		GetWorld()->GetTimerManager().SetTimer(WallRunTimerHandle, this, &UWallRunComponent::EndWallRun, WallRunDuration, false);

		// Apply the fixed wall run velocity immediately
		WallRun();

		// Reset jump count when wall running begins
		if (AIconoclasmCharacter* IconoclasmChar = Cast<AIconoclasmCharacter>(OwningCharacter))
		{
			IconoclasmChar->ResetJumpCount();
		}
	}
}

void UWallRunComponent::StopWallRun()
{
	IsWallRunning = false;

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

	// Clear the WallRun timer if it's still active
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

