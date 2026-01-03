// Fill out your copyright notice in the Description page of Project Settings.


#include "GrappleComponent.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "WallRunComponent.h"
#include "GruntAIController.h"
#include "IconoclasmProjectile.h"


// Sets default values for this component's properties
UGrappleComponent::UGrappleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	IsGrappleActive = false;
	GrappleOnCooldown = false;

	OriginalFOV = 110.0f;
	GrappleFOV = 120.0f;
	InterpSpeed = 5.0f;

	SwingForce = 2000.0f;
	MaxSwingSpeed = 3000.0f;
	SwingDamping = 0.95f;
	GrappleReleaseThreshold = 300.0f;
	SwingTransitionSpeed = 1500.0f;
	MinimumPullForce = 1500.0f;

	CurrentFOV = OriginalFOV;
	TargetFOV = OriginalFOV;

	GrappleVisualMesh = nullptr;
	bWasGroundedWhenGrappleStarted = false;

	// Enemy
	GrappledActor = nullptr;
	GrabbedEnemy = nullptr;
	bIsHoldingEnemy = false;

	// NEW: projectile
	GrappledProjectile = nullptr;
	HeldProjectile = nullptr;
	bIsHoldingProjectile = false;
}

void UGrappleComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningCharacter = Cast<ACharacter>(GetOwner());

	// Get the original FOV
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (UCameraComponent* CameraComponent = OwnerCharacter->FindComponentByClass<UCameraComponent>())
		{
			OriginalFOV = CameraComponent->FieldOfView;
			CurrentFOV = OriginalFOV;
			TargetFOV = OriginalFOV;
		}
	}

	// Create the HUD
	if (GrappleHUDClass)
	{
		GrappleHUD = CreateWidget<UGrappleHUD>(GetWorld(), GrappleHUDClass);
		if (GrappleHUD)
		{
			GrappleHUD->AddToViewport();
			GrappleHUD->UpdateProgressBar(1.0f);
		}
	}

	CreateGrappleVisual();
}

void UGrappleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateGrabbedEnemyPosition();

	// NEW: if we're pulling a projectile, handle it
	if (IsGrappleActive && GrappledProjectile)
	{
		ApplyProjectileGrapplePhysics(DeltaTime);
		UpdateGrappleVisual();
	}

	if (IsGrappleActive)
	{
		// Check if grounded and release grapple if true (but only if we weren't grounded when we started)
		if (OwningCharacter && OwningCharacter->GetCharacterMovement() &&
			OwningCharacter->GetCharacterMovement()->IsMovingOnGround() &&
			!bWasGroundedWhenGrappleStarted)
		{
			ReleaseGrapple();
			return;
		}

		// Check if wall running and release grapple if true
		if (OwningCharacter)
		{
			if (UWallRunComponent* WallRunComp = OwningCharacter->FindComponentByClass<UWallRunComponent>())
			{
				if (WallRunComp->IsWallRunning)
				{
					ReleaseGrapple();
					return;
				}
			}
		}

		// Only apply player/world/enemy physics if we are NOT projectile grappling
		if (!GrappledProjectile)
		{
			ApplyCombinedGrapplePhysics(DeltaTime);
			UpdateGrappleVisual();
		}
	}

	// Interpolate FOV
	if (CurrentFOV != TargetFOV)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, InterpSpeed);

		if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
		{
			if (UCameraComponent* CameraComponent = OwnerCharacter->FindComponentByClass<UCameraComponent>())
			{
				CameraComponent->SetFieldOfView(CurrentFOV);
			}
		}
	}

	if (GrappleOnCooldown && GrappleCooldownDuration > 0.0f)
	{
		float ElapsedTime = GetWorld()->GetTimerManager().GetTimerElapsed(GrappleCooldownTimerHandle);
		float Progress = ElapsedTime / GrappleCooldownDuration;

		if (GrappleHUD)
		{
			GrappleHUD->UpdateProgressBar(Progress);
		}
	}
	else if (GrappleHUD)
	{
		GrappleHUD->UpdateProgressBar(1.0f);
	}
}

