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
#include "BestResultsSubsystem.h"


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
		if (bCanFireGunslinger)
		{
			GunslingerMode();
			bCanFireGunslinger = false;
			GetWorld()->GetTimerManager().SetTimer(
				GunslingerCooldownTimer,
				this,
				&URevolver_WeaponComponent::ResetGunslingerCooldown,
				GunslingerCooldown,
				false
			);
		}
		break;
	case ERevolverMode::RevolverMode2:
		// Check if Hellfire mode is unlocked
		if (!bHellfireModeUnlocked)
		{
			UE_LOG(LogTemp, Warning, TEXT("Hellfire mode is locked!"));
			return;
		}

		if (bCanFireHellfire)
		{
			HellfireMode();
			bCanFireHellfire = false;
			GetWorld()->GetTimerManager().SetTimer(
				HellfireCooldownTimer,
				this,
				&URevolver_WeaponComponent::ResetHellfireCooldown,
				HellfireCooldown,
				false
			);
		}
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
		// Check if Hellfire mode is unlocked
		if (!bHellfireModeUnlocked)
		{
			UE_LOG(LogTemp, Warning, TEXT("Hellfire mode is locked!"));
			return;
		}
		AltHellfireMode();
		break;
	default:
		break;
	}
}

// Static helper function to get the revolver component from the player
URevolver_WeaponComponent* URevolver_WeaponComponent::GetRevolverComponentFromPlayer(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Get the player controller
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return nullptr;
	}

	// Get the player pawn
	APawn* PlayerPawn = PC->GetPawn();
	if (!PlayerPawn)
	{
		return nullptr;
	}

	// Method 1: Check if component is on the pawn directly
	URevolver_WeaponComponent* RevolverComp = PlayerPawn->FindComponentByClass<URevolver_WeaponComponent>();
	if (RevolverComp)
	{
		return RevolverComp;
	}

	// Method 2: Check attached actors (weapons picked up)
	TArray<AActor*> AttachedActors;
	PlayerPawn->GetAttachedActors(AttachedActors);

	for (AActor* AttachedActor : AttachedActors)
	{
		if (AttachedActor)
		{
			RevolverComp = AttachedActor->FindComponentByClass<URevolver_WeaponComponent>();
			if (RevolverComp)
			{
				return RevolverComp;
			}
		}
	}

	// Method 3: Search all actors with the component
	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

	for (AActor* Actor : AllActors)
	{
		RevolverComp = Actor->FindComponentByClass<URevolver_WeaponComponent>();
		if (RevolverComp)
		{
			// Check if this component belongs to our player
			AIconoclasmCharacter* RevolverOwner = Cast<AIconoclasmCharacter>(RevolverComp->Character);
			AIconoclasmCharacter* PlayerCharacter = Cast<AIconoclasmCharacter>(PlayerPawn);

			if (RevolverOwner == PlayerCharacter)
			{
				return RevolverComp;
			}
		}
	}

	return nullptr;
}


