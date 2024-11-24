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
#include "EnhancedInputSubsystems.h"

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
		TimeWarpMode();
		break;
	case EShotgunMode::ShotgunMode2:
		DefconMode();
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
		AltDefconMode();
		break;
	default:
		break;
	}
}

void UShotgun_WeaponComponent::SwitchFireMode()
{
	// Cycle through the weapon modes
	CurrentWeaponMode = static_cast<EShotgunMode>((static_cast<uint8>(CurrentWeaponMode) + 1) % (static_cast<uint8>(EShotgunMode::ShotgunMode2) + 1));
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

		if (GetWorld()->LineTraceSingleByChannel(HitResult, CurrentStartLocation, CurrentEndLocation, ECC_Visibility, Params))
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

		for (int32 i = 0; i < 8; ++i)
		{
			// Randomize the spread within the cone
			FRotator SpreadRotation = CameraRotation;
			SpreadRotation.Pitch += FMath::RandRange(-ConeAngle, ConeAngle);
			SpreadRotation.Yaw += FMath::RandRange(-ConeAngle, ConeAngle);

			FVector EndLocation = StartLocation + (SpreadRotation.Vector() * 10000.0f);
			PerformPumpHitscan(StartLocation, EndLocation, 8); // Limiting to 3 ricochets
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
	if (PlayerController)
	{
		FVector StartLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
		FRotator CameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
		FVector EndLocation = StartLocation + (CameraRotation.Vector() * 10000.0f);

		FHitResult HitResult;
		FVector ImpactLocation;

		PerformTeleportHitscan(ImpactLocation, HitResult);

		if (HitResult.GetActor())
		{
			TeleportPlayer(HitResult.Location);
		}

		// Play fire sound
		if (FireSound != nullptr)
		{
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
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
}

void UShotgun_WeaponComponent::DefconMode()
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

		// Perform multiple line traces within the cone
		for (int32 i = 0; i < 8; ++i)
		{
			FRotator SpreadRotation = CameraRotation;
			SpreadRotation.Pitch += FMath::RandRange(-ConeAngle, ConeAngle);
			SpreadRotation.Yaw += FMath::RandRange(-ConeAngle, ConeAngle);

			FVector EndLocation = StartLocation + (SpreadRotation.Vector() * 10000.0f);
			FHitResult HitResult;

			// Perform the hitscan
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(Character);

			GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params);

			// Draw debug line for visualization
			DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Red, false, 2.0f, 0, 1.0f);

			// Apply the launch force if we hit something
			if (HitResult.GetActor())
			{
				if (ACharacter* HitCharacter = Cast<ACharacter>(HitResult.GetActor()))
				{
					FVector LaunchDirection = (HitResult.Location - HitResult.TraceStart).GetSafeNormal();
					float LaunchStrength = 2000.0f; // Adjust as needed

					HitCharacter->LaunchCharacter(LaunchDirection * LaunchStrength, true, true);
				}
				else if (UPrimitiveComponent* HitComponent = Cast<UPrimitiveComponent>(HitResult.GetComponent()))
				{
					FVector LaunchDirection = (HitResult.Location - HitResult.TraceStart).GetSafeNormal();
					float LaunchStrength = 2000.0f; // Adjust as needed

					if (HitComponent->IsSimulatingPhysics())
					{
						HitComponent->AddImpulse(LaunchDirection * LaunchStrength, NAME_None, true);
					}
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

void UShotgun_WeaponComponent::AltDefconMode()
{
	if (!bCanUseAltDefcon || Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (PlayerController)
	{
		FVector StartLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
		FRotator CameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
		float ConeAngle = 10.0f;
		FVector ImpactLocation;

		// Perform hitscan to determine where to spawn the sphere
		PerformHitscan(ImpactLocation);

		// Spawn a giant sphere to act as a nuclear blast
		float BlastRadius = 5000.0f; // Adjust the radius as needed
		FVector SphereLocation = ImpactLocation;

		// Draw debug sphere (for visualization)
		DrawDebugSphere(GetWorld(), SphereLocation, BlastRadius, 12, FColor::Green, false, 5.0f);

		// Apply a radial force or damage in the area
		UGameplayStatics::ApplyRadialDamage(GetWorld(), 100.0f, SphereLocation, BlastRadius, UDamageType::StaticClass(), TArray<AActor*>(), Character, Character->GetController(), true);

		// Optionally apply a force to physics objects
		FCollisionShape CollisionShape;
		CollisionShape.SetSphere(BlastRadius);

		TArray<FHitResult> HitResults;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Character);

		bool bHit = GetWorld()->SweepMultiByChannel(HitResults, SphereLocation, SphereLocation, FQuat::Identity, ECC_PhysicsBody, CollisionShape, QueryParams);

		if (bHit)
		{
			for (const FHitResult& Hit : HitResults)
			{
				UPrimitiveComponent* HitComponent = Hit.GetComponent();
				if (HitComponent && HitComponent->IsSimulatingPhysics())
				{
					FVector ForceDirection = (Hit.Component->GetComponentLocation() - SphereLocation).GetSafeNormal();
					HitComponent->AddRadialForce(SphereLocation, BlastRadius, 10000.0f, ERadialImpulseFalloff::RIF_Linear, true); // Adjust force as needed
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

		// Set cooldown
		bCanUseAltDefcon = false;
		GetWorld()->GetTimerManager().SetTimer(
			AltDefconCooldownTimer,
			[this]() { bCanUseAltDefcon = true; },
			AltDefconCooldownDuration,
			false);
	}
}