void UGrappleComponent::FireGrapple()
{
	// NEW: If holding a projectile, pressing grapple again throws it
	if (IsHoldingProjectile())
	{
		ThrowHeldProjectile();
		return;
	}

	// Can't grapple while holding an enemy
	if (bIsHoldingEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot grapple while holding an enemy! Execute or release them first."));
		return;
	}

	// If already grappling, release the grapple instead of firing a new one
	if (IsGrappleActive)
	{
		ReleaseGrapple();
		return;
	}

	if (GrappleOnCooldown || !OwningCharacter)
	{
		return;
	}

	// Store whether we were grounded when starting the grapple
	if (UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement())
	{
		bWasGroundedWhenGrappleStarted = CharacterMovement->IsMovingOnGround();
	}

	// Get the player's viewpoint
	FVector ViewPointLocation;
	FRotator ViewPointRotation;
	OwningCharacter->GetActorEyesViewPoint(ViewPointLocation, ViewPointRotation);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwningCharacter);

	// -------------------------
	// Trace for world (Visibility)
	// -------------------------
	FHitResult HitResult;
	FVector WorldEndPoint = ViewPointLocation + ViewPointRotation.Vector() * GrappleLength;
	bool bHitWorld = GetWorld()->LineTraceSingleByChannel(HitResult, ViewPointLocation, WorldEndPoint, ECC_Visibility, QueryParams);

	// -------------------------
	// Trace for enemies (Pawn)
	// -------------------------
	FHitResult EnemyHitResult;
	bool bHitEnemy = false;
	if (bCanGrappleEnemies)
	{
		FVector EnemyEndPoint = ViewPointLocation + ViewPointRotation.Vector() * EnemyGrappleRange;
		bHitEnemy = GetWorld()->LineTraceSingleByChannel(EnemyHitResult, ViewPointLocation, EnemyEndPoint, ECC_Pawn, QueryParams);
	}

	// -------------------------
	// NEW: Trace for projectiles (Object trace for WorldDynamic)
	// We use a multi trace and pick the closest AIconoclasmProjectile
	// -------------------------
	bool bHitProjectile = false;
	FHitResult ProjectileHitResult;

	{
		TArray<FHitResult> Hits;
		FCollisionObjectQueryParams ObjParams;
		ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);

		FVector ProjEndPoint = ViewPointLocation + ViewPointRotation.Vector() * ProjectileGrappleRange;

		if (GetWorld()->LineTraceMultiByObjectType(Hits, ViewPointLocation, ProjEndPoint, ObjParams, QueryParams))
		{
			float BestDist = TNumericLimits<float>::Max();
			for (const FHitResult& H : Hits)
			{
				if (AIconoclasmProjectile* P = Cast<AIconoclasmProjectile>(H.GetActor()))
				{
					float D = FVector::Dist(ViewPointLocation, H.ImpactPoint);
					if (D < BestDist)
					{
						BestDist = D;
						ProjectileHitResult = H;
						bHitProjectile = true;
					}
				}
			}
		}
	}

	// -------------------------
	// Decide closest target among: projectile, enemy, world
	// -------------------------
	enum class ETargetType { None, Projectile, Enemy, World };
	ETargetType TargetType = ETargetType::None;

	float BestDistance = TNumericLimits<float>::Max();

	if (bHitProjectile)
	{
		float D = FVector::Dist(ViewPointLocation, ProjectileHitResult.ImpactPoint);
		if (D < BestDistance)
		{
			BestDistance = D;
			TargetType = ETargetType::Projectile;
		}
	}

	if (bHitEnemy)
	{
		float D = FVector::Dist(ViewPointLocation, EnemyHitResult.ImpactPoint);
		if (D < BestDistance)
		{
			BestDistance = D;
			TargetType = ETargetType::Enemy;
		}
	}

	if (bHitWorld)
	{
		float D = FVector::Dist(ViewPointLocation, HitResult.ImpactPoint);
		if (D < BestDistance)
		{
			BestDistance = D;
			TargetType = ETargetType::World;
		}
	}

	// -------------------------
	// Execute chosen grapple
	// -------------------------
	if (TargetType == ETargetType::Projectile)
	{
		AIconoclasmProjectile* Proj = Cast<AIconoclasmProjectile>(ProjectileHitResult.GetActor());
		if (Proj && Proj->IsGrapplable())
		{
			StartProjectileGrapple(Proj);

			// cooldown + FOV + HUD + visual (same as normal grapple)
			GrappleOnCooldown = true;
			GetWorld()->GetTimerManager().SetTimer(GrappleCooldownTimerHandle, this, &UGrappleComponent::ResetGrappleCooldown, GrappleCooldownDuration, false);

			TargetFOV = GrappleFOV;
			if (GrappleHUD) GrappleHUD->UpdateProgressBar(0.0f);
			ShowGrappleVisual();

			return;
		}
	}

	if (TargetType == ETargetType::Enemy)
	{
		if (EnemyHitResult.GetActor() && EnemyHitResult.GetActor()->Tags.Contains(FName("Enemy")))
		{
			GrappledActor = EnemyHitResult.GetActor();
			GrappleLocation = EnemyHitResult.ImpactPoint;
			IsGrappleActive = true;

			StartEnemyGrapple();
		}
		else
		{
			GrappledActor = nullptr;
			GrappleLocation = EnemyHitResult.ImpactPoint;
			IsGrappleActive = true;

			if (bWasGroundedWhenGrappleStarted && OwningCharacter)
			{
				if (UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement())
				{
					CharacterMovement->SetMovementMode(MOVE_Falling);
					CharacterMovement->Launch(FVector(0, 0, 500.0f));
				}
			}

			StartWorldGrapple();
		}
	}
	else if (TargetType == ETargetType::World)
	{
		GrappledActor = nullptr;
		GrappleLocation = HitResult.ImpactPoint;
		IsGrappleActive = true;

		if (bWasGroundedWhenGrappleStarted && OwningCharacter)
		{
			if (UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement())
			{
				CharacterMovement->SetMovementMode(MOVE_Falling);
				CharacterMovement->Launch(FVector(0, 0, 500.0f));
			}
		}

		StartWorldGrapple();
	}
	else
	{
		IsGrappleActive = false;
		return;
	}

	// Common grapple setup for world/enemy
	GrappleDistance = FVector::Dist(OwningCharacter->GetActorLocation(), GrappleLocation);

	GrappleOnCooldown = true;
	GetWorld()->GetTimerManager().SetTimer(GrappleCooldownTimerHandle, this, &UGrappleComponent::ResetGrappleCooldown, GrappleCooldownDuration, false);

	TargetFOV = GrappleFOV;

	if (GrappleHUD)
	{
		GrappleHUD->UpdateProgressBar(0.0f);
	}

	ShowGrappleVisual();
}

