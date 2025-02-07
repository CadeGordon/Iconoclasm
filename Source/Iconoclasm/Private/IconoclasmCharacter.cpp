// Copyright Epic Games, Inc. All Rights Reserved.

#include "IconoclasmCharacter.h"
#include "IconoclasmProjectile.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WallRunComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/SpringArmComponent.h"
#include "TP_WeaponComponent.h"
#include "DashHUD.h"
#include "Components/ProgressBar.h"
#include "Revolver_WeaponComponent.h"
#include "Shotgun_WeaponComponent.h"
#include "WheelHUD.h"


DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AIconoclasmCharacter

AIconoclasmCharacter::AIconoclasmCharacter()
{
	// Character doesnt have a rifle at start
	bHasRifle = false;
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	//Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	//double jump variables
	JumpCount = 0;
	//Dashing Variables
	GroundDash = 10000.0f;
	AirDash = 4000.0f;
	DashCharges = 3;
	DashCooldown = 2.0f;
	CanDash = true;
	IsDashing = false;
	CanDashAgain = true;
	//Slide Varibales
	IsSliding = false;
	SlideSpeed = 2000.0f;
	SlideJumpBoostStrength = 3500.0f;
	GroundSlamStrength = 200000.0f;

	//DashUI
	TargetDashProgress = 1.0f;  // Starts full
	CurrentDashProgress = 1.0f;
	ProgressInterpSpeed = 5.0f; // Adjust this for smoother or faster interpolation
}

