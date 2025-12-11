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
#include "PlayerHealthBarHUD.h"
#include "Blueprint/UserWidget.h"
#include "HealthComponent.h"
#include "DeathScreenHUD.h"
#include "GrappleComponent.h"
#include "WeaponSaveGame.h"
#include "WeaponTypes.h"

const FString AIconoclasmCharacter::WeaponSaveSlotName = TEXT("WeaponSaveSlot");
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
	AirDash = 3800.0f;
	DashCharges = 3;
	DashCooldown = 2.0f;
	CanDash = true;
	IsDashing = false;
	CanDashAgain = true;
	//Slide Varibales
	IsSliding = false;
	SlideSpeed = 5000.0f;
	SlideJumpBoostStrength = 2000.0f;
	GroundSlamStrength = 200000.0f;

	// Slide momentum variables
	bIsDeceleratingFromSlide = false;
	CurrentSlideSpeed = 0.0f;
	DefaultWalkSpeed = 1600.0f; // Match your current MaxWalkSpeed
	SlideDecelerationRate = 100.0f; // Adjust this to control how fast momentum fades

	// Ground Slam Jump variables
	bCanSlamJump = false;
	SlamJumpCount = 0;
	SlamJumpWindowTime = 2.5f; // 1 second window
	BaseSlamJumpHeight = 1600.0f; // Base jump height
	SlamJumpHeightMultiplier = 1.5f; // Each successive jump is 1.5x higher

	// Ground Slam Slide variables
	bCanSlamSlide = false;
	SlamSlideWindowTime = 2.5f; // 1 second window

	//DashUI
	TargetDashProgress = 1.0f;  // Starts full
	CurrentDashProgress = 1.0f;
	ProgressInterpSpeed = 5.0f; // Adjust this for smoother or faster interpolation

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	ScoreComponent = CreateDefaultSubobject<UScoreComponent>(TEXT("ScoreComponent"));
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
			// Add to viewport with highest Z-order to ensure it's on top
			WeaponWheelWidget->AddToViewport(9999); // Very high Z-order value

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
			WeaponWheelWidget->AddToViewport(0);
			WeaponWheelWidget->SetVisibility(ESlateVisibility::Hidden);  // Initially hidden
		}
	}

	if (!HealthComponent)
	 { 
		UE_LOG(LogTemp, Error, TEXT("BeginPlay: HealthComponent is NULL in Player!"));
		return;
	 }

	if (HealthWidgetClass)
	{
		HealthWidget = CreateWidget<UPlayerHealthBarHUD>(GetWorld(), HealthWidgetClass);
		if (HealthWidget)
		{
			HealthWidget->AddToViewport();
			HealthWidget->InitializeHealthBar(HealthComponent);
			UE_LOG(LogTemp, Warning, TEXT("Health Widget Created and Initialized"));
		}
	}

	LoadWeaponState();
	
}

void AIconoclasmCharacter::SaveWeaponPickup(UActorComponent* WeaponComponent)
{
	if (!WeaponComponent)
	{
		return;
	}

	UWeaponSaveGame* SaveGameInstance =
		Cast<UWeaponSaveGame>(UGameplayStatics::LoadGameFromSlot(WeaponSaveSlotName, 0));

	if (!SaveGameInstance)
	{
		SaveGameInstance = Cast<UWeaponSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UWeaponSaveGame::StaticClass()));
	}

	if (!SaveGameInstance)
	{
		return;
	}

	EWeaponType WeaponType = EWeaponType::Revolver; // default

	bool bImpulseUnlocked = false;
	bool bDefconUnlocked = false;
	bool bHellfireUnlocked = false;

	// Figure out which weapon this is and pull mode unlocks
	if (URevolver_WeaponComponent* Revolver = Cast<URevolver_WeaponComponent>(WeaponComponent))
	{
		WeaponType = EWeaponType::Revolver;
		bHellfireUnlocked = Revolver->IsHellfireUnlocked();
	}
	else if (UShotgun_WeaponComponent* Shotgun = Cast<UShotgun_WeaponComponent>(WeaponComponent))
	{
		WeaponType = EWeaponType::Shotgun;
		bDefconUnlocked = Shotgun->IsDefconUnlocked();
	}
	else if (UTP_WeaponComponent* GL = Cast<UTP_WeaponComponent>(WeaponComponent))
	{
		WeaponType = EWeaponType::GrenadeLauncher;
		bImpulseUnlocked = GL->IsImpulseUnlocked();
	}

	SaveGameInstance->AddWeapon(WeaponType);
	FWeaponSaveData* Data = SaveGameInstance->GetWeaponData(WeaponType);

	if (Data)
	{
		Data->bHasWeapon = true;
		Data->bImpulseModeUnlocked = bImpulseUnlocked || Data->bImpulseModeUnlocked;
		Data->bDefconModeUnlocked = bDefconUnlocked || Data->bDefconModeUnlocked;
		Data->bHellfireModeUnlocked = bHellfireUnlocked || Data->bHellfireModeUnlocked;
	}

	UGameplayStatics::SaveGameToSlot(SaveGameInstance, WeaponSaveSlotName, 0);
}

