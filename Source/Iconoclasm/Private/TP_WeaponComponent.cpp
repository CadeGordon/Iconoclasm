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
#include "HealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RaphaelBossCharacter.h"
#include "HealthComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

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

void UTP_WeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update HUD cooldown progress
	UpdateCooldowns();
}

void UTP_WeaponComponent::ApplyHealing()
{
	UWorld* World = GetWorld();
	if (!World) return;

	TArray<AActor*> OverlappingActors;
	UGameplayStatics::GetAllActorsOfClass(World, ACharacter::StaticClass(), OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		if (FVector::Dist(Actor->GetActorLocation(), HealingSphereLocation) <= HealingSphereRadius)
		{
			AIconoclasmCharacter* PlayerCharacter = Cast<AIconoclasmCharacter>(Actor);
			if (PlayerCharacter)
			{
				// Get the HealthComponent
				UHealthComponent* HealthComp = PlayerCharacter->FindComponentByClass<UHealthComponent>();
				if (HealthComp)
				{
					HealthComp->Heal(10.0f);  // Adjust healing amount as needed
				}
			}
		}
	}
}

void UTP_WeaponComponent::StopHealing()
{
	GetWorld()->GetTimerManager().ClearTimer(HealingTimerHandle);
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
		if (bCanFireLifeBlood)
		{
			LifeBloodMode();
			bCanFireLifeBlood = false;
			GetWorld()->GetTimerManager().SetTimer(
				LifeBloodCooldownTimer,
				this,
				&UTP_WeaponComponent::ResetLifeBloodCooldown,
				LifeBloodCooldown,
				false
			);
		}
		break;
	case EWeaponMode::Mode2:
		if (bCanFireImpulse)
		{
			ImpulseMode();
			bCanFireImpulse = false;
			GetWorld()->GetTimerManager().SetTimer(
				ImpulseCooldownTimer,
				this,
				&UTP_WeaponComponent::ResetImpulseCooldown,
				ImpulseCooldown,
				false
			);
		}
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
			AltImpulseMode();
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
		Params.bTraceComplex = true;

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

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	AIconoclasmCharacter* PlayerCharacter = Cast<AIconoclasmCharacter>(OwnerActor);

	// Manually detect actors in the explosion radius
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(Radius);

	bool bHit = GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		ImpactLocation,
		FQuat::Identity,
		ECC_Pawn,
		CollisionShape
	);

	if (bHit)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* HitActor = Result.GetActor();
			if (!HitActor) continue;

			if (HitActor->IsA(AIconoclasmCharacter::StaticClass()))
			{
				continue; // Skip damaging any player characters
			}

			// Apply damage
			UGameplayStatics::ApplyDamage(
				HitActor,
				100.0f,
				OwnerActor->GetInstigatorController(),
				OwnerActor,
				UDamageType::StaticClass()
			);
		}
	}

	// Apply force to all characters in radius
	TArray<AActor*> OverlappingActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		AIconoclasmCharacter* AffectedCharacter = Cast<AIconoclasmCharacter>(Actor);
		if (AffectedCharacter && FVector::Dist(AffectedCharacter->GetActorLocation(), ImpactLocation) <= Radius)
		{
			FVector LaunchDirection = (AffectedCharacter->GetActorLocation() - ImpactLocation).GetSafeNormal();
			AffectedCharacter->LaunchCharacter(LaunchDirection * Strength, true, true);
		}
	}

}

