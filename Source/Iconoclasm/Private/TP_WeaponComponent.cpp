// Copyright Epic Games, Inc. All Rights Reserved.


#include "TP_WeaponComponent.h"
#include "IconoclasmCharacter.h"
#include "IconoclasmProjectile.h"
#include "GrenadeLauncherHealProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GLHUD.h"

// Sets default values for this component's properties
UTP_WeaponComponent::UTP_WeaponComponent()
{
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
	CurrentWeaponMode = EWeaponMode::Mode1;
}

void UTP_WeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<AIconoclasmCharacter>(GetOwner());
}

void UTP_WeaponComponent::Fire()
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	switch (CurrentWeaponMode)
	{
	case EWeaponMode::Mode1:
		LifeBloodMode();
		break;
	case EWeaponMode::Mode2:
		ImpulseMode();
		break;
	default:
		break;
	}
}

void UTP_WeaponComponent::AltFire()
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();

	// Handle fire modes with cooldowns
	switch (CurrentWeaponMode)
	{
	case EWeaponMode::Mode1: // AltLifeBloodMode
		if (CurrentTime >= LastAltLifeBloodModeTime) // Only checks cooldown expiration
		{
			// Execute AltLifeBloodMode logic
			FVector ImpactLocation;
			PerformHitscan(ImpactLocation);
			HealingSphere(ImpactLocation, 200.0f); // Example radius
			UE_LOG(LogTemp, Warning, TEXT("AltLifeBloodMode activated!"));

			// Start cooldown after execution
			LastAltLifeBloodModeTime = CurrentTime + AltLifeBloodCooldown;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AltLifeBloodMode on cooldown!"));
		}
		break;

	case EWeaponMode::Mode2: // AltImpulseMode
		if (CurrentTime >= LastAltImpulseModeTime) // Only checks cooldown expiration
		{
			// Execute AltImpulseMode logic
			FVector ImpactLocation;
			PerformHitscan(ImpactLocation);
			ImpulseEffect(ImpactLocation, 300.0f, 1500.0f); // Example radius and strength
			UE_LOG(LogTemp, Warning, TEXT("AltImpulseMode activated!"));

			// Start cooldown after execution
			LastAltImpulseModeTime = CurrentTime + AltImpulseCooldown;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AltImpulseMode on cooldown!"));
		}
		break;

	default:
		break;
	}
}

void UTP_WeaponComponent::SwitchFireMode()
{
	// Cycle through the weapon modes
	CurrentWeaponMode = static_cast<EWeaponMode>((static_cast<uint8>(CurrentWeaponMode) + 1) % (static_cast<uint8>(EWeaponMode::Mode2) + 1));

	// Map modes to colors
	FLinearColor ModeColor;
	switch (CurrentWeaponMode)
	{
	case EWeaponMode::Mode1:
		ModeColor = FLinearColor::Red;  // Example: Mode1 = Red
		break;
	case EWeaponMode::Mode2:
		ModeColor = FLinearColor::Blue; // Example: Mode2 = Blue
		break;
	default:
		ModeColor = FLinearColor::White; // Default color
		break;
	}

	// Notify the HUD
	if (GLHUDInstance)
	{
		GLHUDInstance->UpdateImageColor(ModeColor);
	}
}

void UTP_WeaponComponent::PerformHitscan(FVector& ImpactLocation)
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

void UTP_WeaponComponent::ApplyExplosionEffect(const FVector& ImpactLocation, float Radius, float Strength)
{
	DrawDebugSphere(GetWorld(), ImpactLocation, Radius, 32, FColor::Green, false, 2.0f);

	TArray<AActor*> OverlappingActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		if (ACharacter* OverlappingCharacter = Cast<ACharacter>(Actor))
		{
			if (FVector::Dist(OverlappingCharacter->GetActorLocation(), ImpactLocation) <= Radius)
			{
				FVector LaunchDirection = (OverlappingCharacter->GetActorLocation() - ImpactLocation).GetSafeNormal();
				OverlappingCharacter->LaunchCharacter(LaunchDirection * Strength, true, true);
			}
		}
	}

}