void AIconoclasmCharacter::EquipRevolver()
{
	for (int32 i = 0; i < WeaponInventory.Num(); i++)
	{
		if (WeaponInventory[i] && WeaponInventory[i]->IsA(URevolver_WeaponComponent::StaticClass()))
		{
			// Detach the current weapon if any
			if (HasWeaponEquipped())
			{
				WeaponInventory[CurrentWeaponIndex]->DetachFromCharacter();
			}

			// Equip the revolver
			CurrentWeaponIndex = i;
			WeaponInventory[i]->AttachWeapon(this);
			UE_LOG(LogTemp, Warning, TEXT("Revolver equipped!"));

			// Close the weapon wheel after selection
			DisableWeaponWheel();
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Revolver not found in inventory!"));
}

void AIconoclasmCharacter::EquipShotgun()
{
	for (int32 i = 0; i < WeaponInventory.Num(); i++)
	{
		if (WeaponInventory[i] && WeaponInventory[i]->IsA(UShotgun_WeaponComponent::StaticClass()))
		{
			// Detach the current weapon if any
			if (HasWeaponEquipped())
			{
				WeaponInventory[CurrentWeaponIndex]->DetachFromCharacter();
			}

			// Equip the shotgun
			CurrentWeaponIndex = i;
			WeaponInventory[i]->AttachWeapon(this);
			UE_LOG(LogTemp, Warning, TEXT("Shotgun equipped!"));

			// Close the weapon wheel after selection
			DisableWeaponWheel();
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Shotgun not found in inventory!"));
}

void AIconoclasmCharacter::EquipGrenadeLauncher()
{
	for (int32 i = 0; i < WeaponInventory.Num(); i++)
	{
		if (WeaponInventory[i] && WeaponInventory[i]->IsA(UTP_WeaponComponent::StaticClass()))
		{
			// Detach the current weapon if any
			if (HasWeaponEquipped())
			{
				WeaponInventory[CurrentWeaponIndex]->DetachFromCharacter();
			}

			// Equip the grenade launcher
			CurrentWeaponIndex = i;
			WeaponInventory[i]->AttachWeapon(this);
			UE_LOG(LogTemp, Warning, TEXT("Grenade Launcher equipped!"));

			// Close the weapon wheel after selection
			DisableWeaponWheel();
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Grenade Launcher not found in inventory!"));
}

void AIconoclasmCharacter::ToggleWeaponWheel()
{
	if (WeaponWheelWidget)
	{
		APlayerController* PlayerController = Cast<APlayerController>(GetController());
		if (!PlayerController) return;

		bool bIsVisible = WeaponWheelWidget->GetVisibility() == ESlateVisibility::Visible;

		if (bIsVisible)
		{
			// Hide Weapon Wheel
			WeaponWheelWidget->SetVisibility(ESlateVisibility::Hidden);

			// Restore player control
			PlayerController->SetShowMouseCursor(false);
			PlayerController->SetInputMode(FInputModeGameOnly());
		}
		else
		{
			// Show Weapon Wheel
			WeaponWheelWidget->SetVisibility(ESlateVisibility::Visible);

			// Set UI mode to focus on the Weapon Wheel
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(WeaponWheelWidget->TakeWidget());
			PlayerController->SetInputMode(InputMode);

			// Ensure mouse cursor is visible and the UI has focus
			PlayerController->SetShowMouseCursor(true);
			WeaponWheelWidget->SetKeyboardFocus();
		}
	}
}

void AIconoclasmCharacter::DisableWeaponWheel()
{
	if (WeaponWheelWidget)
	{
		WeaponWheelWidget->SetVisibility(ESlateVisibility::Hidden);

		// Restore player input controls
		APlayerController* PlayerController = Cast<APlayerController>(GetController());
		if (PlayerController)
		{
			PlayerController->SetShowMouseCursor(false);
			PlayerController->SetInputMode(FInputModeGameOnly());
		}
	}
}

void AIconoclasmCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	WallRunComponent = FindComponentByClass<UWallRunComponent>();

	// Initialize FOV values
	if (UCameraComponent* FirstPersonCamera = GetFirstPersonCameraComponent())
	{
		// Store the original FOV from the camera
		OriginalFOV = FirstPersonCamera->FieldOfView;
		CurrentFOV = OriginalFOV;  // Set current FOV to the original FOV
		TargetFOV = OriginalFOV;
	}

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	
	// Create and add the DashHUD to the viewport
	if (DashHUDClass)
	{
		DashHUD = Cast<UDashHUD>(CreateWidget<UUserWidget>(GetWorld(), DashHUDClass));
		if (DashHUD)
		{
			DashHUD->AddToViewport();
			// Set the progress bar to full (3 charges)
			UpdateDashProgress();
		}
	}
	
	if (WeaponWheelWidgetClass)
	{
		// Create and add the widget to the viewport
		WeaponWheelWidget = CreateWidget<UWheelHUD>(GetWorld(), WeaponWheelWidgetClass);

		if (WeaponWheelWidget)
		{
			WeaponWheelWidget->AddToViewport(100);
			WeaponWheelWidget->SetVisibility(ESlateVisibility::Hidden);  // Initially hidden
		}
	}

}

void AIconoclasmCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	// Check if the player is on the ground
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		LastSafeLocation = GetActorLocation();
	}

	if (IsSliding) {

		UpdateSlide();

	}

	// Interpolate the current FOV towards the target FOV
	if (UCameraComponent* FirstPersonCamera = GetFirstPersonCameraComponent())
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, InterpSpeed);
		FirstPersonCamera->SetFieldOfView(CurrentFOV);
	}

	// Interpolate the progress bar value
	if (FMath::Abs(CurrentDashProgress - TargetDashProgress) > KINDA_SMALL_NUMBER)
	{
		CurrentDashProgress = FMath::FInterpTo(CurrentDashProgress, TargetDashProgress, DeltaTime, ProgressInterpSpeed);

		// Update the HUD
		if (DashHUD)
		{
			DashHUD->UpdateDashProgress(CurrentDashProgress);
		}
	}

}

//////////////////////////////////////////////////////////////////////////// Input

void AIconoclasmCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AIconoclasmCharacter::DoubleJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AIconoclasmCharacter::SlideJump);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AIconoclasmCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AIconoclasmCharacter::Look);


		EnhancedInputComponent->BindAction(CycleWeaponAction, ETriggerEvent::Triggered, this, &AIconoclasmCharacter::CycleWeapon);

		EnhancedInputComponent->BindAction(MeleeAction, ETriggerEvent::Triggered, this, &AIconoclasmCharacter::PerformMelee);

		EnhancedInputComponent->BindAction(WheelAction, ETriggerEvent::Triggered, this, &AIconoclasmCharacter::ToggleWeaponWheel);
		//EnhancedInputComponent->BindAction(DisableWheelAction, ETriggerEvent::Triggered, this, &AIconoclasmCharacter::DisableWeaponWheel);

	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AIconoclasmCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AIconoclasmCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AIconoclasmCharacter::SetHasRifle(bool bNewHasRifle)
{
	bHasRifle = bNewHasRifle;
}