void URevolver_WeaponComponent::SwitchFireMode()
{
	// Store the current mode
	ERevolverMode PreviousMode = CurrentWeaponMode;

	// Cycle through the weapon modes
	CurrentWeaponMode = static_cast<ERevolverMode>((static_cast<uint8>(CurrentWeaponMode) + 1) % (static_cast<uint8>(ERevolverMode::RevolverMode2) + 1));

	// If we switched to Hellfire mode but it's locked, switch back
	if (CurrentWeaponMode == ERevolverMode::RevolverMode2 && !bHellfireModeUnlocked)
	{
		CurrentWeaponMode = PreviousMode;
		UE_LOG(LogTemp, Warning, TEXT("Cannot switch to Hellfire mode - it is locked!"));
		return;
	}

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
			// Alt Fire - bind to both Started and Completed events for charge system
			EnhancedInputComponent->BindAction(AltFireAction, ETriggerEvent::Started, this, &URevolver_WeaponComponent::OnAltFirePressed);
			EnhancedInputComponent->BindAction(AltFireAction, ETriggerEvent::Completed, this, &URevolver_WeaponComponent::OnAltFireReleased);
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

void URevolver_WeaponComponent::UnlockHellfireMode()
{
	if (bHellfireModeUnlocked)
	{
		UE_LOG(LogTemp, Warning, TEXT("Hellfire mode is already unlocked!"));
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
	if (CurrentMoney < HellfireUnlockCost)
	{
		UE_LOG(LogTemp, Warning, TEXT("Not enough money! Need %d, have %d"), HellfireUnlockCost, CurrentMoney);

		// Optional: Show "Not Enough Money" notification to player
		// You can create a widget or use your existing notification system here

		return;
	}

	// Spend the money
	bool bSuccess = BestResultsSubsystem->SpendMoney(HellfireUnlockCost);
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spend money!"));
		return;
	}

	// Unlock the mode
	bHellfireModeUnlocked = true;
	UE_LOG(LogTemp, Warning, TEXT("Hellfire mode unlocked! Spent %d money."), HellfireUnlockCost);

	// Hide the unlock widget
	if (HellfireUnlockWidget)
	{
		HellfireUnlockWidget->RemoveFromParent();
		HellfireUnlockWidget = nullptr;
	}

	// Play unlock sound, show notification, etc.
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

		if (RevolverShotParticle && Character)
		{
			// Make tracer start at the muzzle
			FVector TracerStartLocation = Character->GetActorLocation() + Character->GetControlRotation().RotateVector(MuzzleOffset);
			FVector TracerEndLocation = ImpactLocation;

			UNiagaraComponent* TracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				RevolverShotParticle,
				TracerStartLocation
			);

			if (TracerComp)
			{
				// Make it point along the line trace
				FRotator TracerRotation = (TracerEndLocation - TracerStartLocation).Rotation();
				TracerComp->SetWorldRotation(TracerRotation);

				// Optional: scale length to match distance
				float Distance = FVector::Distance(TracerStartLocation, TracerEndLocation);
				TracerComp->SetWorldScale3D(FVector(Distance / 1000.0f, 1.0f, 1.0f));
			}
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

	// Start charging the shot
	StartChargingShot();
}


void URevolver_WeaponComponent::HellfireMode()
{
	HitscanCount = 1; // Set the number of hitscans to fire

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
			FVector StartLocation = Character->GetActorLocation();
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

	float BaseDamage = 5000.0f; // Base damage for initial line trace
	float EnemyHitSplitDamage = 30000.0f; // Damage for each split trace when hitting enemy
	float SurfaceBounceShatterDamage = 35000.0f; // Increased damage for surface bounce shatter
	float MaxRange = 200000.0f; // Maximum range for initial trace
	float BounceRange = 200000.0f; // Maximum range for bounce trace
	float ShatterSearchRadius = 50000.0f; // Radius to search for enemies when shattering (500m)

	// Initial line trace
	FVector StartLocation = Character->GetActorLocation() + Character->GetControlRotation().RotateVector(MuzzleOffset);
	FVector ForwardVector = Character->GetControlRotation().Vector();
	FVector EndLocation = StartLocation + (ForwardVector * MaxRange);

	FHitResult InitialHitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	// Perform initial sphere trace (thicker line trace)
	float TraceRadius = 50.0f; // Thickness of the trace
	FCollisionShape TraceSphere = FCollisionShape::MakeSphere(TraceRadius);

	bool bInitialHit = GetWorld()->SweepSingleByChannel(
		InitialHitResult,
		StartLocation,
		EndLocation,
		FQuat::Identity,
		ECC_Pawn,
		TraceSphere,
		QueryParams
	);

	FVector ShatterLocation;
	float ShatterDamage = EnemyHitSplitDamage;
	bool bShouldShatter = false;

	if (bInitialHit)
	{
		// Check if we hit an enemy (Pawn)
		if (InitialHitResult.GetActor() && InitialHitResult.GetActor()->IsA(APawn::StaticClass()))
		{
			// Hit an enemy - apply damage and shatter immediately
			Character->bLastAttackWasShatterShot = true;

			UGameplayStatics::ApplyDamage(
				InitialHitResult.GetActor(),
				BaseDamage,
				Character->GetController(),
				Character,
				UDamageType::StaticClass()
			);

			ShatterLocation = InitialHitResult.Location;
			ShatterDamage = EnemyHitSplitDamage;
			bShouldShatter = true;

			// Spawn tracer for initial shot
			if (RevolverShotParticle)
			{
				UNiagaraComponent* TracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					GetWorld(),
					RevolverShotParticle,
					StartLocation
				);

				if (TracerComp)
				{
					FRotator TracerRotation = (ShatterLocation - StartLocation).Rotation();
					TracerComp->SetWorldRotation(TracerRotation);

					float Distance = FVector::Distance(StartLocation, ShatterLocation);
					TracerComp->SetWorldScale3D(FVector(Distance / 100.0f, 1.0f, 1.0f));
				}
			}

			// Draw debug line
			DrawDebugLine(
				GetWorld(),
				StartLocation,
				ShatterLocation,
				FColor::Red,
				false,
				2.0f,
				0,
				5.0f
			);
		}
		else
		{
			// Hit a surface - bounce off and find second impact
			FVector BounceStart = InitialHitResult.Location;
			FVector IncomingDirection = ForwardVector;
			FVector SurfaceNormal = InitialHitResult.Normal;

			// Calculate reflection direction
			FVector BounceDirection = IncomingDirection - (2.0f * FVector::DotProduct(IncomingDirection, SurfaceNormal) * SurfaceNormal);
			BounceDirection.Normalize();

			// Offset the bounce start slightly along the normal to avoid hitting the same surface
			FVector BounceStartOffset = BounceStart + (SurfaceNormal * 10.0f);
			FVector BounceEnd = BounceStartOffset + (BounceDirection * BounceRange);

			// Create new query params for bounce and ignore the surface we just hit
			FCollisionQueryParams BounceQueryParams;
			BounceQueryParams.AddIgnoredActor(Character);
			if (InitialHitResult.GetActor())
			{
				BounceQueryParams.AddIgnoredActor(InitialHitResult.GetActor());
			}

			// Perform bounce trace
			FHitResult BounceHitResult;
			bool bBounceHit = GetWorld()->SweepSingleByChannel(
				BounceHitResult,
				BounceStartOffset,
				BounceEnd,
				FQuat::Identity,
				ECC_Pawn,
				TraceSphere,
				BounceQueryParams
			);

			// Spawn tracer for initial shot to first surface
			if (RevolverShotParticle)
			{
				UNiagaraComponent* TracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					GetWorld(),
					RevolverShotParticle,
					StartLocation
				);

				if (TracerComp)
				{
					FRotator TracerRotation = (BounceStart - StartLocation).Rotation();
					TracerComp->SetWorldRotation(TracerRotation);

					float Distance = FVector::Distance(StartLocation, BounceStart);
					TracerComp->SetWorldScale3D(FVector(Distance / 100.0f, 1.0f, 1.0f));
				}
			}

			// Draw debug line for first impact
			DrawDebugLine(
				GetWorld(),
				StartLocation,
				BounceStart,
				FColor::Yellow,
				false,
				2.0f,
				0,
				5.0f
			);

			if (bBounceHit)
			{
				ShatterLocation = BounceHitResult.Location;
				ShatterDamage = SurfaceBounceShatterDamage; // Increased damage for bounce shatter
				bShouldShatter = true;

				// Spawn tracer for bounce shot
				if (RevolverShotParticle)
				{
					UNiagaraComponent* BounceTracer = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
						GetWorld(),
						RevolverShotParticle,
						BounceStart
					);

					if (BounceTracer)
					{
						FRotator BounceRotation = (ShatterLocation - BounceStart).Rotation();
						BounceTracer->SetWorldRotation(BounceRotation);

						float BounceDistance = FVector::Distance(BounceStart, ShatterLocation);
						BounceTracer->SetWorldScale3D(FVector(BounceDistance / 100.0f, 1.0f, 1.0f));
					}
				}

				// Draw debug line for bounce
				DrawDebugLine(
					GetWorld(),
					BounceStart,
					ShatterLocation,
					FColor::Cyan,
					false,
					2.0f,
					0,
					5.0f
				);
			}
			else
			{
				// Bounce didn't hit anything - shatter at end of bounce trace
				ShatterLocation = BounceEnd;
				ShatterDamage = SurfaceBounceShatterDamage;
				bShouldShatter = true;

				// Spawn tracer for bounce shot
				if (RevolverShotParticle)
				{
					UNiagaraComponent* BounceTracer = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
						GetWorld(),
						RevolverShotParticle,
						BounceStart
					);

					if (BounceTracer)
					{
						FRotator BounceRotation = (ShatterLocation - BounceStart).Rotation();
						BounceTracer->SetWorldRotation(BounceRotation);

						float BounceDistance = FVector::Distance(BounceStart, ShatterLocation);
						BounceTracer->SetWorldScale3D(FVector(BounceDistance / 100.0f, 1.0f, 1.0f));
					}
				}

				// Draw debug line
				DrawDebugLine(
					GetWorld(),
					BounceStart,
					ShatterLocation,
					FColor::Cyan,
					false,
					2.0f,
					0,
					5.0f
				);
			}
		}
	}

	// Shatter logic - always happens after determining shatter location
	if (bShouldShatter)
	{
		// Find nearest enemies within radius from shatter location
		TArray<AActor*> NearestEnemies = FindNearestEnemiesInRadius(ShatterLocation, 2, ShatterSearchRadius);

		// Fire split traces to the nearest enemies (if any found in radius)
		for (AActor* Enemy : NearestEnemies)
		{
			if (Enemy)
			{
				FVector SplitEndLocation = Enemy->GetActorLocation();
				FHitResult SplitHitResult;

				// Perform split sphere trace
				float SplitTraceRadius = 30.0f;
				FCollisionShape SplitTraceSphere = FCollisionShape::MakeSphere(SplitTraceRadius);

				bool bSplitHit = GetWorld()->SweepSingleByChannel(
					SplitHitResult,
					ShatterLocation,
					SplitEndLocation,
					FQuat::Identity,
					ECC_Pawn,
					SplitTraceSphere,
					QueryParams
				);

				if (bSplitHit && SplitHitResult.GetActor())
				{
					Character->bLastAttackWasShatterShot = true;

					// Apply damage to the hit actor
					UGameplayStatics::ApplyDamage(
						SplitHitResult.GetActor(),
						ShatterDamage,
						Character->GetController(),
						Character,
						UDamageType::StaticClass()
					);
				}

				// Spawn tracer for split shot
				if (RevolverShotParticle)
				{
					UNiagaraComponent* SplitTracer = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
						GetWorld(),
						RevolverShotParticle,
						ShatterLocation
					);

					if (SplitTracer)
					{
						FRotator SplitRotation = (SplitEndLocation - ShatterLocation).Rotation();
						SplitTracer->SetWorldRotation(SplitRotation);

						float SplitDistance = FVector::Distance(ShatterLocation, SplitEndLocation);
						SplitTracer->SetWorldScale3D(FVector(SplitDistance / 100.0f, 1.0f, 1.0f));
					}
				}

				// Draw debug line for split traces
				DrawDebugLine(
					GetWorld(),
					ShatterLocation,
					SplitEndLocation,
					FColor::Orange,
					false,
					2.0f,
					0,
					3.0f
				);
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

	// Set the cooldown timer for AltHellfire
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_AltHellfireCooldown,
		[this]()
		{
			bCanFireAltHellfire = true;
		},
		3.0f,
		false
	);

	// Update AltHellfire cooldown progress bar to 0% when activated
	if (RevolverHUD != nullptr)
	{
		RevolverHUD->UpdateAltHellfireCooldownProgress(0.0f);
	}

	// Start updating the progress for the cooldown
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_AltHellfireProgress,
		this,
		&URevolver_WeaponComponent::HandleHellfireAltCooldown,
		0.01f,
		true
	);
}

