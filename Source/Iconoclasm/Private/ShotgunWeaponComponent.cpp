// Fill out your copyright notice in the Description page of Project Settings.


#include "ShotgunWeaponComponent.h"
#include "IconoclasmCharacter.h"
#include "TP_WeaponComponent.h"
#include "IconoclasmProjectile.h"
#include "GrenadeLauncherHealProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

UShotgunWeaponComponent::UShotgunWeaponComponent()
{
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
	CurrentWeaponMode = EShotgunMode::ShotgunMode1;
}

void UShotgunWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<AIconoclasmCharacter>(GetOwner());
}

void UShotgunWeaponComponent::AttachWeapon(AIconoclasmCharacter* TargetCharacter)
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
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &UShotgunWeaponComponent::Fire);
			EnhancedInputComponent->BindAction(AltFireAction, ETriggerEvent::Triggered, this, &UShotgunWeaponComponent::AltFire);
			EnhancedInputComponent->BindAction(SwitchFireModeAction, ETriggerEvent::Triggered, this, &UShotgunWeaponComponent::SwitchFireMode);
		}
	}
}

void UShotgunWeaponComponent::Fire()
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	switch (CurrentWeaponMode)
	{
	case EShotgunMode::ShotgunMode1:
		PumpActionMode();
		break;
	case EShotgunMode::ShotgunMode2:
		SemiAutoMode();
		break;
	default:
		break;
	}
}

void UShotgunWeaponComponent::AltFire()
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	switch (CurrentWeaponMode)
	{
	case EShotgunMode::ShotgunMode1:
		AltPumpActionMode();
		break;
	case EShotgunMode::ShotgunMode2:
		AltSemiAutoMode();
		break;
	default:
		break;
	}
}

void UShotgunWeaponComponent::SwitchFireMode()
{
	// Cycle through the weapon modes
	CurrentWeaponMode = static_cast<EShotgunMode>((static_cast<uint8>(CurrentWeaponMode) + 1) % (static_cast<uint8>(EShotgunMode::ShotgunMode2) + 1));
}

void UShotgunWeaponComponent::PerformHitscan(FVector& ImpactLocation)
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

void UShotgunWeaponComponent::PerformPumpHitscan(FVector& ImpactLocation, FVector& StartLocation, FVector& EndLocation)
{
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

	// Draw debug line for visualization
	DrawDebugLine(GetWorld(), StartLocation, ImpactLocation, FColor::Red, false, 2.0f, 0, 1.0f);
}

void UShotgunWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

void UShotgunWeaponComponent::PumpActionMode()
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
            FVector ImpactLocation;

            PerformPumpHitscan(ImpactLocation, StartLocation, EndLocation);

            // Apply effects or damage to hit targets
            // For example: UGameplayStatics::ApplyDamage(HitActor, DamageAmount, PlayerController, this, DamageTypeClass);
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

void UShotgunWeaponComponent::AltPumpActionMode()
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

void UShotgunWeaponComponent::SemiAutoMode()
{
}

void UShotgunWeaponComponent::AltSemiAutoMode()
{
}