bool AIconoclasmCharacter::GetHasRifle()
{
	return bHasRifle;
}

void AIconoclasmCharacter::DoubleJump()
{

	if (JumpCount < 2)
	{
		if (JumpCount == 0)
		{
			// First jump using the built-in Jump function
			Jump();
			
		}
		else
		{
			// Second jump - manually set the velocity
			LaunchCharacter(FVector(0, 0, 1400.0f), false, true);
		}

		JumpCount++;
	}
}

void AIconoclasmCharacter::Dash()
{
	if ((CanDash || CanDashAgain) && DashCharges > 0)
	{
		FVector DashDirection = GetLastMovementInputVector().GetSafeNormal();

		if (!DashDirection.IsNearlyZero())
		{
			IsDashingForward = DashDirection.Equals(GetActorForwardVector(), 0.1f);

			IsDashing = true;
			float DashSpeed = GetCharacterMovement()->IsMovingOnGround() ? GroundDash : AirDash;
			GetCharacterMovement()->Velocity = DashDirection * DashSpeed;

			DashCharges--;

			// Set target progress based on charges
			TargetDashProgress = static_cast<float>(DashCharges) / 3.0f;

			StartDashCooldown();
		}

		CanDashAgain = (DashCharges > 0);
	}
}

void AIconoclasmCharacter::StartDashCooldown()
{
	CanDash = false;
	CanDashAgain = false;
	GetWorldTimerManager().SetTimer(DashCooldownTimerHandle, this, &AIconoclasmCharacter::ResetDashCooldown, DashCooldown, false);
}

void AIconoclasmCharacter::ResetDashCooldown()
{
	CanDash = true;

	if (DashCharges < 3)
	{
		DashCharges++;

		// Update the target progress smoothly
		TargetDashProgress = static_cast<float>(DashCharges) / 3.0f;

		GetWorldTimerManager().SetTimer(DashCooldownTimerHandle, this, &AIconoclasmCharacter::ResetDashCooldown, DashCooldown, false);
	}
}

void AIconoclasmCharacter::EndDash()
{
	IsDashing = false;
	
	/*TargetFOV = OriginalFOV;*/
}

void AIconoclasmCharacter::StartSlide()
{
	if (!IsSliding && GetCharacterMovement()->IsMovingOnGround())
	{
		IsSliding = true;

		// Set the character's movement direction
		SlideDirection = GetActorForwardVector();

		
		UpdateSlide();
		TargetFOV = SlideFOV;

		// Adjust the camera position or rotation here
        if (UCameraComponent* FirstPersonCamera = GetFirstPersonCameraComponent())
        {
            // Move the camera down
            FirstPersonCamera->AddRelativeLocation(FVector(0.0f, 0.0f, -50.0f)); // Adjust the Z value as needed
        }
	}

}

void AIconoclasmCharacter::UpdateSlide()
{
	if (IsSliding)
	{
		if (GetCharacterMovement()->IsMovingOnGround())
		{
			// Adjust the slide speed
			SlideSpeed = 5000.0f; 

			// Set the character's velocity directly for smooth movement on the ground
			FVector SlideVelocity = SlideDirection * SlideSpeed;
			GetCharacterMovement()->Velocity = SlideVelocity;

			// Rotate the character based on the controller input
			const FRotator ControlRotation = GetControlRotation();
			const FRotator ControlYawRotation(0, ControlRotation.Yaw, 0);
			SetActorRotation(ControlYawRotation);
		}
		else
		{
			//Can be used to launch player when sliding off a ledge not sure if want to use it
			//// If sliding off a ledge, use LaunchCharacter with a set value
			//FVector LaunchVelocity = SlideDirection * 100.0f; 
			//LaunchCharacter(LaunchVelocity, false, false);
		}

		// Additional logic for updating slide
	}

}