void UGrappleComponent::StartEnemyGrapple()
{
	if (OwningCharacter && OwningCharacter->GetCharacterMovement())
	{
		OwningCharacter->GetCharacterMovement()->GravityScale = 2.0f;
	}
}

void UGrappleComponent::StartWorldGrapple()
{
	if (OwningCharacter && OwningCharacter->GetCharacterMovement())
	{
		OwningCharacter->GetCharacterMovement()->GravityScale = 0.4f;
	}
}

void UGrappleComponent::ReleaseGrapple()
{
	IsGrappleActive = false;
	GrappledActor = nullptr;

	// NEW: cancel projectile pull too
	if (GrappledProjectile)
	{
		GrappledProjectile->StopGrapplePull();
		GrappledProjectile = nullptr;
	}

	TargetFOV = OriginalFOV;

	if (OwningCharacter)
	{
		if (UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement())
		{
			CharacterMovement->GravityScale = 2.7f;
		}
	}

	HideGrappleVisual();
}

void UGrappleComponent::PullCharacterToLocation(const FVector& Location)
{
	if (!OwningCharacter) return;

	FVector CharacterLocation = OwningCharacter->GetActorLocation();
	float DistanceToLocationSquared = FVector::DistSquared(CharacterLocation, Location);
	float DistanceThresholdSquared = GrappleEndThreshold * GrappleEndThreshold;

	if (DistanceToLocationSquared <= DistanceThresholdSquared)
	{
		ReleaseGrapple();
		return;
	}

	FVector Direction = Location - CharacterLocation;
	Direction.Normalize();
	FVector Force = Direction * GrappleSpeed;

	if (UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement())
	{
		CharacterMovement->Launch(Force);
	}
}

