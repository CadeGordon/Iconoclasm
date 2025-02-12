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
#include "RevolverHUD.h"


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

	// Update UI color
	if (RevolverHUD)
	{
		RevolverHUD->UpdateRevolverModeColor(static_cast<uint8>(CurrentWeaponMode));

		// Update progress bar visibility based on the current mode
		if (CurrentWeaponMode == ERevolverMode::RevolverMode1)
		{
			RevolverHUD->SetAltGunslingerCooldownVisibility(true);
			RevolverHUD->SetAltHellfireCooldownVisibility(false);
		}
		else if (CurrentWeaponMode == ERevolverMode::RevolverMode2)
		{
			RevolverHUD->SetAltHellfireCooldownVisibility(true);
			RevolverHUD->SetAltGunslingerCooldownVisibility(false);
		}
	}
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

	// Display RevolverHUD
	if (APlayerController* PC = Cast<APlayerController>(TargetCharacter->GetController()))
	{
		if (!RevolverHUD)
		{
			RevolverHUD = CreateWidget<URevolverHUD>(PC, RevolverHUDClass);
			if (RevolverHUD)
			{
				RevolverHUD->AddToViewport();
			}
		}
		if (RevolverHUD)
		{
			RevolverHUD->SetVisibilityState(true);
			RevolverHUD->UpdateRevolverModeColor(static_cast<uint8>(CurrentWeaponMode));
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

		// Hide RevolverHUD
		if (RevolverHUD)
		{
			RevolverHUD->SetVisibilityState(false);
		}
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
		Params.bTraceComplex = true;

		// Perform the line trace
		if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Pawn, Params))
		{
			AActor* HitActor = HitResult.GetActor();
			if (HitActor && HitActor != Character) // Exclude the player character
			{
				ImpactLocation = HitResult.Location;

				// Log for debugging
				UE_LOG(LogTemp, Warning, TEXT("Hit Actor: %s"), *HitActor->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Ignored Actor: %s"), HitActor ? *HitActor->GetName() : TEXT("None"));
				ImpactLocation = EndLocation; // Default to the end location if ignored
			}
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


	// Perform a line trace to find the hit actor
	FHitResult HitResult;
	FVector StartLocation = Character->GetActorLocation();
	FVector EndLocation = ImpactLocation;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character); // Ignore the player
	QueryParams.bTraceComplex = true;       // Trace against complex collision
	


	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Pawn, QueryParams))
	{
		AActor* HitActor = HitResult.GetActor();

		if (HitActor)
		{
			float DamageAmount = 100.0f; // Set the damage amount
			UGameplayStatics::ApplyDamage(
				HitActor,
				DamageAmount,
				Character->GetController(),
				Character,
				UDamageType::StaticClass()
			);
		}
	}

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

	// Set the cooldown and drain the progress bar to 0% immediately when AltFire is used
	ElapsedTime = 0.0f; // Reset the elapsed time to zero when AltFire is used
	if (RevolverHUD != nullptr)
	{
		RevolverHUD->UpdateAltFireCooldownProgress(0.0f); // Set the progress bar to 0% immediately
	}

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

			// Perform a line trace to detect hit actor and apply damage
			FHitResult HitResult;
			FVector StartLocation = Character->GetActorLocation();
			FVector EndLocation = ImpactLocation;

			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(Character); // Ignore the player
			QueryParams.bTraceComplex = true;       // Trace against complex collision
			

			if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Pawn, QueryParams))
			{
				AActor* HitActor = HitResult.GetActor();
				if (HitActor)
				{
					float DamageAmount = 100.0f; // Set the damage amount
					UGameplayStatics::ApplyDamage(
						HitActor,
						DamageAmount,
						Character->GetController(),
						Character,
						UDamageType::StaticClass()
					);
				}
			}

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

	// Start cooldown and update the progress bar during cooldown
	GetWorld()->GetTimerManager().SetTimer(CooldownTimerHandle, this, &URevolver_WeaponComponent::HandleGunslingerAltCooldown, 0.01f, true);
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

			// Trace for hits using the same logic as PerformHitscan
			FVector StartLocation = Character->GetActorLocation() + Character->GetControlRotation().RotateVector(MuzzleOffset);
			FVector EndLocation = ImpactLocation;
			FHitResult HitResult;

			// Perform a line trace to determine if an actor is hit
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(Character);
			

			if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Pawn, QueryParams))
			{
				// Apply damage if an actor is hit
				if (HitResult.GetActor())
				{
					float DamageAmount = 100.0f; // Adjust the damage amount as needed
					UGameplayStatics::ApplyDamage(
						HitResult.GetActor(),
						DamageAmount,
						Character->GetController(),
						Character,
						UDamageType::StaticClass()
					);
				}
			}

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

			// Spawn particle effect
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
	float FlameRadius = 300.0f; // Radius of the flame effect
	float DamageAmount = 20.0f; // Damage dealt per tick

	// Trigger the particle effect
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

	// Hellfire effect function
	auto HellfireEffect = [this, FlameRadius, DamageAmount]()
		{
			if (Character)
			{
				FVector ForwardVector = Character->GetControlRotation().Vector();
				FVector MuzzleLocation = Character->GetActorLocation() + Character->GetControlRotation().RotateVector(MuzzleOffset);

				float DistanceFromPlayer = 500.0f;
				FVector StartLocation = MuzzleLocation + (ForwardVector * DistanceFromPlayer);
				FVector EndLocation = StartLocation + (ForwardVector * 100.0f);

				DrawDebugCylinder(
					GetWorld(),
					StartLocation,
					EndLocation,
					FlameRadius,
					32,
					FColor::Red,
					false,
					0.1f
				);

				TArray<FHitResult> HitResults;
				FCollisionShape FlameSphere = FCollisionShape::MakeSphere(FlameRadius);

				if (GetWorld()->SweepMultiByChannel(
					HitResults,
					StartLocation,
					EndLocation,
					FQuat::Identity,
					ECC_Visibility,
					FlameSphere))
				{
					for (const FHitResult& Hit : HitResults)
					{
						if (Hit.GetActor() && Hit.GetActor() != Character)
						{
							UGameplayStatics::ApplyDamage(
								Hit.GetActor(),
								DamageAmount,
								Character->GetController(),
								Character,
								UDamageType::StaticClass()
							);
						}
					}
				}
			}
		};

	// Start the Hellfire effect
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_HellfireEffect,
		HellfireEffect,
		0.1f,
		true
	);

	// Stop the Hellfire effect after HellfireDuration
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_HellfireStop,
		[this]()
		{
			GetWorld()->GetTimerManager().ClearTimer(TimerHandle_HellfireEffect);
		},
		HellfireDuration,
		false
	);

	// Set the cooldown timer for AltHellfire
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_AltHellfireCooldown,
		[this]()
		{
			bCanFireAltHellfire = true; // Reset AltHellfire to be used again
		},
		3.0f, // Cooldown duration for AltHellfire
		false
	);

	// Update AltHellfire cooldown progress bar to 0% when activated
	if (RevolverHUD != nullptr)
	{
		RevolverHUD->UpdateAltHellfireCooldownProgress(0.0f); // Set to 0% immediately
	}

	// Start updating the progress for the cooldown (filling up to 100%)
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_AltHellfireProgress,
		this,
		&URevolver_WeaponComponent::HandleHellfireAltCooldown,
		0.01f, // Update the progress every 0.01 seconds
		true
	);
}