// Helper function to find nearest enemies
TArray<AActor*> URevolver_WeaponComponent::FindNearestEnemiesInRadius(const FVector& Location, int32 MaxEnemies, float SearchRadius)
{
	TArray<AActor*> NearestEnemies;
	TArray<AActor*> AllPawns;

	// Get all pawns in the world
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), AllPawns);

	// Structure to hold distance and actor pairs
	TArray<TPair<float, AActor*>> EnemyDistances;

	for (AActor* Pawn : AllPawns)
	{
		// Skip if it's the player character or null
		if (!Pawn || Pawn == Character)
		{
			continue;
		}

		// Calculate distance
		float Distance = FVector::Dist(Location, Pawn->GetActorLocation());

		// Only consider enemies within the search radius
		if (Distance <= SearchRadius)
		{
			EnemyDistances.Add(TPair<float, AActor*>(Distance, Pawn));
		}
	}

	// Sort by distance (closest first)
	EnemyDistances.Sort([](const TPair<float, AActor*>& A, const TPair<float, AActor*>& B)
		{
			return A.Key < B.Key;
		});

	// Get the closest enemies up to MaxEnemies
	int32 Count = FMath::Min(MaxEnemies, EnemyDistances.Num());
	for (int32 i = 0; i < Count; i++)
	{
		NearestEnemies.Add(EnemyDistances[i].Value);
	}

	return NearestEnemies;
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