void AIconoclasmCharacter::SaveWeaponState()
{
	UWeaponSaveGame* SaveGameInstance =
		Cast<UWeaponSaveGame>(UGameplayStatics::LoadGameFromSlot(WeaponSaveSlotName, 0));

	if (!SaveGameInstance)
	{
		SaveGameInstance = Cast<UWeaponSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UWeaponSaveGame::StaticClass()));
	}

	if (!SaveGameInstance)
	{
		return;
	}

	// Clear and rebuild from current inventory
	SaveGameInstance->UnlockedWeapons.Empty();

	for (UActorComponent* Comp : GetComponents())
	{
		if (!Comp) continue;

		URevolver_WeaponComponent* Revolver = Cast<URevolver_WeaponComponent>(Comp);
		UShotgun_WeaponComponent* Shotgun = Cast<UShotgun_WeaponComponent>(Comp);
		UTP_WeaponComponent* GL = Cast<UTP_WeaponComponent>(Comp);

		if (!Revolver && !Shotgun && !GL)
		{
			continue;
		}

		EWeaponType WeaponType = EWeaponType::Revolver;
		bool bImpulseUnlocked = false;
		bool bDefconUnlocked = false;
		bool bHellfireUnlocked = false;

		if (Revolver)
		{
			WeaponType = EWeaponType::Revolver;
			bHellfireUnlocked = Revolver->IsHellfireUnlocked();
		}
		else if (Shotgun)
		{
			WeaponType = EWeaponType::Shotgun;
			bDefconUnlocked = Shotgun->IsDefconUnlocked();
		}
		else if (GL)
		{
			WeaponType = EWeaponType::GrenadeLauncher;
			bImpulseUnlocked = GL->IsImpulseUnlocked();
		}

		SaveGameInstance->AddWeapon(WeaponType);
		FWeaponSaveData* Data = SaveGameInstance->GetWeaponData(WeaponType);
		if (Data)
		{
			Data->bHasWeapon = true;
			Data->bImpulseModeUnlocked = bImpulseUnlocked;
			Data->bDefconModeUnlocked = bDefconUnlocked;
			Data->bHellfireModeUnlocked = bHellfireUnlocked;
		}
	}

	UGameplayStatics::SaveGameToSlot(SaveGameInstance, WeaponSaveSlotName, 0);
}