void UGrappleComponent::ApplyCombinedGrapplePhysics(float DeltaTime)
{
	if (!OwningCharacter)
		return;

	// Enemy grapple
	if (GrappledActor && GrappledActor->Tags.Contains(FName("Enemy")))
	{
		ApplyEnemyGrapplePhysics(DeltaTime);
		return;
	}

	UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement();
	if (!CharacterMovement)
		return;

	if (CharacterMovement->MovementMode != MOVE_Falling)
	{
		CharacterMovement->SetMovementMode(MOVE_Falling);
	}

	FVector CharacterLocation = OwningCharacter->GetActorLocation();
	FVector CurrentVelocity = CharacterMovement->Velocity;

	FVector ToGrapplePoint = GrappleLocation - CharacterLocation;
	float CurrentDistance = ToGrapplePoint.Size();

	if (CurrentDistance <= GrappleEndThreshold)
	{
		ReleaseGrapple();
		return;
	}

	ToGrapplePoint.Normalize();
	float CurrentSpeed = CurrentVelocity.Size();

	if (CurrentSpeed < 500.0f)
	{
		FVector DirectPullForce = ToGrapplePoint * GrappleSpeed;
		CharacterMovement->Launch(DirectPullForce);
		return;
	}

	float BasePullStrength = FMath::Max(GrappleSpeed, MinimumPullForce);
	float AdaptivePullStrength = FMath::Max(BasePullStrength, CurrentSpeed * 1.2f);

	FVector PullForce = ToGrapplePoint * AdaptivePullStrength;
	CurrentVelocity += PullForce * DeltaTime;

	float DistanceError = CurrentDistance - GrappleDistance;
	if (DistanceError > 0)
	{
		FVector ConstraintForce = ToGrapplePoint * DistanceError * SwingForce;
		CurrentVelocity += ConstraintForce * DeltaTime;
	}

	ApplySwingInput(CurrentVelocity, ToGrapplePoint, DeltaTime);

	CurrentVelocity *= SwingDamping;

	float AdaptiveMaxSpeed = FMath::Max(MaxSwingSpeed, CurrentSpeed * 1.1f);
	if (CurrentVelocity.Size() > AdaptiveMaxSpeed)
	{
		CurrentVelocity = CurrentVelocity.GetSafeNormal() * AdaptiveMaxSpeed;
	}

	CharacterMovement->Velocity = CurrentVelocity;

	if (CurrentDistance > GrappleDistance + GrappleReleaseThreshold)
	{
		ReleaseGrapple();
	}
}

void UGrappleComponent::ApplyEnemyGrapplePhysics(float DeltaTime)
{
	if (!GrappledActor || !OwningCharacter)
	{
		ReleaseGrapple();
		return;
	}

	if (!GrappledActor->Tags.Contains(FName("Enemy")))
	{
		ReleaseGrapple();
		return;
	}

	FVector PlayerLocation = OwningCharacter->GetActorLocation();
	FVector EnemyLocation = GrappledActor->GetActorLocation();
	FVector ToPlayer = PlayerLocation - EnemyLocation;
	float DistanceToPlayer = ToPlayer.Size();

	if (DistanceToPlayer <= EnemyGrappleEndThreshold)
	{
		GrabEnemy();
		return;
	}

	ToPlayer.Normalize();
	FVector PullForce = ToPlayer * EnemyPullForce;

	if (ACharacter* EnemyCharacter = Cast<ACharacter>(GrappledActor))
	{
		if (UCharacterMovementComponent* EnemyMovement = EnemyCharacter->GetCharacterMovement())
		{
			EnemyMovement->Launch(PullForce);
		}
	}
	else
	{
		if (UPrimitiveComponent* EnemyPrimitive = Cast<UPrimitiveComponent>(GrappledActor->GetRootComponent()))
		{
			if (EnemyPrimitive->IsSimulatingPhysics())
			{
				EnemyPrimitive->AddForce(PullForce, NAME_None, true);
			}
		}
	}

	GrappleLocation = EnemyLocation;
}

void UGrappleComponent::GrabEnemy()
{
	if (!GrappledActor || !OwningCharacter)
	{
		ReleaseGrapple();
		return;
	}

	GrabbedEnemy = GrappledActor;
	bIsHoldingEnemy = true;

	IsGrappleActive = false;
	GrappledActor = nullptr;

	HideGrappleVisual();

	if (ACharacter* EnemyCharacter = Cast<ACharacter>(GrabbedEnemy))
	{
		if (UCharacterMovementComponent* EnemyMovement = EnemyCharacter->GetCharacterMovement())
		{
			EnemyMovement->DisableMovement();
		}

		EnemyCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}

	UpdateGrabbedEnemyPosition();

	UE_LOG(LogTemp, Warning, TEXT("Enemy grabbed! Press melee to execute."));
}