void URevolver_WeaponComponent::ResetGunslingerCooldown()
{
	bCanFireGunslinger = true;
}

void URevolver_WeaponComponent::ResetHellfireCooldown()
{
	bCanFireHellfire = true;
}


void URevolver_WeaponComponent::StartChargingShot()
{
	if (bIsChargingShot)
		return;

	bIsChargingShot = true;
	ChargeStartTime = GetWorld()->GetTimeSeconds();
	CurrentChargeLevel = 0.0f;

	// Start charge timer
	GetWorld()->GetTimerManager().SetTimer(ChargeTimerHandle, this, &URevolver_WeaponComponent::UpdateCharge, 0.02f, true);

	// Start visual trace timer
	GetWorld()->GetTimerManager().SetTimer(TraceVisualizationHandle, this, &URevolver_WeaponComponent::UpdateChargeTrace, 0.02f, true);
}

void URevolver_WeaponComponent::UpdateCharge()
{
	if (!bIsChargingShot)
		return;

	float ElapsedChargeTime = GetWorld()->GetTimeSeconds() - ChargeStartTime;
	CurrentChargeLevel = FMath::Clamp(ElapsedChargeTime / MaxChargeTime, 0.0f, 1.0f);

	// Update HUD to show charge level
	if (RevolverHUD != nullptr)
	{
		RevolverHUD->UpdateAltFireCooldownProgress(CurrentChargeLevel);
	}

	// Stop charging at 100%
	if (CurrentChargeLevel >= 1.0f)
	{
		CurrentChargeLevel = 1.0f;
		GetWorld()->GetTimerManager().ClearTimer(ChargeTimerHandle);
	}
}