void UTP_WeaponComponent::HealingSphere(const FVector& ImpactLocation, float Radius)
{
	// Draw the debug sphere to visualize the radius
	DrawDebugSphere(GetWorld(), ImpactLocation, Radius, 32, FColor::Green, false, 5.0f);

	// Start the healing loop
	GetWorld()->GetTimerManager().SetTimer(HealingTimerHandle, this, &UTP_WeaponComponent::ApplyHealing, 0.5f, true);

	// Stop healing after 5 seconds
	GetWorld()->GetTimerManager().SetTimer(HealingEndTimerHandle, this, &UTP_WeaponComponent::StopHealing, 5.0f, false);

	// Store sphere data
	HealingSphereLocation = ImpactLocation;
	HealingSphereRadius = Radius;
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

	for (AActor* Actor : OverlappingActors)
	{
		if (Actor != Character && FVector::Dist(Actor->GetActorLocation(), ImpactLocation) <= Radius)
		{
			bool bEffectApplied = false;

			// Handle physics-simulating objects
			if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
			{
				if (PrimComp->IsSimulatingPhysics())
				{
					PrimComp->SetEnableGravity(false);
					bEffectApplied = true;

					FTimerHandle TimerHandle;
					GetWorld()->GetTimerManager().SetTimer(TimerHandle, [PrimComp]()
						{
							PrimComp->SetEnableGravity(true);
						}, 5.0f, false);

					UE_LOG(LogTemp, Warning, TEXT("Disabled gravity (physics) for %s"), *Actor->GetName());
				}
			}

			// Handle characters (like AI enemies)
			if (ACharacter* AffectedCharacter = Cast<ACharacter>(Actor))
			{
				UCharacterMovementComponent* MoveComp = AffectedCharacter->GetCharacterMovement();
				if (MoveComp)
				{
					MoveComp->GravityScale = 0.0f;
					bEffectApplied = true;

					FTimerHandle TimerHandle;
					GetWorld()->GetTimerManager().SetTimer(TimerHandle, [MoveComp]()
						{
							MoveComp->GravityScale = 1.0f;
						}, 5.0f, false);

					UE_LOG(LogTemp, Warning, TEXT("Disabled gravity (character) for %s"), *Actor->GetName());
				}
			}

			if (!bEffectApplied)
			{
				UE_LOG(LogTemp, Warning, TEXT("No anti-gravity effect applied to %s"), *Actor->GetName());
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
			GLHUDInstance->RemoveFromParent();
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
		Params.bTraceComplex = true;

		if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Pawn, Params))
		{
			if (AActor* HitActor = HitResult.GetActor())
			{
				UHealthComponent* HitHealthComp = HitActor->FindComponentByClass<UHealthComponent>();
				UHealthComponent* PlayerHealthComp = Character->FindComponentByClass<UHealthComponent>();

				if (HitHealthComp && PlayerHealthComp)
				{
					if (HitActor->IsA(AIconoclasmCharacter::StaticClass()))
					{
						// Heal the player when hitting an IconoclasmCharacter
						PlayerHealthComp->Heal(30.0f); // Example heal amount
					}
					else
					{
						// Deal damage to enemies
						float DamageAmount = 50.0f;
						HitHealthComp->TakeDamage(DamageAmount);
					}
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
	// Try to fire a projectile instead of hitscan
	if (GrenadeProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
			if (PlayerController)
			{
				// Get the camera location and rotation for projectile spawn
				const FRotator SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();

				// Transform the MuzzleOffset from local space to world space
				const FVector SpawnLocation = PlayerController->PlayerCameraManager->GetCameraLocation() +
					SpawnRotation.RotateVector(MuzzleOffset);

				// Set Spawn Collision Handling Override
				FActorSpawnParameters ActorSpawnParams;
				ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

				// Spawn the projectile at the muzzle
				AGrenadeLauncherProjectile* Projectile = World->SpawnActor<AGrenadeLauncherProjectile>(GrenadeProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);

				if (Projectile)
				{
					// Set the projectile's initial trajectory
					const FVector LaunchDirection = SpawnRotation.Vector();
					Projectile->GrenadeFireInDirection(LaunchDirection);
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

void UTP_WeaponComponent::AltImpulseMode()
{
	// Set teleport mark at current location before firing
	SetTeleportMark();

	//// Try to fire a projectile instead of hitscan
	//if (GrenadeProjectileClass != nullptr)
	//{
	//	UWorld* const World = GetWorld();
	//	if (World != nullptr)
	//	{
	//		APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	//		if (PlayerController)
	//		{
	//			// Get the camera location and rotation for projectile spawn
	//			const FRotator SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
	//			// Transform the MuzzleOffset from local space to world space
	//			const FVector SpawnLocation = PlayerController->PlayerCameraManager->GetCameraLocation() +
	//				SpawnRotation.RotateVector(MuzzleOffset);
	//			// Set Spawn Collision Handling Override
	//			FActorSpawnParameters ActorSpawnParams;
	//			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	//			// Spawn the projectile at the muzzle
	//			AGrenadeLauncherProjectile* Projectile = World->SpawnActor<AGrenadeLauncherProjectile>(GrenadeProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
	//			if (Projectile)
	//			{
	//				// Set the projectile's initial trajectory
	//				const FVector LaunchDirection = SpawnRotation.Vector();
	//				Projectile->GrenadeFireInDirection(LaunchDirection);
	//				//// SET ALT FIRE MODE - This is the key addition!
	//				//Projectile->SetAltFireMode(true);
	//			}
	//		}
	//	}
	//}
	
	// Mark that the player is in TimeWarp mode for kill bonus
	if (Character)
	{
		Character->bLastAttackWasTimeWarp = true;
		Character->LastTimeWarpTime = GetWorld()->GetTimeSeconds();
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

void UTP_WeaponComponent::GetCooldownProgress(float LastFireTime, float CooldownDuration) const
{
}

void UTP_WeaponComponent::UpdateCooldowns()
{
	if (GLHUDInstance)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();

		// Determine the currently active mode
		switch (CurrentWeaponMode)
		{
		case EWeaponMode::Mode1: // AltLifeBloodMode
		{
			float AltLifeBloodProgress = 1.0f;
			if (CurrentTime < LastAltLifeBloodModeTime)
			{
				AltLifeBloodProgress = FMath::Clamp(1.0f - ((LastAltLifeBloodModeTime - CurrentTime) / AltLifeBloodCooldown), 0.0f, 1.0f);
			}
			GLHUDInstance->SetAltLifeBloodModeActive(true);
			GLHUDInstance->SetAltImpulseModeActive(false);
			GLHUDInstance->UpdateAltLifeBloodCooldownProgress(AltLifeBloodProgress);
			break;
		}
		case EWeaponMode::Mode2: // AltImpulseMode
		{
			float AltImpulseProgress = 1.0f;
			if (CurrentTime < LastAltImpulseModeTime)
			{
				AltImpulseProgress = FMath::Clamp(1.0f - ((LastAltImpulseModeTime - CurrentTime) / AltImpulseCooldown), 0.0f, 1.0f);
			}
			GLHUDInstance->SetAltLifeBloodModeActive(false);
			GLHUDInstance->SetAltImpulseModeActive(true);
			GLHUDInstance->UpdateAltImpulseCooldownProgress(AltImpulseProgress);
			break;
		}
		default:
			// Hide both progress bars for unhandled modes
			GLHUDInstance->SetAltLifeBloodModeActive(false);
			GLHUDInstance->SetAltImpulseModeActive(false);
			break;
		}
	}
}

void UTP_WeaponComponent::ResetLifeBloodCooldown()
{
	bCanFireLifeBlood = true;
}

void UTP_WeaponComponent::ResetImpulseCooldown()
{
	bCanFireImpulse = true;
}

void UTP_WeaponComponent::SetTeleportMark()
{
	if (Character)
	{
		// Store current location
		TeleportMarkLocation = Character->GetActorLocation();

		// Store current health
		if (UHealthComponent* HealthComp = Character->FindComponentByClass<UHealthComponent>())
		{
			TeleportMarkHealth = HealthComp->GetCurrentHealth();
		}

		bHasTeleportMark = true;

		// Draw a line trace from current location upward to mark the spot
		FVector LineStart = TeleportMarkLocation;
		FVector LineEnd = TeleportMarkLocation + FVector(0, 0, 500.0f); // 500 units upward

		// Draw the teleport mark line
		if (GetWorld())
		{
			DrawDebugLine(
				GetWorld(),
				LineStart,
				LineEnd,
				FColor::Cyan,
				false,
				TeleportLineDuration, // Duration
				0,
				8.0f // Thickness
			);

			// Draw a sphere at the base to make it more visible
			DrawDebugSphere(
				GetWorld(),
				TeleportMarkLocation,
				50.0f,
				12,
				FColor::Cyan,
				false,
				TeleportLineDuration
			);
		}

		// Start timer for auto-teleport
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(
				TeleportTimerHandle,
				this,
				&UTP_WeaponComponent::OnTeleportTimerExpired,
				TeleportDelay,
				false
			);
		}

		// Debug message
		if (GEngine)
		{
			FString DebugMsg = FString::Printf(TEXT("Teleport mark set! Auto-teleport in %.1f seconds"), TeleportDelay);
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, DebugMsg);
		}
	}
}

void UTP_WeaponComponent::TeleportToMark()
{
	if (bHasTeleportMark && Character)
	{
		// Teleport to marked location
		Character->SetActorLocation(TeleportMarkLocation, false, nullptr, ETeleportType::TeleportPhysics);

		// Restore health to what it was when mark was set
		if (UHealthComponent* HealthComp = Character->FindComponentByClass<UHealthComponent>())
		{
			HealthComp->SetCurrentHealth(TeleportMarkHealth);
		}

		// Clear the teleport mark
		bHasTeleportMark = false;

		// Clear the timer if it's still running
		if (GetWorld() && TeleportTimerHandle.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(TeleportTimerHandle);
		}

		// Visual effect at teleport destination
		if (GetWorld())
		{
			// Spawn a brief visual effect at the teleport location
			DrawDebugSphere(
				GetWorld(),
				TeleportMarkLocation,
				100.0f,
				16,
				FColor::Purple,
				false,
				2.0f
			);

			// Upward burst effect
			DrawDebugLine(
				GetWorld(),
				TeleportMarkLocation,
				TeleportMarkLocation + FVector(0, 0, 300.0f),
				FColor::Purple,
				false,
				2.0f,
				0,
				10.0f
			);
		}

		// Play teleport sound if you have one
		// UGameplayStatics::PlaySoundAtLocation(GetWorld(), TeleportSound, TeleportMarkLocation);

		// Debug message
		if (GEngine)
		{
			FString HealthMsg = FString::Printf(TEXT("Teleported back! Health restored to %.1f"), TeleportMarkHealth);
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Purple, HealthMsg);
		}
	}
}

void UTP_WeaponComponent::OnTeleportTimerExpired()
{
	// Auto-teleport when timer expires
	TeleportToMark();
}