void UGrappleComponent::UpdateGrabbedEnemyPosition()
{
	if (!bIsHoldingEnemy || !GrabbedEnemy || !OwningCharacter)
	{
		return;
	}

	if (!IsValid(GrabbedEnemy))
	{
		bIsHoldingEnemy = false;
		GrabbedEnemy = nullptr;
		return;
	}

	FVector PlayerLocation = OwningCharacter->GetActorLocation();
	FVector PlayerForward = OwningCharacter->GetActorForwardVector();
	FVector HoldPosition = PlayerLocation + (PlayerForward * EnemyHoldDistance) + FVector(0, 0, EnemyHoldHeight);

	GrabbedEnemy->SetActorLocation(HoldPosition);

	FRotator LookAtPlayer = UKismetMathLibrary::FindLookAtRotation(GrabbedEnemy->GetActorLocation(), PlayerLocation);
	GrabbedEnemy->SetActorRotation(LookAtPlayer);
}

void UGrappleComponent::ExecuteGrabbedEnemy()
{
	if (!bIsHoldingEnemy || !GrabbedEnemy)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Executing grabbed enemy!"));

	GrabbedEnemy->Destroy();

	GrabbedEnemy = nullptr;
	bIsHoldingEnemy = false;

	TargetFOV = OriginalFOV;
}

void UGrappleComponent::StartSwinging()
{
	IsSwinging = true;

	if (OwningCharacter)
	{
		if (UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement())
		{
			CharacterMovement->GravityScale = 0.3f;
		}
	}
}

void UGrappleComponent::ApplySwingInput(FVector& CurrentVelocity, const FVector& ToGrapplePoint, float DeltaTime)
{
	if (!OwningCharacter)
		return;

	FVector InputVector = OwningCharacter->GetLastMovementInputVector();

	if (!InputVector.IsZero())
	{
		FRotator ControlRotation = OwningCharacter->GetControlRotation();
		FVector ForwardVector = ControlRotation.Vector();
		FVector RightVector = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::Y);

		FVector WorldInput = (ForwardVector * InputVector.X + RightVector * InputVector.Y);
		WorldInput.Z = 0;
		WorldInput.Normalize();

		FVector GrappleDirection = ToGrapplePoint;
		GrappleDirection.Z = 0;
		GrappleDirection.Normalize();

		FVector SwingDirection = FVector::CrossProduct(GrappleDirection, FVector::UpVector);

		float InputDot = FVector::DotProduct(WorldInput, SwingDirection);
		FVector SwingInputForce = SwingDirection * InputDot * SwingForce * 0.5f;

		CurrentVelocity += SwingInputForce * DeltaTime;
	}
}

void UGrappleComponent::ResetGrappleCooldown()
{
	GrappleOnCooldown = false;
}

// =========================
// NEW: Projectile Grapple / Hold / Throw
// =========================

void UGrappleComponent::StartProjectileGrapple(AIconoclasmProjectile* Projectile)
{
	if (!Projectile || !OwningCharacter)
		return;

	GrappledProjectile = Projectile;
	GrappledActor = nullptr;

	GrappleLocation = Projectile->GetActorLocation();
	IsGrappleActive = true;

	// Tell projectile to begin being pulled
	Projectile->StartGrapplePull(OwningCharacter);
}

void UGrappleComponent::ApplyProjectileGrapplePhysics(float DeltaTime)
{
	if (!OwningCharacter || !GrappledProjectile)
	{
		ReleaseGrapple();
		return;
	}

	if (!IsValid(GrappledProjectile) || GrappledProjectile->IsHeldByPlayer())
	{
		GrappledProjectile = nullptr;
		ReleaseGrapple();
		return;
	}

	FVector PlayerLocation = OwningCharacter->GetActorLocation();
	FVector HoldPoint = PlayerLocation + OwningCharacter->GetActorForwardVector() * 120.0f + FVector(0, 0, 60.0f);

	FVector ProjLocation = GrappledProjectile->GetActorLocation();
	float Dist = FVector::Dist(ProjLocation, HoldPoint);

	// Update grapple visual endpoint
	GrappleLocation = ProjLocation;

	// Pull it manually (projectile also self-moves in its Tick; this just makes it deterministic)
	FVector ToHold = (HoldPoint - ProjLocation);
	if (!ToHold.IsNearlyZero())
	{
		FVector Step = ToHold.GetSafeNormal() * ProjectilePullForce * DeltaTime;
		GrappledProjectile->SetActorLocation(ProjLocation + Step);
	}

	if (Dist <= ProjectileCatchDistance)
	{
		HoldProjectile(GrappledProjectile);
	}
}

