// Fill out your copyright notice in the Description page of Project Settings.


#include "Revolver_WeaponComponent.h"
#include "IconoclasmCharacter.h"
#include "TP_WeaponComponent.h"
#include "IconoclasmProjectile.h"
#include "GrenadeLauncherHealProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

URevolver_WeaponComponent::URevolver_WeaponComponent()
{
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
	CurrentWeaponMode = ERevolverMode::RevolverMode1;
}

void URevolver_WeaponComponent::Fire()
{

	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	switch (CurrentWeaponMode)
	{
	case ERevolverMode::RevolverMode1:
		GunslingerMode();
		break;
	case ERevolverMode::RevolverMode2:
		HellfireMode();
		break;
	default:
		break;
	}
}

void URevolver_WeaponComponent::AltFire()
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	switch (CurrentWeaponMode)
	{
	case ERevolverMode::RevolverMode1:
		AltGunslingerMode();
		break;
	case ERevolverMode::RevolverMode2:
		AltHellfireMode();
		break;
	default:
		break;
	}
}

void URevolver_WeaponComponent::SwitchFireMode()
{
	// Cycle through the weapon modes
	CurrentWeaponMode = static_cast<ERevolverMode>((static_cast<uint8>(CurrentWeaponMode) + 1) % (static_cast<uint8>(ERevolverMode::RevolverMode2) + 1));
}

void URevolver_WeaponComponent::AttachWeapon(AIconoclasmCharacter* TargetCharacter)
{
	Character = TargetCharacter;

	// Check that the character is valid, and has no rifle yet
	if (Character == nullptr || Character->GetHasRifle())
	{
		return;
	}

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh1P(), AttachmentRules, FName(TEXT("GripPoint")));

	// Show the weapon and re-enable physics and collision
	SetVisibility(true, true);
	SetSimulatePhysics(false);

	// switch bHasRifle so the animation blueprint can switch to another animation set
	Character->SetHasRifle(true);

	// Set up action bindings
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			// Fire
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &URevolver_WeaponComponent::Fire);
			EnhancedInputComponent->BindAction(AltFireAction, ETriggerEvent::Triggered, this, &URevolver_WeaponComponent::AltFire);
			EnhancedInputComponent->BindAction(SwitchFireModeAction, ETriggerEvent::Triggered, this, &URevolver_WeaponComponent::SwitchFireMode);
		}
	}
}

void URevolver_WeaponComponent::DetachFromCharacter()
{
	if (Character)
	{
		// Reset rifle state
		Character->SetHasRifle(false);

		// Clear action bindings
		if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			{
				Subsystem->RemoveMappingContext(FireMappingContext);
			}

			if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
			{
				EnhancedInputComponent->ClearActionBindings();
			}
		}

		// Detach weapon
		DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

		// Hide the weapon and disable physics and collision
		SetVisibility(false, true);
		SetSimulatePhysics(false);
		SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// Clear reference
		Character = nullptr;
	}
}

void URevolver_WeaponComponent::PerformHitscan(FVector& ImpactLocation)
{
	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (PlayerController)
	{
		FVector StartLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
		FRotator CameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
		FVector EndLocation = StartLocation + (CameraRotation.Vector() * 10000.0f);

		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Character);

		if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params))
		{
			ImpactLocation = HitResult.Location;
		}
		else
		{
			ImpactLocation = EndLocation;
		}

		// Draw debug line
		DrawDebugLine(GetWorld(), StartLocation, ImpactLocation, FColor::Red, false, 2.0f, 0, 1.0f);
	}
}

void URevolver_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Character == nullptr)
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(FireMappingContext);
		}
	}
}

void URevolver_WeaponComponent::GunslingerMode()
{
	FVector ImpactLocation;
	PerformHitscan(ImpactLocation);

	// Play fire sound
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
	}

	// Play fire animation
	if (FireAnimation != nullptr)
	{
		UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void URevolver_WeaponComponent::AltGunslingerMode()
{
	HitscanCount = 6; // Set the number of hitscans to fire

	auto FireHitscan = [this]()
		{
			if (HitscanCount <= 0)
			{
				GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AltGunslingerFire);
				return;
			}

			FVector ImpactLocation;
			PerformHitscan(ImpactLocation);

			// Play fire sound
			if (FireSound != nullptr)
			{
				UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
			}

			// Play fire animation
			if (FireAnimation != nullptr)
			{
				UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
				if (AnimInstance != nullptr)
				{
					AnimInstance->Montage_Play(FireAnimation, 1.f);
				}
			}

			HitscanCount--;
		};

	// Set the timer to call the lambda function every 0.25 seconds
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_AltGunslingerFire, FireHitscan, 0.1f, true);
}

void URevolver_WeaponComponent::HellfireMode()
{
	HitscanCount = 3; // Set the number of hitscans to fire

	auto FireHitscan = [this]()
		{
			if (HitscanCount <= 0)
			{
				GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AltGunslingerFire);
				return;
			}

			FVector ImpactLocation;
			PerformHitscan(ImpactLocation);

			// Play fire sound
			if (FireSound != nullptr)
			{
				UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
			}

			// Play fire animation
			if (FireAnimation != nullptr)
			{
				UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
				if (AnimInstance != nullptr)
				{
					AnimInstance->Montage_Play(FireAnimation, 1.f);
				}
			}

			HitscanCount--;
		};

	// Set the timer to call the lambda function every 0.25 seconds
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_AltGunslingerFire, FireHitscan, 0.05f, true);
}

void URevolver_WeaponComponent::AltHellfireMode()
{
	HellfireDuration = 5.0f; // Duration in seconds

	auto HellfireEffect = [this]()
		{
			if (Character == nullptr || Character->GetController() == nullptr)
			{
				GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AltHellfire);
				return;
			}

			// Determine the location a few feet away from the player
			APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
			if (PlayerController)
			{
				FVector StartLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
				FRotator CameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
				FVector ForwardVector = CameraRotation.Vector();
				FVector EndLocation = StartLocation + (ForwardVector * 300.0f); // Adjust the distance as needed

				// Draw debug cylinder to visualize the effect
				FVector CylinderEndLocation = EndLocation + (ForwardVector * 500.0f); // Adjust the length of the cylinder as needed
				DrawDebugCylinder(GetWorld(), EndLocation, CylinderEndLocation, 200.0f, 32, FColor::Orange, false, 0.1f);

				// Apply the effect
				TArray<FHitResult> HitResults;
				FCollisionShape CollisionShape;
				CollisionShape.SetCapsule(200.0f, 500.0f); // Adjust the radius and half-height as needed

				FQuat RotationQuat = FRotationMatrix::MakeFromX(ForwardVector).ToQuat();

				bool bHit = GetWorld()->SweepMultiByChannel(HitResults, EndLocation, CylinderEndLocation, RotationQuat, ECC_Visibility, CollisionShape);
				if (bHit)
				{
					for (FHitResult& Hit : HitResults)
					{
						if (AActor* HitActor = Hit.GetActor())
						{
							// Apply damage or any other effect to the hit actors
							// For example: UGameplayStatics::ApplyDamage(HitActor, DamageAmount, PlayerController, this, DamageTypeClass);
						}
					}
				}
			}

			// Reduce the duration and clear the timer if the duration is over
			HellfireDuration -= 0.1f;
			if (HellfireDuration <= 0.0f)
			{
				GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AltHellfire);
			}
		};

	// Set the timer to call the lambda function every 0.1 seconds
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_AltHellfire, HellfireEffect, 0.1f, true);
}
