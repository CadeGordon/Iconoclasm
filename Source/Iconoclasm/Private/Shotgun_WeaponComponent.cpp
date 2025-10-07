// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun_WeaponComponent.h"
#include "IconoclasmCharacter.h"
#include "TP_WeaponComponent.h"
#include "IconoclasmProjectile.h"
#include "GrenadeLauncherHealProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/DamageEvents.h"
#include "BestResultsSubsystem.h"

UShotgun_WeaponComponent::UShotgun_WeaponComponent()
{
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
	CurrentWeaponMode = EShotgunMode::ShotgunMode1;
}

void UShotgun_WeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<AIconoclasmCharacter>(GetOwner());
}

void UShotgun_WeaponComponent::AttachWeapon(AIconoclasmCharacter* TargetCharacter)
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
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &UShotgun_WeaponComponent::Fire);
			EnhancedInputComponent->BindAction(AltFireAction, ETriggerEvent::Triggered, this, &UShotgun_WeaponComponent::AltFire);
			EnhancedInputComponent->BindAction(SwitchFireModeAction, ETriggerEvent::Triggered, this, &UShotgun_WeaponComponent::SwitchFireMode);
		}
	}

	

	if (ShotgunHUDClass)
	{
		ShotgunHUDInstance = CreateWidget<UShotgunHUD>(Character->GetWorld(), ShotgunHUDClass);
		if (ShotgunHUDInstance)
		{
			ShotgunHUDInstance->AddToViewport();
			ShotgunHUDInstance->SetActive(true);

			// Update HUD to reflect the current mode
			if (CurrentWeaponMode == EShotgunMode::ShotgunMode1)
			{
				ShotgunHUDInstance->UpdateMode(true); // Red
			}
			else if (CurrentWeaponMode == EShotgunMode::ShotgunMode2)
			{
				ShotgunHUDInstance->UpdateMode(false); // Blue
			}
		}
	}
}

void UShotgun_WeaponComponent::Fire()
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	switch (CurrentWeaponMode)
	{
	case EShotgunMode::ShotgunMode1:
		if (bCanFireTimeWarp)
		{
			TimeWarpMode();
			bCanFireTimeWarp = false;
			GetWorld()->GetTimerManager().SetTimer(
				TimeWarpCooldownTimer,
				this,
				&UShotgun_WeaponComponent::ResetTimeWarpCooldown,
				TimeWarpCooldown,
				false
			);
		}
		break;
	case EShotgunMode::ShotgunMode2:
		// Check if Defcon mode is unlocked
		if (!bDefconModeUnlocked)
		{
			UE_LOG(LogTemp, Warning, TEXT("Defcon mode is locked!"));
			return;
		}

		if (bCanFireDefcon)
		{
			DefconMode();
			bCanFireDefcon = false;
			GetWorld()->GetTimerManager().SetTimer(
				DefconCooldownTimer,
				this,
				&UShotgun_WeaponComponent::ResetDefconCooldown,
				DefconCooldown,
				false
			);
		}
		break;
	default:
		break;
	}
}

void UShotgun_WeaponComponent::AltFire()
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	switch (CurrentWeaponMode)
	{
	case EShotgunMode::ShotgunMode1:
		AltTimeWarpMode();
		break;
	case EShotgunMode::ShotgunMode2:
		// Check if Defcon mode is unlocked
		if (!bDefconModeUnlocked)
		{
			UE_LOG(LogTemp, Warning, TEXT("Defcon mode is locked!"));
			return;
		}
		AltDefconMode();
		break;
	default:
		break;
	}
}

