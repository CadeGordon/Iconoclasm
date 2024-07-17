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

// Sets default values for this component's properties
UTP_WeaponComponent::UTP_WeaponComponent()
{
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
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

	FVector ImpactLocation;
	PerformHitscan(ImpactLocation);
	ApplyExplosionEffect(ImpactLocation, 300.0f, 2000.0f); // Example radius and strength

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

void UTP_WeaponComponent::AltFire()
{
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	FVector ImpactLocation;
	PerformHitscan(ImpactLocation);
	ApplyExplosionEffect(ImpactLocation, 500.0f, 3000.0f); // Different radius and strength for alt fire

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