void URevolver_WeaponComponent::HandleGunslingerAltCooldown()
{
	if (RevolverHUD == nullptr) return;

	// Update the progress bar
	ElapsedTime += GetWorld()->GetDeltaSeconds();
	float Progress = FMath::Clamp(ElapsedTime / CooldownDuration, 0.0f, 1.0f); // Progress from 0 to 1

	// The progress bar should refill from 0 to 1, so update it accordingly
	RevolverHUD->UpdateAltFireCooldownProgress(Progress); // Refill progress bar from 0 to 100%

	// Once cooldown is complete, reset and allow AltFire again
	if (Progress >= 1.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(CooldownTimerHandle); // Clear the cooldown timer
		bCanFireAltGunslinger = true; // Re-enable AltFire
	}
}

void URevolver_WeaponComponent::HandleHellfireAltCooldown()
{
	if (RevolverHUD == nullptr) return;

	// Increment elapsed time based on cooldown duration
	ElapsedHellfireTime += GetWorld()->GetDeltaSeconds(); // Increment elapsed time

	// Calculate the cooldown progress (0.0f = 0%, 1.0f = 100%)
	float Progress = ElapsedHellfireTime / 3.0f; // Assuming cooldown duration is 3 seconds

	// Update the cooldown progress bar (0% to 100%)
	RevolverHUD->UpdateAltHellfireCooldownProgress(Progress);

	// Once cooldown is done, stop updating the progress and reset the timer
	if (Progress >= 1.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AltHellfireProgress); // Stop progress updates
		ElapsedHellfireTime = 0.0f; // Reset elapsed time
	}
}