void AIconoclasmCharacter::StopSlide()
{
	if (IsSliding) {
		IsSliding = false;
		GetCharacterMovement()->MaxWalkSpeed = 1600.0f;
		TargetFOV = OriginalFOV;

		// Adjust the camera position or rotation here
		if (UCameraComponent* FirstPersonCamera = GetFirstPersonCameraComponent())
		{
			// Move the camera down
			FirstPersonCamera->AddRelativeLocation(FVector(0.0f, 0.0f, 50.0f)); // Adjust the Z value as needed
		}

		

	}
}

void AIconoclasmCharacter::SlideJump()
{
	if (IsSliding)
	{
		// Perform a boost when jumping while sliding
		FVector LaunchVelocity = FVector(0.0f, 0.0f, 1.0f) * SlideJumpBoostStrength; // Adjust the Z component for upward boost
		LaunchCharacter(LaunchVelocity, false, false);
		StopSlide(); // Stop sliding when jumping
	}
}


void AIconoclasmCharacter::GroundSlam()
{
	// Cancel out the current velocity
	FVector CurrentVelocity = GetCharacterMovement()->Velocity;
	FVector CancelVelocity = FVector(-CurrentVelocity.X, -CurrentVelocity.Y, 0.0f);
	GetCharacterMovement()->Velocity = CancelVelocity;

	// Perform a slam by launching the character straight down
	FVector LaunchVelocity = FVector(0.0f, 0.0f, -1.0f) * GroundSlamStrength; // Adjust the Z component for downward velocity
	LaunchCharacter(LaunchVelocity, true, true); // Set bXYOverride to true to override XY movement

	// Create a collision sphere to detect nearby objects
	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this); // Ignore the player character
	TArray<FHitResult> HitResults;
	FVector SphereLocation = GetActorLocation();
	float SphereRadius = 1000.0f;

	// Perform the collision sphere trace
	bool bHitSomething = UKismetSystemLibrary::SphereTraceMulti(GetWorld(), SphereLocation, SphereLocation, SphereRadius, UEngineTypes::ConvertToTraceType(ECC_WorldDynamic), false, IgnoreActors, EDrawDebugTrace::None, HitResults, true);

	// Apply upward force to objects within the collision sphere
	if (bHitSomething)
	{
		for (const FHitResult& HitResult : HitResults)
		{
			// Check if the hit actor is simulating physics
			UPrimitiveComponent* HitComponent = HitResult.GetComponent();
			if (HitComponent && HitComponent->IsSimulatingPhysics())
			{
				// Apply upward impulse to the hit component
				FVector UpwardImpulse = FVector(0.0f, 0.0f, 2000.0f);
				HitComponent->AddImpulse(UpwardImpulse, NAME_None, true);
			}
		}
	}

	// Draw debug sphere for visualization
	if (bHitSomething)
	{
		DrawDebugSphere(GetWorld(), SphereLocation, SphereRadius, 12, FColor::Red, false, 1.0f, 0, 1.0f);
	}
	
}

void AIconoclasmCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	CanDashAgain = true;
	JumpCount = 0;

	// If there are no dash charges, start recharging them
	if (DashCharges == 0)
	{
		// Start cooldown timer
		StartDashCooldown();
	}
}