void UTP_WeaponComponent::HealingSphere(const FVector& ImpactLocation, float Radius)
{
	// Draw the debug sphere to visualize the radius
	DrawDebugSphere(GetWorld(), ImpactLocation, Radius, 32, FColor::Green, false, 5.0f);
}

void UTP_WeaponComponent::ImpulseEffect(const FVector& ImpactLocation, float Radius, float Strength)
{
	// Draw debug sphere to visualize the radius
	DrawDebugSphere(GetWorld(), ImpactLocation, Radius, 32, FColor::Blue, false, 2.0f);

	// Get all actors within the radius
	TArray<AActor*> OverlappingActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), OverlappingActors);

	// Iterate over each actor to apply the impulse
	for (AActor* Actor : OverlappingActors)
	{
		// Skip the player character
		if (Actor != Character && FVector::Dist(Actor->GetActorLocation(), ImpactLocation) <= Radius)
		{
			// Get the primitive component to apply the impulse
			UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
			if (PrimComp && PrimComp->IsSimulatingPhysics())
			{
				// Calculate the impulse direction and apply it
				FVector ImpulseDirection = (Actor->GetActorLocation() - ImpactLocation).GetSafeNormal();
				PrimComp->AddImpulse(ImpulseDirection * Strength);

				// Debugging: Log information about the actor and impulse
				UE_LOG(LogTemp, Warning, TEXT("Applying impulse to %s"), *Actor->GetName());
				UE_LOG(LogTemp, Warning, TEXT("Impulse Direction: %s"), *ImpulseDirection.ToString());
				UE_LOG(LogTemp, Warning, TEXT("Impulse Strength: %f"), Strength);
			}
			else
			{
				// Debugging: Log if the primitive component is null or not simulating physics
				if (!PrimComp)
				{
					UE_LOG(LogTemp, Warning, TEXT("No primitive component found on %s"), *Actor->GetName());
				}
				else if (!PrimComp->IsSimulatingPhysics())
				{
					UE_LOG(LogTemp, Warning, TEXT("Primitive component is not simulating physics on %s"), *Actor->GetName());
				}
			}
		}
	}
}

void UTP_WeaponComponent::AntiGravity(const FVector& ImpactLocation, float Radius)
{
	// Draw debug sphere to visualize the radius
	DrawDebugSphere(GetWorld(), ImpactLocation, Radius, 32, FColor::Blue, false, 2.0f);

	// Get all actors within the radius
	TArray<AActor*> OverlappingActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), OverlappingActors);

	// Iterate over each actor to apply the anti-gravity effect
	for (AActor* Actor : OverlappingActors)
	{
		// Skip the player character
		if (Actor != Character && FVector::Dist(Actor->GetActorLocation(), ImpactLocation) <= Radius)
		{
			// Get the primitive component to disable gravity
			UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
			if (PrimComp && PrimComp->IsSimulatingPhysics())
			{
				PrimComp->SetEnableGravity(false);

				// Re-enable gravity after 5 seconds
				FTimerHandle TimerHandle;
				GetWorld()->GetTimerManager().SetTimer(TimerHandle, [PrimComp]()
					{
						PrimComp->SetEnableGravity(true);
					}, 5.0f, false);

				// Debugging: Log information about the actor
				UE_LOG(LogTemp, Warning, TEXT("Disabling gravity for %s"), *Actor->GetName());
			}
			else
			{
				// Debugging: Log if the primitive component is null or not simulating physics
				if (!PrimComp)
				{
					UE_LOG(LogTemp, Warning, TEXT("No primitive component found on %s"), *Actor->GetName());
				}
				else if (!PrimComp->IsSimulatingPhysics())
				{
					UE_LOG(LogTemp, Warning, TEXT("Primitive component is not simulating physics on %s"), *Actor->GetName());
				}
			}
		}
	}
}