void URevolver_WeaponComponent::UpdateChargeTrace()
{
	if (!bIsChargingShot || !Character)
		return;

	// Calculate trace endpoints
	FVector StartLocation = Character->GetActorLocation();
	FVector ForwardVector = Character->GetControlRotation().Vector();
	FVector EndLocation = StartLocation + (ForwardVector * 10000.0f); // Long range trace

	// Determine trace color based on charge level
	FLinearColor TraceColor = GetChargeTraceColor(CurrentChargeLevel);

	// Draw debug line (you might want to replace this with a more sophisticated visual system)
	DrawDebugLine(
		GetWorld(),
		StartLocation,
		EndLocation,
		TraceColor.ToFColor(true),
		false,
		0.05f, // Duration slightly longer than update frequency
		0,
		3.0f // Thickness
	);
}

FLinearColor URevolver_WeaponComponent::GetChargeTraceColor(float ChargePercent)
{
	if (ChargePercent < 0.25f)
	{
		// 0-25%: White to Blue
		float Alpha = ChargePercent / 0.25f;
		return FLinearColor::LerpUsingHSV(FLinearColor::White, FLinearColor::Blue, Alpha);
	}
	else if (ChargePercent < 0.50f)
	{
		// 25-50%: Blue to Green
		float Alpha = (ChargePercent - 0.25f) / 0.25f;
		return FLinearColor::LerpUsingHSV(FLinearColor::Blue, FLinearColor::Green, Alpha);
	}
	else if (ChargePercent < 0.75f)
	{
		// 50-75%: Green to Yellow
		float Alpha = (ChargePercent - 0.50f) / 0.25f;
		return FLinearColor::LerpUsingHSV(FLinearColor::Green, FLinearColor::Yellow, Alpha);
	}
	else
	{
		// 75-100%: Yellow to Orange
		float Alpha = (ChargePercent - 0.75f) / 0.25f;
		return FLinearColor::LerpUsingHSV(FLinearColor::Yellow, FLinearColor(1.0f, 0.5f, 0.0f), Alpha);
	}
}