void AIconoclasmCharacter::EquipWeapon(UTP_WeaponComponent* Weapon)
{
	if (Weapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("Trying to equip weapon: %s"), *Weapon->GetName());

		// Check if weapon is already in the inventory
		if (!WeaponInventory.Contains(Weapon))
		{
			// Add the weapon to the inventory
			int32 NewWeaponIndex = WeaponInventory.Add(Weapon);
			UE_LOG(LogTemp, Warning, TEXT("Weapon %s added to inventory at index %d"), *Weapon->GetName(), NewWeaponIndex);

			// If no weapon is currently equipped, equip this one
			if (WeaponInventory.Num() == 1)
			{
				CurrentWeaponIndex = NewWeaponIndex;
				Weapon->AttachWeapon(this);
				UE_LOG(LogTemp, Warning, TEXT("Equipped weapon: %s"), *Weapon->GetName());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Weapon %s is already in inventory!"), *Weapon->GetName());
		}
	}
}

void AIconoclasmCharacter::AddWeaponToInventory(UTP_WeaponComponent* Weapon)
{
	if (Weapon)
	{
		WeaponInventory.Add(Weapon);
	}
}

void AIconoclasmCharacter::CycleWeapon()
{
	if (WeaponInventory.Num() > 1)
	{
		// Detach the currently equipped weapon
		UTP_WeaponComponent* CurrentWeapon = WeaponInventory[CurrentWeaponIndex];
		CurrentWeapon->DetachFromCharacter();
		UE_LOG(LogTemp, Warning, TEXT("Detached weapon: %s"), *CurrentWeapon->GetName());

		// Move to the next weapon in the inventory
		CurrentWeaponIndex = (CurrentWeaponIndex + 1) % WeaponInventory.Num();

		// Equip the new weapon
		UTP_WeaponComponent* NewWeapon = WeaponInventory[CurrentWeaponIndex];
		NewWeapon->AttachWeapon(this);
		UE_LOG(LogTemp, Warning, TEXT("Switched to weapon: %s"), *NewWeapon->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No other weapon to cycle to!"));
	}
}

bool AIconoclasmCharacter::HasWeaponEquipped() const
{
	return WeaponInventory.Num() > 0 && WeaponInventory.IsValidIndex(CurrentWeaponIndex);
}

void AIconoclasmCharacter::PerformMelee()
{

	float MeleeDamage = 20.0f;

	// Get the start and end points of the trace
	FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	FVector ForwardVector = FirstPersonCameraComponent->GetForwardVector();
	FVector End = Start + (ForwardVector * MeleeRange);

	// Trace parameters
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // Ignore self

	// Perform the line trace
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams);

	// Visualize the trace for debugging
	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.0f, 0, 1.0f);

	if (bHit)
	{
		AActor* HitActor = HitResult.GetActor();

		if (HitActor)
		{
			// Apply knockback
			UPrimitiveComponent* HitComponent = HitResult.GetComponent();
			if (HitComponent && HitComponent->IsSimulatingPhysics())
			{
				FVector KnockbackDirection = (HitResult.ImpactPoint - Start).GetSafeNormal();
				HitComponent->AddImpulse(KnockbackDirection * KnockbackStrength, NAME_None, true);
			}

			// Apply damage to the hit actor
			UGameplayStatics::ApplyDamage(HitActor, MeleeDamage, GetController(), this, UDamageType::StaticClass());

			// Check if the hit actor has a health component
			UHealthComponent* EnemyHealthComp = HitActor->FindComponentByClass<UHealthComponent>();
			if (EnemyHealthComp)
			{
				// Heal the player by a fraction of the melee damage
				UHealthComponent* PlayerHealthComp = FindComponentByClass<UHealthComponent>();
				if (PlayerHealthComp)
				{
					float HealAmount = MeleeDamage * 0.5f; // Heal 50% of the melee damage dealt
					PlayerHealthComp->Heal(HealAmount);
				}
			}

			// Log the hit
			UE_LOG(LogTemp, Log, TEXT("Melee hit: %s"), *HitActor->GetName());
		}
	}
}

void AIconoclasmCharacter::UpdateDashProgress()
{
	if (DashHUD)
	{
		float Progress = static_cast<float>(DashCharges) / 3.0f;
		DashHUD->UpdateDashProgress(Progress);
	}
}