void AIconoclasmCharacter::LoadWeaponState()
{
	UWeaponSaveGame* SaveGameInstance =
		Cast<UWeaponSaveGame>(UGameplayStatics::LoadGameFromSlot(WeaponSaveSlotName, 0));

	if (!SaveGameInstance)
	{
		return; // no save yet
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Spawn each owned weapon and apply unlocks
	for (const FWeaponSaveData& Data : SaveGameInstance->UnlockedWeapons)
	{
		if (!Data.bHasWeapon)
		{
			continue;
		}

		TSubclassOf<AActor> WeaponActorClass = nullptr;

		switch (Data.WeaponType)
		{
		case EWeaponType::Revolver:
			WeaponActorClass = RevolverWeaponActorClass;
			break;
		case EWeaponType::Shotgun:
			WeaponActorClass = ShotgunWeaponActorClass;
			break;
		case EWeaponType::GrenadeLauncher:
			WeaponActorClass = GrenadeLauncherWeaponActorClass;
			break;
		default:
			break;
		}

		if (!WeaponActorClass)
		{
			continue;
		}

		// Spawn at player location (can be refined later)
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* WeaponActor = World->SpawnActor<AActor>(
			WeaponActorClass,
			GetActorLocation(),
			GetActorRotation(),
			Params
		);

		if (!WeaponActor)
		{
			continue;
		}

		// Grab the right weapon component and apply unlocks
		if (URevolver_WeaponComponent* Revolver =
			WeaponActor->FindComponentByClass<URevolver_WeaponComponent>())
		{
			Revolver->SetHellfireUnlocked(Data.bHellfireModeUnlocked);
			EquipWeapon(Revolver); // will hide pickup, attach, etc.
		}
		else if (UShotgun_WeaponComponent* Shotgun =
			WeaponActor->FindComponentByClass<UShotgun_WeaponComponent>())
		{
			Shotgun->SetDefconUnlocked(Data.bDefconModeUnlocked);
			EquipWeapon(Shotgun);
		}
		else if (UTP_WeaponComponent* GL =
			WeaponActor->FindComponentByClass<UTP_WeaponComponent>())
		{
			GL->SetImpulseUnlocked(Data.bImpulseModeUnlocked);
			EquipWeapon(GL);
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

	if (IsSliding)
	{
		UpdateSlide();
	}

	// IMPROVED: Handle slide/dash momentum deceleration
	if (bIsDeceleratingFromSlide && !IsSliding && GetCharacterMovement()->IsMovingOnGround())
	{
		// Get current velocity and input
		FVector CurrentVelocity = GetCharacterMovement()->Velocity;
		FVector HorizontalVelocity = FVector(CurrentVelocity.X, CurrentVelocity.Y, 0.0f);
		float CurrentSpeed = HorizontalVelocity.Size();

		FVector InputVector = GetLastMovementInputVector();
		bool bHasInput = !InputVector.IsNearlyZero();

		if (bHasInput)
		{
			// With input: decelerate faster to allow responsive player control
			CurrentSlideSpeed = FMath::FInterpTo(CurrentSlideSpeed, DefaultWalkSpeed, DeltaTime, 8.0f);

			// Apply momentum in the direction of movement
			if (CurrentSlideSpeed > DefaultWalkSpeed)
			{
				FVector Direction = HorizontalVelocity.GetSafeNormal();
				GetCharacterMovement()->Velocity = (Direction * CurrentSlideSpeed) + FVector(0, 0, CurrentVelocity.Z);
				GetCharacterMovement()->MaxWalkSpeed = CurrentSlideSpeed;
			}
			else
			{
				// Back to normal speed
				bIsDeceleratingFromSlide = false;
				CurrentSlideSpeed = DefaultWalkSpeed;
				GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;
			}
		}
		else
		{
			// No input: decelerate faster and stop applying velocity override
			CurrentSlideSpeed = FMath::FInterpTo(CurrentSlideSpeed, 0.0f, DeltaTime, 6.0f);

			// Let the character movement component handle natural deceleration
			// Just set max walk speed back to default
			GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;

			// Stop momentum system when speed is low enough
			if (CurrentSlideSpeed < DefaultWalkSpeed * 0.5f)
			{
				bIsDeceleratingFromSlide = false;
				CurrentSlideSpeed = DefaultWalkSpeed;
			}
		}
	}
	// Ensure max walk speed is reset if deceleration ended
	else if (!bIsDeceleratingFromSlide && !IsSliding && !IsDashing)
	{
		if (GetCharacterMovement()->MaxWalkSpeed != DefaultWalkSpeed)
		{
			GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;
		}
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

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AIconoclasmCharacter::WallJump);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AIconoclasmCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AIconoclasmCharacter::Look);


		EnhancedInputComponent->BindAction(CycleWeaponAction, ETriggerEvent::Triggered, this, &AIconoclasmCharacter::CycleWeapon);

		EnhancedInputComponent->BindAction(MeleeAction, ETriggerEvent::Triggered, this, &AIconoclasmCharacter::PerformMelee);

		//EnhancedInputComponent->BindAction(WheelAction, ETriggerEvent::Triggered, this, &AIconoclasmCharacter::ToggleWeaponWheel);
		//EnhancedInputComponent->BindAction(WheelAction, ETriggerEvent::Completed, this, &AIconoclasmCharacter::DisableWeaponWheel);

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
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	// Check if this is a slam jump
	if (bCanSlamJump && MoveComp->IsMovingOnGround())
	{
		// Calculate progressive jump height
		float JumpHeight = BaseSlamJumpHeight * FMath::Pow(SlamJumpHeightMultiplier, SlamJumpCount);
		LaunchCharacter(FVector(0, 0, JumpHeight), false, true);
		SlamJumpCount++;
		UE_LOG(LogTemp, Warning, TEXT("Slam Jump #%d! Height: %f"), SlamJumpCount, JumpHeight);
		return;
	}
	// === WALL JUMP - MOST POWERFUL WITH MOMENTUM AND CAMERA CONTROL ===
	if (WallRunComponent && WallRunComponent->IsWallRunning)
	{
		FVector WallNormal = WallRunComponent->GetWallNormal();
		FVector WallDirection = WallRunComponent->GetWallRunDirection();
		// Get current velocity to preserve momentum
		FVector CurrentVelocity = MoveComp->Velocity;
		FVector CurrentHorizontalVelocity = FVector(CurrentVelocity.X, CurrentVelocity.Y, 0.0f);
		float CurrentSpeed = CurrentHorizontalVelocity.Size();
		// Get camera direction for player control
		FVector CameraForward = FirstPersonCameraComponent->GetForwardVector();
		FVector CameraRight = FirstPersonCameraComponent->GetRightVector();
		// Project camera forward onto horizontal plane (remove pitch)
		FVector CameraForwardHorizontal = CameraForward;
		CameraForwardHorizontal.Z = 0.0f;
		CameraForwardHorizontal.Normalize();
		// Build powerful wall jump velocity based on camera direction
		FVector WallJumpVelocity = FVector::ZeroVector;
		// 1. Add horizontal velocity based on where player is looking
		float ForwardSpeed = FMath::Max(CurrentSpeed * 1.2f, 1000.0f); // Boost by 20% or minimum
		WallJumpVelocity += CameraForwardHorizontal * ForwardSpeed;
		// 2. Add push away from wall (scaled based on if player is looking away from wall)
		float WallPushDot = FVector::DotProduct(CameraForwardHorizontal, WallNormal);
		float WallPushMultiplier = FMath::Max(WallPushDot, 0.3f); // Minimum 30% push, max 100%
		WallJumpVelocity += WallNormal * (800.0f * WallPushMultiplier);
		// 3. High vertical boost (preserve camera pitch influence)
		float PitchInfluence = FMath::Clamp(CameraForward.Z, -0.5f, 0.8f); // Clamp to prevent extreme angles
		WallJumpVelocity.Z = 1800.0f + (PitchInfluence * 600.0f); // Looking up = higher jump, down = less high
		// Apply the wall jump with momentum preservation
		LaunchCharacter(WallJumpVelocity, false, true);
		// Stop wall running
		WallRunComponent->StopWallRun();
		// Start momentum deceleration system (like dash/slide)
		bIsDeceleratingFromSlide = true;
		CurrentSlideSpeed = ForwardSpeed * 1.3f; // Higher multiplier for wall jump momentum
		// Mark as double jump for kill window
		bLastActionWasDoubleJump = true;
		LastDoubleJumpTime = GetWorld()->GetTimeSeconds();
		// Reset jump count to allow one more air jump
		JumpCount = 1;
		UE_LOG(LogTemp, Warning, TEXT("Wall Jump! Looking direction, Speed: %f, Pitch: %f"),
			ForwardSpeed, PitchInfluence);
		return;
	}
	// === REGULAR JUMP LOGIC ===
	if (MoveComp->IsMovingOnGround())
	{
		bHasLeftGround = false;
	}
	if (MoveComp->IsFalling() && JumpCount == 0 && !bHasLeftGround)
	{
		JumpCount = 1;
		bHasLeftGround = true;
	}
	if (JumpCount < 2)
	{
		if (MoveComp->IsMovingOnGround())
		{
			Jump();
		}
		else
		{
			// DOUBLE JUMP WITH INSTANT DIRECTION CHANGE
			FVector CurrentVelocity = MoveComp->Velocity;
			FVector HorizontalVelocity = FVector(CurrentVelocity.X, CurrentVelocity.Y, 0.0f);
			float CurrentSpeed = HorizontalVelocity.Size();

			// Get player input direction
			FVector InputDirection = GetLastMovementInputVector();

			FVector NewHorizontalVelocity;
			if (!InputDirection.IsNearlyZero())
			{
				// Player is giving input - redirect ALL momentum to input direction
				InputDirection.Z = 0.0f;
				InputDirection.Normalize();

				// Use current speed (or minimum) in the NEW direction - instant redirect!
				float NewSpeed = FMath::Max(CurrentSpeed, 800.0f);
				NewHorizontalVelocity = InputDirection * NewSpeed;
			}
			else
			{
				// No input - preserve current horizontal velocity
				NewHorizontalVelocity = HorizontalVelocity;
			}

			FVector LaunchVelocity = NewHorizontalVelocity + FVector(0, 0, 1000.0f);
			LaunchCharacter(LaunchVelocity, false, true);
			bLastActionWasDoubleJump = true;
			LastDoubleJumpTime = GetWorld()->GetTimeSeconds();
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

			// Get current velocity
			FVector CurrentVelocity = GetCharacterMovement()->Velocity;
			FVector CurrentHorizontalVelocity = FVector(CurrentVelocity.X, CurrentVelocity.Y, 0.0f);
			float CurrentHorizontalSpeed = CurrentHorizontalVelocity.Size();

			// Only apply dash if it would increase speed, otherwise maintain current momentum
			float FinalSpeed = FMath::Max(CurrentHorizontalSpeed, DashSpeed);

			// If in air, reset vertical velocity to stop falling (no upward boost)
			float VerticalVelocity = CurrentVelocity.Z;
			if (!GetCharacterMovement()->IsMovingOnGround())
			{
				// Reset falling velocity to 0
				VerticalVelocity = 0.0f;
			}

			// Apply velocity in dash direction with the higher speed
			GetCharacterMovement()->Velocity = (DashDirection * FinalSpeed) + FVector(0, 0, VerticalVelocity);

			// Store the higher speed for momentum deceleration
			CurrentSlideSpeed = FinalSpeed;
			bIsDeceleratingFromSlide = false; // Reset any ongoing deceleration

			DashCharges--;
			// Set target progress based on charges
			TargetDashProgress = static_cast<float>(DashCharges) / 3.0f;
			LastDashTime = GetWorld()->GetTimeSeconds();
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

	// Only preserve momentum if on ground and moving fast enough
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		FVector HorizontalVel = GetCharacterMovement()->Velocity;
		HorizontalVel.Z = 0;
		float CurrentSpeed = HorizontalVel.Size();

		// Only start momentum deceleration if significantly faster than walk speed
		if (CurrentSpeed > DefaultWalkSpeed * 1.3f)
		{
			bIsDeceleratingFromSlide = true;
			CurrentSlideSpeed = CurrentSpeed * 0.75f; // Reduce by 25% for better feel
		}
		else
		{
			// Not fast enough to preserve momentum
			bIsDeceleratingFromSlide = false;
			CurrentSlideSpeed = DefaultWalkSpeed;
			GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;
		}
	}
}

void AIconoclasmCharacter::StartSlide()
{
	if (!IsSliding && GetCharacterMovement()->IsMovingOnGround())
	{
		IsSliding = true;
		bIsDeceleratingFromSlide = false;

		// Clear the damaged actors list for this new slide
		DamagedActorsThisSlide.Empty();

		// Set the character's movement direction
		SlideDirection = GetActorForwardVector();

		// Check if this is a slam slide (within window and has slam jumps)
		if (bCanSlamSlide && SlamJumpCount > 0)
		{
			// Add bonus speed on top of base slide speed (5000 per slam jump)
			float SlideSpeedBoost = SlideSpeed + (5000.0f * SlamJumpCount);
			CurrentSlideSpeed = SlideSpeedBoost;
			UE_LOG(LogTemp, Warning, TEXT("Slam Slide! Speed boosted by %d slams: %f"), SlamJumpCount, SlideSpeedBoost);

			// Reset slam counters after using the boost
			SlamJumpCount = 0;
			bCanSlamSlide = false;
			GetWorld()->GetTimerManager().ClearTimer(SlamSlideWindowTimerHandle);
		}
		else
		{
			// Normal slide speed
			CurrentSlideSpeed = SlideSpeed;
		}

		UpdateSlide();
		TargetFOV = SlideFOV;

		// Adjust the camera position
		if (UCameraComponent* FirstPersonCamera = GetFirstPersonCameraComponent())
		{
			FirstPersonCamera->AddRelativeLocation(FVector(0.0f, 0.0f, -50.0f));
		}
	}

}

void AIconoclasmCharacter::UpdateSlide()
{
	if (IsSliding)
	{
		if (GetCharacterMovement()->IsMovingOnGround())
		{
			// Don't overwrite CurrentSlideSpeed - it's set in StartSlide()
			// Only set it if it's somehow zero
			if (CurrentSlideSpeed <= 0.0f)
			{
				CurrentSlideSpeed = 5000.0f;
			}

			// Set the character's velocity directly for smooth movement on the ground
			FVector SlideVelocity = SlideDirection * CurrentSlideSpeed;
			GetCharacterMovement()->Velocity = SlideVelocity;

			// Check for enemies to damage while sliding
			CheckSlideCollisions();

			// Rotate the character based on the controller input
			const FRotator ControlRotation = GetControlRotation();
			const FRotator ControlYawRotation(0, ControlRotation.Yaw, 0);
			SetActorRotation(ControlYawRotation);
		}
	}
}

void AIconoclasmCharacter::CheckSlideCollisions()
{
	// Perform a sphere sweep in front of the character
	FVector Start = GetActorLocation();
	FVector End = Start + (SlideDirection * 100.0f); // Check 100 units ahead

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	TArray<FHitResult> HitResults;
	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn, // Collision channel for pawns
		FCollisionShape::MakeSphere(80.0f), // Collision radius
		QueryParams
	);

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && !DamagedActorsThisSlide.Contains(HitActor))
			{
				// Check if the actor has a HealthComponent
				if (UHealthComponent* HealthComp = HitActor->FindComponentByClass<UHealthComponent>())
				{
					// Don't damage yourself or dead enemies
					if (HitActor != this && !HealthComp->IsDead())
					{
						ApplySlideDamage(HitActor, Hit);
						DamagedActorsThisSlide.Add(HitActor);
					}
				}
			}
		}
	}
}