void URevolver_WeaponComponent::ReleaseChargedShot()
{
	if (!bIsChargingShot)
		return;

	// Stop charging
	bIsChargingShot = false;
	GetWorld()->GetTimerManager().ClearTimer(ChargeTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(TraceVisualizationHandle);

	// Calculate damage multiplier based on charge level
	float DamageMultiplier = GetDamageMultiplier(CurrentChargeLevel);
	float BaseDamage = 100.0f;
	float FinalDamage = BaseDamage * DamageMultiplier;

	// Fire the charged shot
	FireChargedShot(FinalDamage);

	// Start cooldown
	bCanFireAltGunslinger = false;
	ElapsedTime = 0.0f;

	if (RevolverHUD != nullptr)
	{
		RevolverHUD->UpdateAltFireCooldownProgress(0.0f);
	}

	GetWorld()->GetTimerManager().SetTimer(CooldownTimerHandle, this, &URevolver_WeaponComponent::HandleGunslingerAltCooldown, 0.01f, true);
}

float URevolver_WeaponComponent::GetDamageMultiplier(float ChargePercent)
{
	if (ChargePercent < 0.25f)
	{
		return 1.25f; // 25% increase
	}
	else if (ChargePercent < 0.50f)
	{
		return 1.50f; // 50% increase
	}
	else if (ChargePercent < 0.75f)
	{
		return 1.75f; // 75% increase
	}
	else
	{
		return 2.00f; // 100% increase
	}
}

void URevolver_WeaponComponent::FireChargedShot(float DamageAmount)
{
	if (!Character)
		return;

	FVector StartLocation = Character->GetActorLocation();
	FVector ForwardVector = Character->GetControlRotation().Vector();
	FVector EndLocation = StartLocation + (ForwardVector * 10000.0f); // Max range

	// Show red firing trace for the full path
	DrawDebugLine(
		GetWorld(),
		StartLocation,
		EndLocation,
		FColor::Red,
		false,
		1.0f,
		0,
		5.0f
	);

	// Manual penetration system
	FVector CurrentStart = StartLocation;
	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(Character);

	int32 MaxPenetrations = 10; // Prevent infinite loops
	int32 CurrentPenetrations = 0;

	while (CurrentPenetrations < MaxPenetrations)
	{
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActors(IgnoredActors);
		QueryParams.bTraceComplex = true;

		// Trace from current position to end
		bool bHit = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			CurrentStart,
			EndLocation,
			ECC_Pawn,
			QueryParams
		);

		if (!bHit)
		{
			// No more targets, we're done
			break;
		}

		AActor* HitActor = HitResult.GetActor();
		if (HitActor)
		{
			// Apply damage
			Character->bLastAttackWasChargedShot = true;

			UGameplayStatics::ApplyDamage(
				HitActor,
				DamageAmount,
				Character->GetController(),
				Character,
				UDamageType::StaticClass()
			);

			// Visual feedback
			DrawDebugSphere(
				GetWorld(),
				HitResult.Location,
				20.0f,
				12,
				FColor::Orange,
				false,
				1.5f
			);

			UE_LOG(LogTemp, Warning, TEXT("Penetrating shot hit: %s at distance: %f"),
				*HitActor->GetName(),
				FVector::Dist(StartLocation, HitResult.Location));

			// Add this actor to ignore list for next trace
			IgnoredActors.Add(HitActor);

			// Continue tracing from slightly past the hit point
			FVector PenetrationOffset = ForwardVector * 10.0f; // Small offset to get past the actor
			CurrentStart = HitResult.Location + PenetrationOffset;

			CurrentPenetrations++;
		}
		else
		{
			// Hit something but no actor (probably world geometry), stop here
			break;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Charged shot penetrated %d targets"), CurrentPenetrations);

	// Play enhanced effects based on charge level
	PlayChargedShotEffects(CurrentChargeLevel);
}

void URevolver_WeaponComponent::PlayChargedShotEffects(float ChargeLevel)
{
	// Play sound (could vary based on charge level)
	if (FireSound != nullptr)
	{
		float VolumeMultiplier = 1.0f + (ChargeLevel * 0.5f); // Louder for higher charges
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation(), VolumeMultiplier);
	}

	// Play animation
	if (FireAnimation != nullptr)
	{
		UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			float AnimSpeed = 1.0f + (ChargeLevel * 0.3f); // Faster animation for higher charges
			AnimInstance->Montage_Play(FireAnimation, AnimSpeed);
		}
	}

	// Spawn enhanced particle effect
	if (AltGunslingerParticle && Character)
	{
		FVector MuzzleLocation = Character->GetActorLocation() + Character->GetControlRotation().RotateVector(MuzzleOffset);
		FRotator CameraRotation = Character->GetControlRotation();
		NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), AltGunslingerParticle, MuzzleLocation);

		if (NiagaraComp)
		{
			NiagaraComp->SetWorldRotation(CameraRotation);

			// Scale particle effect based on charge level
			float EffectScale = 1.0f + (ChargeLevel * 1.0f);
			NiagaraComp->SetWorldScale3D(FVector(EffectScale));

			// You could also set Niagara parameters based on charge level
			// NiagaraComp->SetFloatParameter(TEXT("ChargeLevel"), ChargeLevel);
		}
	}
}

// Add these functions to handle input - call from your input binding
void URevolver_WeaponComponent::OnAltFirePressed()
{
	// Only start charging if we're in Gunslinger mode
	if (CurrentWeaponMode == ERevolverMode::RevolverMode1) // Gunslinger mode
	{
		AltGunslingerMode(); // This starts charging
	}
	else
	{
		// Call the original AltFire function for other modes (like Hellfire)
		AltFire();
	}
}

void URevolver_WeaponComponent::OnAltFireReleased()
{
	// Only release charged shot if we're in Gunslinger mode and actually charging
	if (CurrentWeaponMode == ERevolverMode::RevolverMode1 && bIsChargingShot)
	{
		ReleaseChargedShot(); // This fires the shot
	}
	// For other modes, do nothing on release since they fire immediately on press
}

