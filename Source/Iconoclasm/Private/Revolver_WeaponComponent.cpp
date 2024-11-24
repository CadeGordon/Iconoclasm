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
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/SphereComponent.h"


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

	if (GunslingerParticle && Character)
	{
		FVector MuzzleLocation = Character->GetActorLocation() + Character->GetControlRotation().RotateVector(MuzzleOffset);
		FRotator CameraRotation = Character->GetControlRotation();

		// Spawn the Niagara system
		NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), GunslingerParticle, MuzzleLocation);

		if (NiagaraComp)
		{
			// Set the initial direction of the Niagara component
			NiagaraComp->SetWorldRotation(CameraRotation);
		}
	}
}

void URevolver_WeaponComponent::AltGunslingerMode()
{
	if (!bCanFireAltGunslinger)
	{
		return; // Prevent firing if the cooldown is active
	}

	bCanFireAltGunslinger = false; // Set to false to trigger cooldown

	HitscanCount = 6; // Set the number of hitscans to fire

	auto FireHitscan = [this]()
		{
			if (HitscanCount <= 0)
			{
				GetWorld()->GetTimerManager().ClearTimer(TimerHandle_HellfireEffect);
				return;
			}

			FVector ImpactLocation;
			PerformHitscan(ImpactLocation);

			if (FireSound != nullptr)
			{
				UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
			}

			if (FireAnimation != nullptr)
			{
				UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
				if (AnimInstance != nullptr)
				{
					AnimInstance->Montage_Play(FireAnimation, 1.f);
				}
			}

			if (AltGunslingerParticle && Character)
			{
				FVector MuzzleLocation = Character->GetActorLocation() + Character->GetControlRotation().RotateVector(MuzzleOffset);
				FRotator CameraRotation = Character->GetControlRotation();

				NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), AltGunslingerParticle, MuzzleLocation);

				if (NiagaraComp)
				{
					NiagaraComp->SetWorldRotation(CameraRotation);
				}
			}

			HitscanCount--;
		};

	GetWorld()->GetTimerManager().SetTimer(TimerHandle_HellfireEffect, FireHitscan, 0.1f, true);

	// Set the cooldown timer
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_AltGunslingerCooldown, [this]()
	{
			bCanFireAltGunslinger = true;
	}, 2.0f, false); // Co
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

			if (HellfireParticle && Character)
			{
				FVector MuzzleLocation = Character->GetActorLocation() + Character->GetControlRotation().RotateVector(MuzzleOffset);
				FRotator CameraRotation = Character->GetControlRotation();

				// Spawn the Niagara system
				NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HellfireParticle, MuzzleLocation);

				if (NiagaraComp)
				{
					// Set the initial direction of the Niagara component
					NiagaraComp->SetWorldRotation(CameraRotation);
				}
			}

			HitscanCount--;
		};

	// Set the timer to call the lambda function every 0.25 seconds
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_AltGunslingerFire, FireHitscan, 0.05f, true);
}

void URevolver_WeaponComponent::AltHellfireMode()
{
	if (!bCanFireAltHellfire)
	{
		return; // Prevent firing if the cooldown is active
	}

	bCanFireAltHellfire = false; // Set to false to trigger cooldown

	HellfireDuration = 5.0f;

	if (AltHellfireParticle && Character)
	{
		FVector MuzzleLocation = Character->GetActorLocation() + Character->GetControlRotation().RotateVector(MuzzleOffset);
		FRotator CameraRotation = Character->GetControlRotation();

		NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), AltHellfireParticle, MuzzleLocation);

		if (NiagaraComp)
		{
			NiagaraComp->SetWorldRotation(CameraRotation);
		}
	}

	auto HellfireEffect = [this]()
		{
			// Logic for continuous Hellfire effect
		};

	GetWorld()->GetTimerManager().SetTimer(TimerHandle_HellfireEffect, HellfireEffect, 0.1f, true, HellfireDuration);

	// Set the cooldown timer
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_AltHellfireCooldown, [this]()
	{
			bCanFireAltHellfire = true;
	}, 3.0f, false); // Cooldown duration
}