void UShotgun_WeaponComponent::SwitchFireMode()
{
	// Store the current mode
	EShotgunMode PreviousMode = CurrentWeaponMode;

	// Cycle through the weapon modes
	CurrentWeaponMode = static_cast<EShotgunMode>((static_cast<uint8>(CurrentWeaponMode) + 1) % (static_cast<uint8>(EShotgunMode::ShotgunMode2) + 1));

	// If we switched to Defcon mode but it's locked, switch back
	if (CurrentWeaponMode == EShotgunMode::ShotgunMode2 && !bDefconModeUnlocked)
	{
		CurrentWeaponMode = PreviousMode;
		UE_LOG(LogTemp, Warning, TEXT("Cannot switch to Defcon mode - it is locked!"));
		return;
	}

	// Update the HUD based on the new mode
	if (ShotgunHUDInstance)
	{
		// Assuming ShotgunMode1 corresponds to "Red" and ShotgunMode2 to "Blue"
		bool bIsRedMode = (CurrentWeaponMode == EShotgunMode::ShotgunMode1);
		ShotgunHUDInstance->UpdateMode(bIsRedMode);
	}

	// Update HUD to show the correct progress bar
	if (ShotgunHUDInstance)
	{
		if (CurrentWeaponMode == EShotgunMode::ShotgunMode1)
		{
			ShotgunHUDInstance->ShowAltTimeWarpProgressBar();
		}
		else if (CurrentWeaponMode == EShotgunMode::ShotgunMode2)
		{
			ShotgunHUDInstance->ShowAltDefconProgressBar();
		}
	}

}

void UShotgun_WeaponComponent::PerformHitscan(FVector& ImpactLocation)
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
		Params.bTraceComplex = true;

		if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Pawn, Params))
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

void UShotgun_WeaponComponent::PerformTeleportHitscan(FVector& ImpactLocation, FHitResult& OutHit)
{
	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (PlayerController)
	{
		FVector StartLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
		FRotator CameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
		FVector EndLocation = StartLocation + (CameraRotation.Vector() * 10000.0f);

		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Character);
		Params.bTraceComplex = true;

		if (GetWorld()->LineTraceSingleByChannel(OutHit, StartLocation, EndLocation, ECC_Visibility, Params))
		{
			ImpactLocation = OutHit.Location;
		}
		else
		{
			ImpactLocation = EndLocation;
		}

		// Draw debug line
		DrawDebugLine(GetWorld(), StartLocation, ImpactLocation, FColor::Red, false, 2.0f, 0, 1.0f);
	}
}

void UShotgun_WeaponComponent::PerformPumpHitscan(FVector& StartLocation, FVector& EndLocation, int32 MaxRicochets)
{
	//float MaxRicochets = 8;

	FVector CurrentStartLocation = StartLocation;
	FVector CurrentEndLocation = EndLocation;

	for (int32 i = 0; i <= MaxRicochets; ++i)
	{
		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Character);
		Params.bTraceComplex = true;

		if (GetWorld()->LineTraceSingleByChannel(HitResult, CurrentStartLocation, CurrentEndLocation, ECC_Pawn, Params))
		{
			// Draw debug line for visualization
			DrawDebugLine(GetWorld(), CurrentStartLocation, HitResult.Location, FColor::Red, false, 2.0f, 0, 1.0f);

			// Calculate reflection vector
			FVector IncomingVector = CurrentEndLocation - CurrentStartLocation;
			FVector ReflectedVector = FMath::GetReflectionVector(IncomingVector, HitResult.Normal);

			CurrentStartLocation = HitResult.Location;
			CurrentEndLocation = CurrentStartLocation + (ReflectedVector * 10000.0f);

			// Apply effects or damage to hit targets
			AActor* HitActor = HitResult.GetActor();
			if (HitActor)
			{
				// For example: UGameplayStatics::ApplyDamage(HitActor, DamageAmount, Character->GetController(), this, DamageTypeClass);
			}
		}
		else
		{
			// If no hit, draw line to end location
			DrawDebugLine(GetWorld(), CurrentStartLocation, CurrentEndLocation, FColor::Red, false, 2.0f, 0, 1.0f);
			break;
		}
	}
}

void UShotgun_WeaponComponent::TeleportPlayer(FVector TeleportLocation)
{
	if (Character)
	{
		// Teleport the player
		Character->SetActorLocation(TeleportLocation);
	}
}

void UShotgun_WeaponComponent::DetachFromCharacter()
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

		if (ShotgunHUDInstance)
		{
			ShotgunHUDInstance->SetActive(false);
			ShotgunHUDInstance->RemoveFromViewport();
			ShotgunHUDInstance = nullptr;
		}

		// Clear reference
		Character = nullptr;
	}
}

void UShotgun_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