void AIconoclasmCharacter::ApplySlideDamage(AActor* Enemy, const FHitResult& Hit)
{
	if (!Enemy) return;

	// Calculate damage (bonus damage for slam slides based on speed)
	float SpeedMultiplier = FMath::Clamp(CurrentSlideSpeed / SlideSpeed, 1.0f, 3.0f);
	float FinalDamage = SlideDamage * SpeedMultiplier;

	// Apply damage using Unreal's damage system
	// This will trigger the HealthComponent's HandleTakeAnyDamage and award points
	UGameplayStatics::ApplyDamage(
		Enemy,
		FinalDamage,
		GetController(),
		this,
		UDamageType::StaticClass()
	);

	UE_LOG(LogTemp, Warning, TEXT("Slide Hit Enemy: %s for %f damage!"), *Enemy->GetName(), FinalDamage);
}

void AIconoclasmCharacter::StopSlide()
{
	if (IsSliding)
	{
		IsSliding = false;
		DamagedActorsThisSlide.Empty();

		// No momentum preservation - instant stop
		bIsDeceleratingFromSlide = false;
		CurrentSlideSpeed = DefaultWalkSpeed;
		GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;

		TargetFOV = OriginalFOV;

		if (UCameraComponent* FirstPersonCamera = GetFirstPersonCameraComponent())
		{
			FirstPersonCamera->AddRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
		}
	}
}