void UGrappleComponent::HoldProjectile(AIconoclasmProjectile* Projectile)
{
	if (!OwningCharacter || !Projectile)
	{
		ReleaseGrapple();
		return;
	}

	// End grapple
	IsGrappleActive = false;
	GrappledProjectile = nullptr;
	HideGrappleVisual();
	TargetFOV = OriginalFOV;

	// Hold it
	HeldProjectile = Projectile;
	bIsHoldingProjectile = true;

	Projectile->AttachToPlayer(OwningCharacter, ProjectileHoldSocketName, ProjectileHoldRelativeOffset);

	UE_LOG(LogTemp, Warning, TEXT("Projectile held! Press grapple again to throw."));
}

void UGrappleComponent::ThrowHeldProjectile()
{
	if (!OwningCharacter || !HeldProjectile)
		return;

	FVector ViewLoc;
	FRotator ViewRot;
	OwningCharacter->GetActorEyesViewPoint(ViewLoc, ViewRot);

	FVector ThrowDir = ViewRot.Vector();

	AIconoclasmProjectile* Proj = HeldProjectile;

	// Clear hold state first
	HeldProjectile = nullptr;
	bIsHoldingProjectile = false;

	// Throw it (this makes it explode on any hit using bIsReflected logic)
	Proj->ThrowFromPlayer(ThrowDir, ProjectileThrowSpeed, OwningCharacter);

	UE_LOG(LogTemp, Warning, TEXT("Projectile thrown!"));
}

// =========================
// Grapple Visuals (unchanged from your code)
// =========================

void UGrappleComponent::CreateGrappleVisual()
{
	if (!OwningCharacter || !GetWorld())
	{
		return;
	}

	GrappleVisualActor = GetWorld()->SpawnActor<AStaticMeshActor>();
	if (GrappleVisualActor)
	{
		GrappleVisualMesh = GrappleVisualActor->GetStaticMeshComponent();

		UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		if (CylinderMesh && GrappleVisualMesh)
		{
			GrappleVisualMesh->SetStaticMesh(CylinderMesh);
			GrappleVisualMesh->SetMobility(EComponentMobility::Movable);
			GrappleVisualMesh->SetWorldScale3D(FVector(0.02f, 1.02f, 1.0f));
		}

		GrappleVisualActor->SetActorHiddenInGame(true);
		GrappleVisualActor->SetActorEnableCollision(false);
	}
}

void UGrappleComponent::ShowGrappleVisual()
{
	if (GrappleVisualActor)
	{
		GrappleVisualActor->SetActorHiddenInGame(false);
		UpdateGrappleVisual();
	}
}

void UGrappleComponent::HideGrappleVisual()
{
	if (GrappleVisualActor)
	{
		GrappleVisualActor->SetActorHiddenInGame(true);
	}
}

void UGrappleComponent::UpdateGrappleVisual()
{
	if (!GrappleVisualActor || !GrappleVisualMesh || !OwningCharacter)
	{
		return;
	}

	FVector StartPoint = OwningCharacter->GetActorLocation();
	FVector EndPoint = GrappleLocation;

	if (GrappledActor)
	{
		EndPoint = GrappledActor->GetActorLocation();
	}
	else if (GrappledProjectile)
	{
		EndPoint = GrappledProjectile->GetActorLocation();
	}

	FVector MidPoint = (StartPoint + EndPoint) * 0.5f;

	FVector Direction = EndPoint - StartPoint;
	float Distance = Direction.Size();
	Direction.Normalize();

	GrappleVisualActor->SetActorLocation(MidPoint);

	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(StartPoint, EndPoint);
	FRotator AdjustedRotation = LookAtRotation + FRotator(90.0f, 0.0f, 0.0f);
	GrappleVisualActor->SetActorRotation(AdjustedRotation);

	FVector Scale = GrappleVisualMesh->GetComponentScale();
	Scale.Z = Distance / 100.0f;
	GrappleVisualMesh->SetWorldScale3D(Scale);
}