void UShotgun_WeaponComponent::TimeWarpMode()
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (PlayerController)
	{
		FVector StartLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
		FRotator CameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
		FVector ForwardVector = CameraRotation.Vector();

		// Define the cone angle in degrees
		float ConeAngle = 10.0f;
		int32 MaxRicochets = 3; // Limit ricochets per shot

		for (int32 i = 0; i < 8; ++i) // 8 pellets for the shotgun spread
		{
			FVector CurrentStart = StartLocation;
			FRotator SpreadRotation = CameraRotation;

			// Randomize the spread within the cone
			SpreadRotation.Pitch += FMath::RandRange(-ConeAngle, ConeAngle);
			SpreadRotation.Yaw += FMath::RandRange(-ConeAngle, ConeAngle);

			FVector CurrentDirection = SpreadRotation.Vector();
			int32 CurrentRicochetCount = 0;

			while (CurrentRicochetCount <= MaxRicochets)
			{
				FVector EndLocation = CurrentStart + (CurrentDirection * 10000.0f);
				FHitResult HitResult;

				// Perform the hitscan
				FCollisionQueryParams QueryParams;
				QueryParams.AddIgnoredActor(Character);

				bool bHit = GetWorld()->LineTraceSingleByChannel(
					HitResult,
					CurrentStart,
					EndLocation,
					ECC_Pawn,
					QueryParams
				);

				// Draw debug line for visualization
				DrawDebugLine(GetWorld(), CurrentStart, EndLocation, FColor::Red, false, 2.0f, 0, 1.0f);

				if (bHit)
				{
					// Apply damage to the hit actor
					if (HitResult.GetActor())
					{
						float DamageAmount = 100.0f; // Adjust as needed
						UGameplayStatics::ApplyDamage(
							HitResult.GetActor(),
							DamageAmount,
							Character->GetController(),
							Character,
							UDamageType::StaticClass()
						);
					}

					// Check if we can ricochet
					if (HitResult.Normal != FVector::ZeroVector && CurrentRicochetCount < MaxRicochets)
					{
						// Calculate new direction based on ricochet
						CurrentDirection = FMath::GetReflectionVector(CurrentDirection, HitResult.Normal);
						CurrentStart = HitResult.Location + CurrentDirection * 10.0f; // Slight offset to avoid re-hitting the same surface
						CurrentRicochetCount++;
					}
					else
					{
						// Stop ricocheting if max ricochets are reached or no valid surface is hit
						break;
					}
				}
				else
				{
					// Stop if no hit is detected
					break;
				}
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
	}
}

void UShotgun_WeaponComponent::AltTimeWarpMode()
{
	if (!bCanUseAltTimeWarp || Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (!PlayerController) return;

	FVector StartLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
	FRotator CameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();

	// Calculate knockback direction (opposite of camera forward)
	FVector KnockbackDirection = -CameraRotation.Vector();

	// Set knockback force
	float KnockbackForce = 3000.0f;
	FVector KnockbackVelocity = KnockbackDirection * KnockbackForce;

	// Apply knockback to character
	if (Character->GetCharacterMovement())
	{
		Character->LaunchCharacter(KnockbackVelocity, true, true);
	}

	// Play fire sound
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
	}

	// Activate AltTimeWarp mode if fully charged
	if (AltTimeWarpProgress >= 1.0f)
	{
		AltTimeWarpProgress = 0.0f;

		// Start timer for cooldown updates
		GetWorld()->GetTimerManager().SetTimer(
			AltTimeWarpTimerHandle, this, &UShotgun_WeaponComponent::UpdateCooldowns, 0.1f, true);

		// Show progress bar
		if (ShotgunHUDInstance)
		{
			ShotgunHUDInstance->ShowAltTimeWarpProgressBar();
			ShotgunHUDInstance->UpdateCooldown(0.0f);
		}
	}

	// Mark that this is a BoomStick attack and store the time
	if (AIconoclasmCharacter* IconoChar = Cast<AIconoclasmCharacter>(Character))
	{
		IconoChar->bLastAttackWasBoomStick = true;
		IconoChar->LastBoomStickTime = GetWorld()->GetTimeSeconds();
	}

	// Set cooldown
	bCanUseAltTimeWarp = false;
	GetWorld()->GetTimerManager().SetTimer(
		AltTimeWarpCooldownTimer,
		[this]() { bCanUseAltTimeWarp = true; },
		AltTimeWarpCooldownDuration,
		false
	);
}

void UShotgun_WeaponComponent::DefconMode()
{

	
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	// Try and fire a projectile
	if (ShotgunProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
			const FRotator SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();

			// Get the actual muzzle location from the weapon mesh
			FVector SpawnLocation;
			if (GetOwner()->GetRootComponent())
			{
				// Try to get muzzle socket location first
				if (USkeletalMeshComponent* WeaponMesh = Cast<USkeletalMeshComponent>(GetOwner()->GetRootComponent()))
				{
					if (WeaponMesh->DoesSocketExist(FName("Muzzle")))
					{
						SpawnLocation = WeaponMesh->GetSocketLocation(FName("Muzzle"));
					}
					else
					{
						// Fall back to weapon location + offset
						SpawnLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * MuzzleOffset.X + GetOwner()->GetActorRightVector() * MuzzleOffset.Y + GetOwner()->GetActorUpVector() * MuzzleOffset.Z;
					}
				}
				else
				{
					// Fall back to camera location + offset
					SpawnLocation = PlayerController->PlayerCameraManager->GetCameraLocation() + SpawnRotation.RotateVector(MuzzleOffset);
				}
			}
			else
			{
				// Final fallback
				SpawnLocation = PlayerController->PlayerCameraManager->GetCameraLocation() + SpawnRotation.RotateVector(MuzzleOffset);
			}

			// Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

			// Clean up any destroyed projectiles from the array
			ActiveProjectiles.RemoveAll([](const TWeakObjectPtr<AShotgunProjectile>& ProjectilePtr) {
				return !ProjectilePtr.IsValid();
				});

			// If we have 4 projectiles, destroy the oldest one
			if (ActiveProjectiles.Num() >= 4)
			{
				if (ActiveProjectiles[0].IsValid())
				{
					ActiveProjectiles[0]->Destroy();
				}
				ActiveProjectiles.RemoveAt(0);
			}

			// Spawn the projectile at the muzzle
			AShotgunProjectile* SpawnedProjectile = World->SpawnActor<AShotgunProjectile>(ShotgunProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);

			if (SpawnedProjectile)
			{
				// Set the player character reference to prevent self-damage
				SpawnedProjectile->SetPlayerCharacter(Character);

				// Set the projectile's initial trajectory
				const FVector LaunchDirection = SpawnRotation.Vector();
				SpawnedProjectile->ShotgunFireInDirection(LaunchDirection);

				// Add to our tracking array
				ActiveProjectiles.Add(SpawnedProjectile);
			}
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
}

void UShotgun_WeaponComponent::AltDefconMode()
{
	if (!bCanUseAltDefcon || Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (PlayerController)
	{
		FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
		FRotator CameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();

		// Perform hitscan to find the center point where player is looking
		FVector CenterLocation;
		FVector StartLocation = CameraLocation;
		FVector EndLocation = StartLocation + (CameraRotation.Vector() * 10000.0f);

		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Character);
		Params.bTraceComplex = true;

		if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params))
		{
			CenterLocation = HitResult.Location;
		}
		else
		{
			// If no hit, use a point in front of the player
			CenterLocation = StartLocation + (CameraRotation.Vector() * 1000.0f);
		}

		// Clean up destroyed projectiles from our tracking array
		ActiveProjectiles.RemoveAll([](const TWeakObjectPtr<AShotgunProjectile>& ProjectilePtr) {
			return !ProjectilePtr.IsValid();
			});

		// Gather all active projectiles and move them to center
		TArray<AShotgunProjectile*> ValidProjectiles;
		for (const TWeakObjectPtr<AShotgunProjectile>& ProjectilePtr : ActiveProjectiles)
		{
			if (ProjectilePtr.IsValid())
			{
				AShotgunProjectile* Projectile = ProjectilePtr.Get();
				ValidProjectiles.Add(Projectile);

				// Move projectile to center location
				Projectile->SetActorLocation(CenterLocation);

				// Stop the projectile's movement
				if (Projectile->ShotgunProjectileMovement)
				{
					Projectile->ShotgunProjectileMovement->Velocity = FVector::ZeroVector;
					Projectile->ShotgunProjectileMovement->SetActive(false);
				}

				// Destroy the projectile after a short delay to show the convergence
				FTimerHandle DestroyTimer;
				GetWorld()->GetTimerManager().SetTimer(DestroyTimer, [Projectile]() {
					if (IsValid(Projectile))
					{
						Projectile->Destroy();
					}
					}, 0.5f, false);
			}
		}

		// Only create damage radius if we had projectiles to gather
		if (ValidProjectiles.Num() > 0)
		{
			// Calculate damage radius based on number of projectiles
			float BaseRadius = 500.0f;
			float RadiusPerProjectile = 200.0f;
			float TotalRadius = BaseRadius + (ValidProjectiles.Num() * RadiusPerProjectile);

			// Calculate damage based on number of projectiles
			float BaseDamage = 150.0f;
			float DamagePerProjectile = 50.0f;
			float TotalDamage = BaseDamage + (ValidProjectiles.Num() * DamagePerProjectile);

			// Create visual effect - draw debug sphere
			DrawDebugSphere(GetWorld(), CenterLocation, TotalRadius, 16, FColor::Red, false, 3.0f, 0, 5.0f);

			// Apply damage using our own overlap detection (similar to projectile damage)
			ApplyAltDefconDamage(CenterLocation, TotalRadius, TotalDamage);

			// Apply physics impulse to nearby objects
			FCollisionShape CollisionShape;
			CollisionShape.SetSphere(TotalRadius);

			TArray<FHitResult> HitResults;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(Character);

			bool bHit = GetWorld()->SweepMultiByChannel(
				HitResults,
				CenterLocation,
				CenterLocation,
				FQuat::Identity,
				ECC_PhysicsBody,
				CollisionShape,
				QueryParams
			);

			if (bHit)
			{
				for (const FHitResult& Hit : HitResults)
				{
					if (UPrimitiveComponent* HitComponent = Hit.GetComponent())
					{
						if (HitComponent->IsSimulatingPhysics())
						{
							float ForceStrength = 5000.0f * ValidProjectiles.Num();
							HitComponent->AddRadialForce(
								CenterLocation,
								TotalRadius,
								ForceStrength,
								ERadialImpulseFalloff::RIF_Linear,
								true
							);
						}
					}
				}
			}

			// Clear our projectile tracking array since we destroyed them
			ActiveProjectiles.Empty();

			// Optional: Add screen shake or other effects
			// UGameplayStatics::PlayWorldCameraShake(GetWorld(), CameraShakeClass, CenterLocation, 0.0f, TotalRadius);
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

		// Handle cooldown progression
		if (AltDefconProgress >= 1.0f)
		{
			AltDefconProgress = 0.0f;
			GetWorld()->GetTimerManager().SetTimer(
				AltDefconTimerHandle, this, &UShotgun_WeaponComponent::UpdateCooldowns, 0.1f, true);

			if (ShotgunHUDInstance)
			{
				ShotgunHUDInstance->ShowAltDefconProgressBar();
				ShotgunHUDInstance->UpdateCooldown(0.0f);
			}
		}

		// Set cooldown
		bCanUseAltDefcon = false;
		GetWorld()->GetTimerManager().SetTimer(
			AltDefconCooldownTimer,
			[this]() { bCanUseAltDefcon = true; },
			AltDefconCooldownDuration,
			false
		);
	}
}

void UShotgun_WeaponComponent::UpdateCooldowns()
{
	// Update AltTimeWarp progress
	if (AltTimeWarpProgress < 1.0f && CurrentWeaponMode == EShotgunMode::ShotgunMode1)
	{
		AltTimeWarpProgress += 0.1f / AltTimeWarpCooldown;
		if (ShotgunHUDInstance)
		{
			ShotgunHUDInstance->UpdateCooldown(AltTimeWarpProgress);
		}
		if (AltTimeWarpProgress >= 1.0f)
		{
			AltTimeWarpProgress = 1.0f;
			GetWorld()->GetTimerManager().ClearTimer(AltTimeWarpTimerHandle);
		}
	}

	// Update AltDefcon progress
	if (AltDefconProgress < 1.0f && CurrentWeaponMode == EShotgunMode::ShotgunMode2)
	{
		AltDefconProgress += 0.1f / AltDefconCooldown;
		if (ShotgunHUDInstance)
		{
			ShotgunHUDInstance->UpdateCooldown(AltDefconProgress);
		}
		if (AltDefconProgress >= 1.0f)
		{
			AltDefconProgress = 1.0f;
			GetWorld()->GetTimerManager().ClearTimer(AltDefconTimerHandle);
		}
	}
}

void UShotgun_WeaponComponent::ResetTimeWarpCooldown()
{
	bCanFireTimeWarp = true;
}

void UShotgun_WeaponComponent::ResetDefconCooldown()
{
	bCanFireDefcon = true;
}

void UShotgun_WeaponComponent::ApplyAltDefconDamage(const FVector& Origin, float Radius, float Damage)
{
	// Mark that the last attack is AltDefcon
	if (Character)
	{
		Character->bLastAttackWasZeroPoint = true;
	}

	// Get all actors within damage radius
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Character); // Ignore the player character

	TArray<AActor*> HitActors;
	bool bHit = UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		Origin,
		Radius,
		TArray<TEnumAsByte<EObjectTypeQuery>>(), // Empty array means all object types
		APawn::StaticClass(), // Only get pawns (characters)
		ActorsToIgnore,
		HitActors
	);

	if (bHit)
	{
		for (AActor* HitActor : HitActors)
		{
			if (HitActor && HitActor->CanBeDamaged())
			{
				// Double-check that this isn't the player character
				if (HitActor != Character)
				{
					if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
					{
						// Temporary marker: store on the actor that it was hit by AltDefcon
						HitActor->Tags.AddUnique(TEXT("ZeroPointPending"));

						float Distance = FVector::Dist(Origin, HitActor->GetActorLocation());
						float DamageMultiplier = 1.0f - (Distance / Radius);
						DamageMultiplier = FMath::Clamp(DamageMultiplier, 0.1f, 1.0f);
						float FinalDamage = Damage * DamageMultiplier;

						// Proper damage event
						FPointDamageEvent DamageEvent;
						DamageEvent.Damage = FinalDamage;
						DamageEvent.HitInfo.ImpactPoint = HitCharacter->GetActorLocation();
						DamageEvent.ShotDirection = (HitCharacter->GetActorLocation() - Origin).GetSafeNormal();

						// Pass Character's controller as InstigatedBy
						AController* PlayerController = Character ? Character->GetController() : nullptr;

						HitCharacter->TakeDamage(FinalDamage, DamageEvent, PlayerController, Character);

						UE_LOG(LogTemp, Warning, TEXT("AltDefcon applied %f damage to %s (Distance: %f)"), FinalDamage, *HitActor->GetName(), Distance);
					}
				}
			}
		}
	}

	// Reset the flag immediately after applying damage
	Character->bLastAttackWasZeroPoint = false;
}