void AIconoclasmCharacter::SlideJump()
{
	if (IsSliding)
	{
		IsSliding = false;

		// Clear damaged actors list when slide ends
		DamagedActorsThisSlide.Empty();

		// NEW: Start momentum deceleration instead of instant stop
		bIsDeceleratingFromSlide = true;
		// CurrentSlideSpeed retains its current value and will gradually decrease

		TargetFOV = OriginalFOV;

		// Adjust the camera position back
		if (UCameraComponent* FirstPersonCamera = GetFirstPersonCameraComponent())
		{
			FirstPersonCamera->AddRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
		}
	}
}


void AIconoclasmCharacter::GroundSlam()
{
	// Cancel out the current velocity
	FVector CurrentVelocity = GetCharacterMovement()->Velocity;
	FVector CancelVelocity = FVector(-CurrentVelocity.X, -CurrentVelocity.Y, 0.0f);
	GetCharacterMovement()->Velocity = CancelVelocity;

	// Perform a slam by launching the character straight down
	FVector LaunchVelocity = FVector(0.0f, 0.0f, -1.0f) * GroundSlamStrength;
	LaunchCharacter(LaunchVelocity, true, true);

	// Set up timer to check for ground impact
	GetWorld()->GetTimerManager().SetTimer(
		GroundSlamTimerHandle,
		this,
		&AIconoclasmCharacter::CheckGroundSlamImpact,
		0.1f, // Check every 0.1 seconds
		true  // Loop
	);
}

void AIconoclasmCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	CanDashAgain = true;
	JumpCount = 0;
	bHasLeftGround = false;
	bCanWallJump = true;
	WallJumpCount = 0;

	// Check if landing with significant momentum
	FVector HorizontalVel = GetCharacterMovement()->Velocity;
	HorizontalVel.Z = 0;
	float LandingSpeed = HorizontalVel.Size();

	// Only preserve momentum if landing speed is significantly above walk speed
	if (LandingSpeed > DefaultWalkSpeed * 1.5f)
	{
		bIsDeceleratingFromSlide = true;
		CurrentSlideSpeed = LandingSpeed * 0.8f; // Reduce by 20% on landing
	}
	else
	{
		// Normal landing - no momentum preservation
		bIsDeceleratingFromSlide = false;
		CurrentSlideSpeed = DefaultWalkSpeed;
		GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;
	}

	// If there are no dash charges, start recharging them
	if (DashCharges == 0)
	{
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
			// Hide the ground pickup before adding to inventory
			AActor* WeaponOwner = Weapon->GetOwner();
			if (WeaponOwner && !WeaponOwner->IsA<AIconoclasmCharacter>())
			{
				WeaponOwner->SetActorHiddenInGame(true);
				WeaponOwner->SetActorEnableCollision(false);
				UE_LOG(LogTemp, Warning, TEXT("Hidden weapon pickup %s from ground"), *Weapon->GetName());
			}

			// Unequip current weapon if one is equipped
			if (CurrentWeaponIndex >= 0 && CurrentWeaponIndex < WeaponInventory.Num())
			{
				WeaponInventory[CurrentWeaponIndex]->DetachFromCharacter();
				if (WeaponInventory[CurrentWeaponIndex]->GetOwner())
				{
					WeaponInventory[CurrentWeaponIndex]->GetOwner()->SetActorHiddenInGame(true);
				}
				UE_LOG(LogTemp, Warning, TEXT("Unequipped previous weapon"));
			}

			// Add the weapon to the inventory
			int32 NewWeaponIndex = WeaponInventory.Add(Weapon);
			UE_LOG(LogTemp, Warning, TEXT("Weapon %s added to inventory at index %d"), *Weapon->GetName(), NewWeaponIndex);

			// Always equip the newly picked up weapon
			CurrentWeaponIndex = NewWeaponIndex;

			// Make sure the weapon actor is visible when equipping
			if (Weapon->GetOwner())
			{
				Weapon->GetOwner()->SetActorHiddenInGame(false);
			}

			Weapon->AttachWeapon(this);
			UE_LOG(LogTemp, Warning, TEXT("Equipped weapon: %s"), *Weapon->GetName());

			//  NEW: Save this pickup to the save slot
			SaveWeaponPickup(Weapon);
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

		// Hide the weapon pickup from the ground (only if it's not already attached to a character)
		AActor* WeaponOwner = Weapon->GetOwner();
		if (WeaponOwner && !WeaponOwner->IsA<AIconoclasmCharacter>())
		{
			WeaponOwner->SetActorHiddenInGame(true);
			WeaponOwner->SetActorEnableCollision(false);
			UE_LOG(LogTemp, Warning, TEXT("Hidden weapon pickup %s from ground"), *Weapon->GetName());
		}

		//  NEW: Save as soon as it’s added to inventory
		SaveWeaponPickup(Weapon);
	}
}