void UTP_WeaponComponent::DetachFromCharacter()
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

		// Remove the HUD instance
		if (GLHUDInstance)
		{
			GLHUDInstance->RemoveFromViewport();
			GLHUDInstance = nullptr;
		}

		// Clear reference
		Character = nullptr;
	}
}


void UTP_WeaponComponent::AttachWeapon(AIconoclasmCharacter* TargetCharacter)
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
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::Fire);
			EnhancedInputComponent->BindAction(AltFireAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::AltFire);
			EnhancedInputComponent->BindAction(SwitchFireModeAction, ETriggerEvent::Triggered, this, &UTP_WeaponComponent::SwitchFireMode);
		}
	}

	if (GLHUDClass && !GLHUDInstance)
	{
		// Create the HUD instance
		GLHUDInstance = CreateWidget<UGLHUD>(GetWorld(), GLHUDClass);
		if (GLHUDInstance)
		{
			GLHUDInstance->AddToViewport();

			// Set initial mode color
			FLinearColor InitialColor = (CurrentWeaponMode == EWeaponMode::Mode1) ? FLinearColor::Red : FLinearColor::Blue;
			GLHUDInstance->UpdateImageColor(InitialColor);
		}
	}
}

void UTP_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

void UTP_WeaponComponent::LifeBloodMode()
{
	FVector ImpactLocation;
	PerformHitscan(ImpactLocation);
	ApplyExplosionEffect(ImpactLocation, 300.0f, 2000.0f); // Example radius and strength


	// Check if we hit an actor with a health component
	FHitResult HitResult;
	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	if (PlayerController)
	{
		FVector StartLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
		FRotator CameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
		FVector EndLocation = StartLocation + (CameraRotation.Vector() * 10000.0f);

		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Character);

		if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params))
		{
			if (AActor* HitActor = HitResult.GetActor())
			{
				// Check if the actor has the health component
				UHealthComponent* HealthComp = HitActor->FindComponentByClass<UHealthComponent>();
				if (HealthComp)
				{
					// Apply damage
					float DamageAmount = 50.0f; // Example damage value
					HealthComp->TakeDamage(DamageAmount);
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

void UTP_WeaponComponent::AltlifeBloodMode()
{
	FVector ImpactLocation;
	PerformHitscan(ImpactLocation);
	HealingSphere(ImpactLocation, 300); // Different radius and strength for alt fire

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

void UTP_WeaponComponent::ImpulseMode()
{
	FVector ImpactLocation;
	PerformHitscan(ImpactLocation);

	// Impulse effect
	float ImpulseRadius = 300.0f;
	float ImpulseStrength = 500000.0f;
	ImpulseEffect(ImpactLocation, ImpulseRadius, ImpulseStrength);

	// Apply radial damage
	float Damage = 100.0f; // Set the damage value
	TSubclassOf<UDamageType> DamageTypeClass = UDamageType::StaticClass();
	AController* InstigatorController = Character->GetController(); // Assumes Character is valid
	AActor* DamageCauser = Character;

	UGameplayStatics::ApplyRadialDamage(
		GetWorld(),
		Damage,
		ImpactLocation,
		ImpulseRadius,
		DamageTypeClass,
		TArray<AActor*>(), // Optional array of actors to ignore
		DamageCauser,
		InstigatorController,
		true // Whether to cause damage even if there’s no line of sight
	);

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

void UTP_WeaponComponent::AltImpulseMode()
{
	FVector ImpactLocation;
	PerformHitscan(ImpactLocation);

	// Apply anti-gravity effect
	AntiGravity(ImpactLocation, 500.0f); // Example radius

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

void UTP_WeaponComponent::GetCooldownProgress(float LastFireTime, float CooldownDuration) const
{
}