void UShotgun_WeaponComponent::UnlockDefconMode()
{
	if (bDefconModeUnlocked)
	{
		UE_LOG(LogTemp, Warning, TEXT("Defcon mode is already unlocked!"));
		return;
	}

	// Get the BestResultsSubsystem
	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get GameInstance!"));
		return;
	}

	UBestResultsSubsystem* BestResultsSubsystem = GameInstance->GetSubsystem<UBestResultsSubsystem>();
	if (!BestResultsSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get BestResultsSubsystem!"));
		return;
	}

	// Check if player has enough money
	int32 CurrentMoney = BestResultsSubsystem->GetCurrentMoney();
	if (CurrentMoney < DefconUnlockCost)
	{
		UE_LOG(LogTemp, Warning, TEXT("Not enough money! Need %d, have %d"), DefconUnlockCost, CurrentMoney);

		// Optional: Show "Not Enough Money" notification to player
		// You can create a widget or use your existing notification system here

		return;
	}

	// Spend the money
	bool bSuccess = BestResultsSubsystem->SpendMoney(DefconUnlockCost);
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spend money!"));
		return;
	}

	// Unlock the mode
	bDefconModeUnlocked = true;
	UE_LOG(LogTemp, Warning, TEXT("Defcon mode unlocked! Spent %d money."), DefconUnlockCost);

	// Play unlock sound, show notification, etc.
}