void AIconoclasmCharacter::CycleWeapon()
{
	if (WeaponInventory.Num() > 1)
	{
		// Detach the currently equipped weapon
		UTP_WeaponComponent* CurrentWeapon = WeaponInventory[CurrentWeaponIndex];
		CurrentWeapon->DetachFromCharacter();

		// Hide the current weapon's actor
		if (CurrentWeapon->GetOwner())
		{
			CurrentWeapon->GetOwner()->SetActorHiddenInGame(true);
		}

		UE_LOG(LogTemp, Warning, TEXT("Detached weapon: %s"), *CurrentWeapon->GetName());

		// Move to the next weapon in the inventory
		CurrentWeaponIndex = (CurrentWeaponIndex + 1) % WeaponInventory.Num();

		// Equip the new weapon
		UTP_WeaponComponent* NewWeapon = WeaponInventory[CurrentWeaponIndex];

		// Make the new weapon's actor visible
		if (NewWeapon->GetOwner())
		{
			NewWeapon->GetOwner()->SetActorHiddenInGame(false);
		}

		NewWeapon->AttachWeapon(this);
		UE_LOG(LogTemp, Warning, TEXT("Switched to weapon: %s"), *NewWeapon->GetName());

		// Mark cycle weapon action for scoring
		bLastActionWasCycleWeapon = true;
		LastCycleWeaponTime = GetWorld()->GetTimeSeconds();
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
	if (!bCanMelee)
	{
		UE_LOG(LogTemp, Warning, TEXT("Melee is on cooldown."));
		return;
	}

	// Check if we're holding a grappled enemy first
	UGrappleComponent* GrappleComp = FindComponentByClass<UGrappleComponent>();
	if (GrappleComp && GrappleComp->IsHoldingEnemy())
	{
		// Execute the grabbed enemy
		GrappleComp->ExecuteGrabbedEnemy();

		// Heal the player for executing an enemy
		UHealthComponent* PlayerHealthComp = FindComponentByClass<UHealthComponent>();
		if (PlayerHealthComp)
		{
			float HealAmount = 50.0f; // Adjust this value as needed
			PlayerHealthComp->Heal(HealAmount);
			UE_LOG(LogTemp, Log, TEXT("Enemy executed! Healed for %f"), HealAmount);
		}

		// Start cooldown for execution
		bCanMelee = false;
		GetWorldTimerManager().SetTimer(MeleeCooldownTimerHandle, [this]()
			{
				bCanMelee = true;
				UE_LOG(LogTemp, Log, TEXT("Melee cooldown reset."));
			}, MeleeCooldownDuration, false);

		bLastAttackWasMelee = true;
		return; // Exit early, don't do regular melee
	}

	// === Regular melee logic if not holding an enemy ===

	// Start cooldown
	bCanMelee = false;
	GetWorldTimerManager().SetTimer(MeleeCooldownTimerHandle, [this]()
		{
			bCanMelee = true;
			UE_LOG(LogTemp, Log, TEXT("Melee cooldown reset."));
		}, MeleeCooldownDuration, false);

	float MeleeDamage = 1000000.0f;

	FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	FVector ForwardVector = FirstPersonCameraComponent->GetForwardVector();
	FVector End = Start + (ForwardVector * MeleeRange);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, QueryParams);

	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.0f, 0, 1.0f);

	if (bHit)
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor)
		{
			UPrimitiveComponent* HitComponent = HitResult.GetComponent();
			if (HitComponent && HitComponent->IsSimulatingPhysics())
			{
				FVector KnockbackDirection = (HitResult.ImpactPoint - Start).GetSafeNormal();
				HitComponent->AddImpulse(KnockbackDirection * KnockbackStrength, NAME_None, true);
			}

			bLastAttackWasMelee = true;
			UGameplayStatics::ApplyDamage(HitActor, MeleeDamage, GetController(), this, UDamageType::StaticClass());

			UHealthComponent* EnemyHealthComp = HitActor->FindComponentByClass<UHealthComponent>();
			if (EnemyHealthComp)
			{
				UHealthComponent* PlayerHealthComp = FindComponentByClass<UHealthComponent>();
				if (PlayerHealthComp)
				{
					float HealAmount = MeleeDamage * 0.5f;
					PlayerHealthComp->Heal(HealAmount);
				}
			}

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

void AIconoclasmCharacter::ShowDeathScreen()
{
	if (DeathScreenWidgetClass)
	{
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (PC)
		{
			//  Check if the widget already exists
			if (!DeathScreenHUD)
			{
				DeathScreenHUD = CreateWidget<UDeathScreenHUD>(PC, DeathScreenWidgetClass);
			}

			//  Make sure it's added to the viewport
			if (DeathScreenHUD && !DeathScreenHUD->IsInViewport())
			{
				DeathScreenHUD->AddToViewport();

				// Pause the game
				UGameplayStatics::SetGamePaused(GetWorld(), true);

				// Set input mode to UI only
				PC->SetInputMode(FInputModeUIOnly());
				PC->SetShowMouseCursor(true);
			}
		}
	}
}

void AIconoclasmCharacter::SetCheckpointLocation(FVector NewLocation)
{
	CheckpointLocation = NewLocation;
	bCheckpointActive = true;
}

FVector AIconoclasmCharacter::GetCheckpointLocation() const
{
	return CheckpointLocation;
}

bool AIconoclasmCharacter::IsCheckpointActive() const
{
	return bCheckpointActive;

}

void AIconoclasmCharacter::RestoreFullHealth()
{
	UHealthComponent* HealthComp = FindComponentByClass<UHealthComponent>();
	if (HealthComp)
	{
		HealthComp->Heal(HealthComp->GetMaxHealth());
	}
}

UHealthComponent* AIconoclasmCharacter::GetHealthComponent() const
{
	return FindComponentByClass<UHealthComponent>();
}

void AIconoclasmCharacter::ResetJumpCount()
{
	JumpCount = 0;
	bHasLeftGround = false;
}


void AIconoclasmCharacter::CheckGroundSlamImpact()
{
	if (GetCharacterMovement()->IsMovingOnGround() ||
		GetCharacterMovement()->IsFalling() == false)
	{
		// Stop the ground slam timer
		GetWorld()->GetTimerManager().ClearTimer(GroundSlamTimerHandle);

		// Enable slam jump window
		bCanSlamJump = true;
		GetWorld()->GetTimerManager().SetTimer(
			SlamJumpWindowTimerHandle,
			this,
			&AIconoclasmCharacter::ResetSlamJumpWindow,
			SlamJumpWindowTime,
			false
		);

		// Enable slam slide window
		bCanSlamSlide = true;
		GetWorld()->GetTimerManager().SetTimer(
			SlamSlideWindowTimerHandle,
			this,
			&AIconoclasmCharacter::ResetSlamSlideWindow,
			SlamSlideWindowTime,
			false
		);

		// Trace setup
		TArray<AActor*> IgnoreActors;
		IgnoreActors.Add(this);
		TArray<FHitResult> HitResults;
		FVector SphereLocation = GetActorLocation();
		float SphereRadius = 1000.0f;

		bool bHitSomething = UKismetSystemLibrary::SphereTraceMulti(
			GetWorld(),
			SphereLocation,
			SphereLocation,
			SphereRadius,
			UEngineTypes::ConvertToTraceType(ECC_WorldDynamic),
			false,
			IgnoreActors,
			EDrawDebugTrace::None,
			HitResults,
			true
		);

		if (bHitSomething)
		{
			//  Track already damaged actors so we don't hit them multiple times
			TSet<AActor*> DamagedActors;

			for (const FHitResult& HitResult : HitResults)
			{
				AActor* HitActor = HitResult.GetActor();
				if (!HitActor || DamagedActors.Contains(HitActor)) continue;

				DamagedActors.Add(HitActor);

				//  Deal damage
				float DamageAmount = 50.0f; // tweak this as needed
				bLastAttackWasSlam = true;

				UGameplayStatics::ApplyDamage(
					HitActor,
					DamageAmount,
					GetController(),   // Instigator
					this,              // Damage causer
					nullptr            // Damage type
				);

				//  Apply knockback/physics
				if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
				{
					FVector LaunchForce = FVector(0.0f, 0.0f, 2000.0f);
					HitCharacter->LaunchCharacter(LaunchForce, false, true);
				}
				else if (UPrimitiveComponent* HitComponent = HitResult.GetComponent())
				{
					if (HitComponent->IsSimulatingPhysics())
					{
						FVector UpwardImpulse = FVector(0.0f, 0.0f, 2000.0f);
						HitComponent->AddImpulse(UpwardImpulse, NAME_None, true);
					}
				}
			}

			// Debug visualization
			DrawDebugSphere(GetWorld(), SphereLocation, SphereRadius, 12, FColor::Red, false, 1.0f, 0, 1.0f);
		}
	}
}

void AIconoclasmCharacter::WallJump()
{
	// Check if we're in the air and can wall jump
	if (!GetCharacterMovement()->IsFalling() || !bCanWallJump)
	{
		return;
	}

	// Check if we've reached the maximum wall jumps
	if (WallJumpCount >= MaxWallJumps)
	{
		UE_LOG(LogTemp, Warning, TEXT("Maximum wall jumps reached! (%d/%d)"), WallJumpCount, MaxWallJumps);
		return;
	}

	FVector WallNormal;
	if (!CanPerformWallJump(WallNormal))
	{
		return;
	}

	// Calculate launch direction
	// Wall normal points away from wall, so we use it as the backward direction
	FVector UpwardDirection = FVector::UpVector;
	FVector BackwardDirection = WallNormal; // Away from wall

	// Combine upward and backward forces
	FVector LaunchVelocity = (UpwardDirection * WallJumpUpwardForce) +
		(BackwardDirection * WallJumpBackwardForce);

	// Launch the character
	LaunchCharacter(LaunchVelocity, true, true);

	// Increment wall jump count
	WallJumpCount++;

	// Reset jump count to allow another jump/double jump
	JumpCount = 1; // Set to 1 so player can still double jump after wall jump

	// Start cooldown
	bCanWallJump = false;
	GetWorldTimerManager().SetTimer(
		WallJumpCooldownTimerHandle,
		this,
		&AIconoclasmCharacter::ResetWallJumpCooldown,
		WallJumpCooldown,
		false
	);

	// Visual feedback (optional)
	DrawDebugLine(
		GetWorld(),
		GetActorLocation(),
		GetActorLocation() + (LaunchVelocity.GetSafeNormal() * 200.0f),
		FColor::Green,
		false,
		1.0f,
		0,
		3.0f
	);

	UE_LOG(LogTemp, Warning, TEXT("Wall Jump performed! (%d/%d)"), WallJumpCount, MaxWallJumps);
}

bool AIconoclasmCharacter::CanPerformWallJump(FVector& OutWallNormal)
{
	// Get camera forward vector (where player is looking)
	FVector CameraForward = FirstPersonCameraComponent->GetForwardVector();
	FVector Start = GetActorLocation();
	FVector End = Start + (CameraForward * WallCheckDistance);

	// Perform line trace to check for wall
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bHitWall = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	// Debug visualization
	DrawDebugLine(
		GetWorld(),
		Start,
		End,
		bHitWall ? FColor::Green : FColor::Red,
		false,
		0.1f,
		0,
		2.0f
	);

	if (!bHitWall)
	{
		return false;
	}

	// Check if we hit a valid wall (not floor or ceiling)
	OutWallNormal = HitResult.ImpactNormal;
	float WallAngle = FMath::Abs(FVector::DotProduct(OutWallNormal, FVector::UpVector));

	// Wall should be mostly vertical (dot product close to 0 means perpendicular to up vector)
	if (WallAngle > 0.3f) // 0 = perfectly vertical, 1 = horizontal
	{
		return false;
	}

	// Project camera forward onto horizontal plane to check if looking at wall
	// This allows looking up/down while still facing the wall
	FVector CameraForwardHorizontal = CameraForward;
	CameraForwardHorizontal.Z = 0.0f; // Remove vertical component
	CameraForwardHorizontal.Normalize();

	// Check if player is looking roughly perpendicular to the wall (ignoring pitch)
	float LookAngle = FVector::DotProduct(CameraForwardHorizontal, -OutWallNormal);
	float AngleThreshold = FMath::Cos(FMath::DegreesToRadians(WallJumpAngleThreshold));

	if (LookAngle < AngleThreshold)
	{
		return false;
	}

	return true;
}

void AIconoclasmCharacter::ResetWallJumpCooldown()
{
	bCanWallJump = true;
}

// New helper functions - add these to your character class:
void AIconoclasmCharacter::ResetSlamJumpWindow()
{
	bCanSlamJump = false;
	SlamJumpCount = 0; // Reset the counter when window expires
}

void AIconoclasmCharacter::ResetSlamSlideWindow()
{
	bCanSlamSlide = false;
}