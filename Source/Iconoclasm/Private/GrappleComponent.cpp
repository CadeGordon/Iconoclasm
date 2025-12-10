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


// Sets default values for this component's properties
UGrappleComponent::UGrappleComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

    IsGrappleActive = false;
    GrappleOnCooldown = false;


    // Set default values for FOV and interpolation speed
    OriginalFOV = 110.0f;
    GrappleFOV = 120.0f;
    InterpSpeed = 5.0f;

    // Swing parameters
    SwingForce = 2000.0f;
    MaxSwingSpeed = 3000.0f;
    SwingDamping = 0.95f;
    GrappleReleaseThreshold = 300.0f; // Distance threshold for auto-release
    SwingTransitionSpeed = 1500.0f; // Speed threshold to transition from pull to swing
    MinimumPullForce = 1500.0f;



    CurrentFOV = OriginalFOV;
    TargetFOV = OriginalFOV;
    
    // Initialize grapple visual component
    GrappleVisualMesh = nullptr;

    bWasGroundedWhenGrappleStarted = false;

	// ...
}

// Called when the game starts
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
            CurrentFOV = OriginalFOV; // Initialize CurrentFOV to OriginalFOV
            TargetFOV = OriginalFOV; // Initialize TargetFOV to OriginalFOV
        }
    }

    // Create the HUD
    if (GrappleHUDClass)
    {
        GrappleHUD = CreateWidget<UGrappleHUD>(GetWorld(), GrappleHUDClass);
        if (GrappleHUD)
        {
            GrappleHUD->AddToViewport();
            GrappleHUD->UpdateProgressBar(1.0f); // Set full progress initially
        }
    }

    // Create grapple visual mesh component
    CreateGrappleVisual();
}

void UGrappleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UpdateGrabbedEnemyPosition();

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
            // Try to find the wallrun component
            if (UWallRunComponent* WallRunComp = OwningCharacter->FindComponentByClass<UWallRunComponent>())
            {
                if (WallRunComp->IsWallRunning)
                {
                    ReleaseGrapple();
                    return;
                }
            }
        }

        // Apply both pulling and swinging physics simultaneously
        ApplyCombinedGrapplePhysics(DeltaTime);
        UpdateGrappleVisual();
    }

    // Interpolate FOV
    if (CurrentFOV != TargetFOV)
    {
        CurrentFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, InterpSpeed);

        // Apply FOV to camera component
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
        // Calculate recharge progress
        float ElapsedTime = GetWorld()->GetTimerManager().GetTimerElapsed(GrappleCooldownTimerHandle);
        float Progress = ElapsedTime / GrappleCooldownDuration;

        // Update HUD progress bar
        if (GrappleHUD)
        {
            GrappleHUD->UpdateProgressBar(Progress);
        }
    }
    else if (GrappleHUD)
    {
        // Ensure progress bar is full when not on cooldown
        GrappleHUD->UpdateProgressBar(1.0f);
    }


 
}

void UGrappleComponent::FireGrapple()
{

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

    // Calculate the end point of the grapple
    FVector EndPoint = ViewPointLocation + ViewPointRotation.Vector() * GrappleLength;

    // Perform line trace for world geometry first
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwningCharacter);

    bool bHitWorld = GetWorld()->LineTraceSingleByChannel(HitResult, ViewPointLocation, EndPoint, ECC_Visibility, QueryParams);

    // Perform a separate trace for enemies using the Pawn collision channel
    FHitResult EnemyHitResult;
    FCollisionQueryParams EnemyQueryParams;
    EnemyQueryParams.AddIgnoredActor(OwningCharacter);
    bool bHitEnemy = false;

    if (bCanGrappleEnemies)
    {
        // Use a shorter range for enemy grappling
        FVector EnemyEndPoint = ViewPointLocation + ViewPointRotation.Vector() * EnemyGrappleRange;
        bHitEnemy = GetWorld()->LineTraceSingleByChannel(EnemyHitResult, ViewPointLocation, EnemyEndPoint, ECC_Pawn, EnemyQueryParams);
    }

    // Determine which target to grapple to
    FHitResult* TargetHit = nullptr;
    bool bGrapplingEnemy = false;

    if (bHitEnemy && bHitWorld)
    {
        // If we hit both, choose the closer one
        float EnemyDistance = FVector::Dist(ViewPointLocation, EnemyHitResult.ImpactPoint);
        float WorldDistance = FVector::Dist(ViewPointLocation, HitResult.ImpactPoint);

        if (EnemyDistance < WorldDistance)
        {
            TargetHit = &EnemyHitResult;
            bGrapplingEnemy = true;
        }
        else
        {
            TargetHit = &HitResult;
        }
    }
    else if (bHitEnemy)
    {
        TargetHit = &EnemyHitResult;
        bGrapplingEnemy = true;
    }
    else if (bHitWorld)
    {
        TargetHit = &HitResult;
    }

    if (TargetHit)
    {
        // Check if we're grappling an enemy
        if (bGrapplingEnemy && TargetHit->GetActor())
        {
            // Verify the hit actor has the "Enemy" tag
            if (TargetHit->GetActor()->Tags.Contains(FName("Enemy")))
            {
                GrappledActor = TargetHit->GetActor();
                GrappleLocation = TargetHit->ImpactPoint;
                IsGrappleActive = true;

                // For enemy grappling, pull the enemy toward the player instead of player toward enemy
                StartEnemyGrapple();
            }
            else
            {
                // Not a valid enemy, treat as world geometry
                GrappledActor = nullptr;
                GrappleLocation = TargetHit->ImpactPoint;
                IsGrappleActive = true;

                // Force character off ground if grounded (for world grapple only)
                if (bWasGroundedWhenGrappleStarted && OwningCharacter)
                {
                    if (UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement())
                    {
                        CharacterMovement->SetMovementMode(MOVE_Falling);
                        FVector LaunchVelocity = FVector(0, 0, 500.0f);
                        CharacterMovement->Launch(LaunchVelocity);
                    }
                }

                StartWorldGrapple();
            }
        }
        else
        {
            // Regular world grappling
            GrappledActor = nullptr;
            GrappleLocation = TargetHit->ImpactPoint;
            IsGrappleActive = true;

            // Force character off ground if grounded (for world grapple only)
            if (bWasGroundedWhenGrappleStarted && OwningCharacter)
            {
                if (UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement())
                {
                    CharacterMovement->SetMovementMode(MOVE_Falling);
                    FVector LaunchVelocity = FVector(0, 0, 500.0f);
                    CharacterMovement->Launch(LaunchVelocity);
                }
            }

            StartWorldGrapple();
        }

        // Common grapple setup
        GrappleDistance = FVector::Dist(OwningCharacter->GetActorLocation(), GrappleLocation);

        GrappleOnCooldown = true;
        GetWorld()->GetTimerManager().SetTimer(GrappleCooldownTimerHandle, this, &UGrappleComponent::ResetGrappleCooldown, GrappleCooldownDuration, false);

        // Set target FOV for when grapple is active
        TargetFOV = GrappleFOV;

        // Set progress to zero immediately
        if (GrappleHUD)
        {
            GrappleHUD->UpdateProgressBar(0.0f);
        }

        // Show grapple visual
        ShowGrappleVisual();
    }
    else
    {
        // If no trace hits anything, do not fire the grapple
        IsGrappleActive = false;
    }
    
}

// New function to handle enemy grappling
void UGrappleComponent::StartEnemyGrapple()
{
    // For enemy grappling, we pull the enemy toward us rather than us toward the enemy
    // Set different physics parameters if needed
    if (OwningCharacter && OwningCharacter->GetCharacterMovement())
    {
        OwningCharacter->GetCharacterMovement()->GravityScale = 2.0f;
    }
}

// New function to handle world grappling  
void UGrappleComponent::StartWorldGrapple()
{
    // Standard world grappling setup
    if (OwningCharacter && OwningCharacter->GetCharacterMovement())
    {
        OwningCharacter->GetCharacterMovement()->GravityScale = 0.4f;
    }
}

void UGrappleComponent::ReleaseGrapple()
{
    IsGrappleActive = false;
    GrappledActor = nullptr;

    TargetFOV = OriginalFOV;

    // Restore normal gravity
    if (OwningCharacter)
    {
        if (UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement())
        {
            CharacterMovement->GravityScale = 2.7f;
        }
    }

    // Hide grapple visual
    HideGrappleVisual();

}

void UGrappleComponent::PullCharacterToLocation(const FVector& Location)
{
    if (!OwningCharacter)
    {
        return;
    }

    FVector CharacterLocation = OwningCharacter->GetActorLocation();
    float DistanceToLocationSquared = FVector::DistSquared(CharacterLocation, Location);
    float DistanceThresholdSquared = GrappleEndThreshold * GrappleEndThreshold; // Adjust this threshold as needed

    if (DistanceToLocationSquared <= DistanceThresholdSquared)
    {
        ReleaseGrapple();
        return;
    }

    FVector Direction = Location - CharacterLocation;
    Direction.Normalize();
    FVector Force = Direction * GrappleSpeed;

    UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement();
    if (CharacterMovement)
    {
        CharacterMovement->Launch(Force);
    }
}

void UGrappleComponent::ApplyCombinedGrapplePhysics(float DeltaTime)
{
    if (!OwningCharacter)
        return;

    // If we're grappling an enemy, handle enemy pulling
    if (GrappledActor && GrappledActor->Tags.Contains(FName("Enemy")))
    {
        ApplyEnemyGrapplePhysics(DeltaTime);
        return; // Exit early - don't apply world grapple physics for enemies
    }

    // Only apply falling mode and world grapple physics for non-enemy grapples
    UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement();
    if (!CharacterMovement)
        return;

    // NEW: Ensure we stay in falling mode during world grapple
    if (CharacterMovement->MovementMode != MOVE_Falling)
    {
        CharacterMovement->SetMovementMode(MOVE_Falling);
    }

    FVector CharacterLocation = OwningCharacter->GetActorLocation();
    FVector CurrentVelocity = CharacterMovement->Velocity;

    // Calculate vector from character to grapple point
    FVector ToGrapplePoint = GrappleLocation - CharacterLocation;
    float CurrentDistance = ToGrapplePoint.Size();

    // Check if we're close enough to release
    if (CurrentDistance <= GrappleEndThreshold)
    {
        ReleaseGrapple();
        return;
    }

    ToGrapplePoint.Normalize();
    float CurrentSpeed = CurrentVelocity.Size();

    // MODIFIED: Use direct pull for slower speeds OR if we just started grappling from ground
    // This prevents the "dragging" effect
    if (CurrentSpeed < 500.0f) // Increased threshold
    {
        // Use the original direct pull method
        FVector DirectPullForce = ToGrapplePoint * GrappleSpeed;
        CharacterMovement->Launch(DirectPullForce);
        return;
    }

    // For moving players, apply FULL POWER swing physics
    float BasePullStrength = FMath::Max(GrappleSpeed, MinimumPullForce);
    float AdaptivePullStrength = FMath::Max(BasePullStrength, CurrentSpeed * 1.2f);

    // 1. PULLING FORCE - Adaptive strength that scales with player momentum
    FVector PullForce = ToGrapplePoint * AdaptivePullStrength;
    CurrentVelocity += PullForce * DeltaTime;

    // 2. PENDULUM CONSTRAINT - Strong constraint to maintain rope length
    float DistanceError = CurrentDistance - GrappleDistance;
    if (DistanceError > 0)
    {
        FVector ConstraintForce = ToGrapplePoint * DistanceError * SwingForce;
        CurrentVelocity += ConstraintForce * DeltaTime;
    }

    // 3. SWING INPUT - Full power swing input
    ApplySwingInput(CurrentVelocity, ToGrapplePoint, DeltaTime);

    // 4. Apply minimal damping to maintain momentum
    CurrentVelocity *= SwingDamping;

    // 5. Adaptive max swing speed
    float AdaptiveMaxSpeed = FMath::Max(MaxSwingSpeed, CurrentSpeed * 1.1f);
    if (CurrentVelocity.Size() > AdaptiveMaxSpeed)
    {
        CurrentVelocity = CurrentVelocity.GetSafeNormal() * AdaptiveMaxSpeed;
    }

    // Apply the final velocity
    CharacterMovement->Velocity = CurrentVelocity;

    // Check for auto-release if rope stretches too much
    if (CurrentDistance > GrappleDistance + GrappleReleaseThreshold)
    {
        ReleaseGrapple();
    }
}

// New function to handle enemy grapple physics
void UGrappleComponent::ApplyEnemyGrapplePhysics(float DeltaTime)
{
    if (!GrappledActor || !OwningCharacter)
    {
        ReleaseGrapple();
        return;
    }

    // Check if the enemy is still valid and has the Enemy tag
    if (!GrappledActor->Tags.Contains(FName("Enemy")))
    {
        ReleaseGrapple();
        return;
    }

    FVector PlayerLocation = OwningCharacter->GetActorLocation();
    FVector EnemyLocation = GrappledActor->GetActorLocation();
    FVector ToPlayer = PlayerLocation - EnemyLocation;
    float DistanceToPlayer = ToPlayer.Size();

    // Release if enemy gets too close
    if (DistanceToPlayer <= EnemyGrappleEndThreshold)
    {
        GrabEnemy();
        return;
    }

    // Pull the enemy toward the player
    ToPlayer.Normalize();
    FVector PullForce = ToPlayer * EnemyPullForce;

    // Try to get the enemy's character movement component
    if (ACharacter* EnemyCharacter = Cast<ACharacter>(GrappledActor))
    {
        if (UCharacterMovementComponent* EnemyMovement = EnemyCharacter->GetCharacterMovement())
        {
            // Launch the enemy toward the player
            EnemyMovement->Launch(PullForce);
        }
    }
    else
    {
        // If it's not a character, try to move it using physics
        if (UPrimitiveComponent* EnemyPrimitive = Cast<UPrimitiveComponent>(GrappledActor->GetRootComponent()))
        {
            if (EnemyPrimitive->IsSimulatingPhysics())
            {
                EnemyPrimitive->AddForce(PullForce, NAME_None, true);
            }
        }
    }

    // Update grapple location to enemy's current position
    GrappleLocation = EnemyLocation;
}

// New function to grab and hold the enemy
void UGrappleComponent::GrabEnemy()
{
    if (!GrappledActor || !OwningCharacter)
    {
        ReleaseGrapple();
        return;
    }

    // Store the grabbed enemy
    GrabbedEnemy = GrappledActor;
    bIsHoldingEnemy = true;

    // End the grapple phase
    IsGrappleActive = false;
    GrappledActor = nullptr;

    // Hide grapple visual
    HideGrappleVisual();

    // Disable enemy AI and physics
    if (ACharacter* EnemyCharacter = Cast<ACharacter>(GrabbedEnemy))
    {
        //// Disable AI
        //if (AGruntAIController* GruntAIController = Cast<AGruntAIController>(EnemyCharacter->GetController()))
        //{
        //    GruntAIController->GetBrainComponent()->StopLogic("Grabbed");
        //}

        // Disable movement
        if (UCharacterMovementComponent* EnemyMovement = EnemyCharacter->GetCharacterMovement())
        {
            EnemyMovement->DisableMovement();
        }

        // Disable collision with the player
        EnemyCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    }

    // Position enemy in front of player
    UpdateGrabbedEnemyPosition();

    UE_LOG(LogTemp, Warning, TEXT("Enemy grabbed! Press melee to execute."));
}

// Update the grabbed enemy's position every frame
void UGrappleComponent::UpdateGrabbedEnemyPosition()
{
    if (!bIsHoldingEnemy || !GrabbedEnemy || !OwningCharacter)
    {
        //ReleaseGrabbedEnemy();
        return;
    }

    // Check if the enemy is still valid
    if (!IsValid(GrabbedEnemy))
    {
        bIsHoldingEnemy = false;
        GrabbedEnemy = nullptr;
        return;
    }

    // Calculate position in front of player
    FVector PlayerLocation = OwningCharacter->GetActorLocation();
    FVector PlayerForward = OwningCharacter->GetActorForwardVector();
    FVector HoldPosition = PlayerLocation + (PlayerForward * EnemyHoldDistance) + FVector(0, 0, EnemyHoldHeight);

    // Smoothly interpolate to the hold position for better visuals
    FVector CurrentEnemyLocation = GrabbedEnemy->GetActorLocation();
    FVector NewLocation = FMath::VInterpTo(CurrentEnemyLocation, HoldPosition, GetWorld()->GetDeltaSeconds(), 15.0f);

    // Set enemy location
    GrabbedEnemy->SetActorLocation(HoldPosition);

    // Make enemy face the player
    FRotator LookAtPlayer = UKismetMathLibrary::FindLookAtRotation(GrabbedEnemy->GetActorLocation(), PlayerLocation);
    GrabbedEnemy->SetActorRotation(LookAtPlayer);
}

// Execute (destroy) the grabbed enemy - call this from melee input
void UGrappleComponent::ExecuteGrabbedEnemy()
{
    if (!bIsHoldingEnemy || !GrabbedEnemy)
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Executing grabbed enemy!"));

    // Optional: Spawn particles, play sound, etc. here
    // You could also play an execution animation on the player

    // Destroy the enemy
    GrabbedEnemy->Destroy();

    // Clear references
    GrabbedEnemy = nullptr;
    bIsHoldingEnemy = false;

    // Restore normal FOV if it was changed
    TargetFOV = OriginalFOV;
}


//bool UGrappleComponent::ShouldTransitionToSwing()
//{
//    if (!OwningCharacter)
//        return false;
//
//    // Get player's current velocity
//    FVector CurrentVelocity = OwningCharacter->GetCharacterMovement()->Velocity;
//    float CurrentSpeed = CurrentVelocity.Size();
//
//    // Get direction to grapple point
//    FVector ToGrapplePoint = (GrappleLocation - OwningCharacter->GetActorLocation()).GetSafeNormal();
//
//    // Check if player has sufficient speed and isn't moving directly toward grapple point
//    float DotProduct = FVector::DotProduct(CurrentVelocity.GetSafeNormal(), ToGrapplePoint);
//
//    // Transition to swing if:
//    // 1. Player has enough speed
//    // 2. Player isn't moving directly toward the grapple point (allows for pendulum motion)
//    // 3. Player has some horizontal velocity component
//    return CurrentSpeed > SwingTransitionSpeed &&
//        DotProduct < 0.8f &&
//        FMath::Abs(CurrentVelocity.Z) < CurrentSpeed * 0.8f;
//}

void UGrappleComponent::StartSwinging()
{
    IsSwinging = true;

    if (OwningCharacter)
    {
        // Disable gravity temporarily for smoother swinging
        UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement();
        if (CharacterMovement)
        {
            CharacterMovement->GravityScale = 0.3f; // Reduce gravity for swing feel
        }
    }
}

//void UGrappleComponent::ApplySwingPhysics(float DeltaTime)
//{
//    if (!OwningCharacter)
//        return;
//
//    UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement();
//    if (!CharacterMovement)
//        return;
//
//    FVector CharacterLocation = OwningCharacter->GetActorLocation();
//    FVector CurrentVelocity = CharacterMovement->Velocity;
//
//    // Calculate vector from character to grapple point
//    FVector ToGrapplePoint = GrappleLocation - CharacterLocation;
//    float CurrentDistance = ToGrapplePoint.Size();
//    ToGrapplePoint.Normalize();
//
//    // Constraint force to maintain grapple distance (pendulum constraint)
//    float DistanceError = CurrentDistance - GrappleDistance;
//    FVector ConstraintForce = ToGrapplePoint * DistanceError * SwingForce;
//
//    // Only apply inward constraint force (prevent stretching, allow compression)
//    if (DistanceError > 0)
//    {
//        CurrentVelocity += ConstraintForce * DeltaTime;
//    }
//
//    // Apply player input for swinging control
//    ApplySwingInput(CurrentVelocity, ToGrapplePoint, DeltaTime);
//
//    // Apply damping to prevent infinite swinging
//    CurrentVelocity *= SwingDamping;
//
//    // Clamp maximum swing speed
//    if (CurrentVelocity.Size() > MaxSwingSpeed)
//    {
//        CurrentVelocity = CurrentVelocity.GetSafeNormal() * MaxSwingSpeed;
//    }
//
//    // Apply the velocity
//    CharacterMovement->Velocity = CurrentVelocity;
//
//    // Check for release conditions
//    if (CurrentDistance > GrappleDistance + GrappleReleaseThreshold)
//    {
//        ReleaseGrapple();
//
//        // Restore normal gravity
//        CharacterMovement->GravityScale = 1.0f;
//    }
//}

void UGrappleComponent::ApplySwingInput(FVector& CurrentVelocity, const FVector& ToGrapplePoint, float DeltaTime)
{
    if (!OwningCharacter)
        return;

    // Get player input
    FVector InputVector = OwningCharacter->GetLastMovementInputVector();

    if (!InputVector.IsZero())
    {
        // Convert input to world space
        FRotator ControlRotation = OwningCharacter->GetControlRotation();
        FVector ForwardVector = ControlRotation.Vector();
        FVector RightVector = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::Y);

        FVector WorldInput = (ForwardVector * InputVector.X + RightVector * InputVector.Y);
        WorldInput.Z = 0; // Remove vertical component for horizontal swing control
        WorldInput.Normalize();

        // Apply swing input force perpendicular to grapple line
        FVector GrappleDirection = ToGrapplePoint;
        GrappleDirection.Z = 0; // Project to horizontal plane
        GrappleDirection.Normalize();

        // Get perpendicular direction for swinging
        FVector SwingDirection = FVector::CrossProduct(GrappleDirection, FVector::UpVector);

        // Apply input in swing direction
        float InputDot = FVector::DotProduct(WorldInput, SwingDirection);
        FVector SwingInputForce = SwingDirection * InputDot * SwingForce * 0.5f;

        CurrentVelocity += SwingInputForce * DeltaTime;
    }
}


void UGrappleComponent::ResetGrappleCooldown()
{
    GrappleOnCooldown = false;
}

void UGrappleComponent::CreateGrappleVisual()
{
    if (!OwningCharacter || !GetWorld())
    {
        return;
    }

    // Create a static mesh actor for the grapple line
    GrappleVisualActor = GetWorld()->SpawnActor<AStaticMeshActor>();
    if (GrappleVisualActor)
    {
        GrappleVisualMesh = GrappleVisualActor->GetStaticMeshComponent();

        // Load a default cylinder mesh (you can replace this with your own mesh)
        UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        if (CylinderMesh && GrappleVisualMesh)
        {
            GrappleVisualMesh->SetStaticMesh(CylinderMesh);

            GrappleVisualMesh->SetMobility(EComponentMobility::Movable);

            // Create a material for the grapple line (optional - you can set this in Blueprint)
            // Make it thin like a rope/cable
            GrappleVisualMesh->SetWorldScale3D(FVector(0.02f, 1.02f, 1.0f)); // Thin cylinder
        }

        // Initially hide the visual
        GrappleVisualActor->SetActorHiddenInGame(true);
        GrappleVisualActor->SetActorEnableCollision(false); // Disable collision for visual
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

    // Get start and end points
    FVector StartPoint = OwningCharacter->GetActorLocation();
    FVector EndPoint = GrappleLocation;

    // If we're grappling an enemy, use their current location
    if (GrappledActor)
    {
        EndPoint = GrappledActor->GetActorLocation();
    }
    else
    {
        EndPoint = GrappleLocation;
    }

    // Calculate midpoint
    FVector MidPoint = (StartPoint + EndPoint) * 0.5f;

    // Calculate distance and direction
    FVector Direction = EndPoint - StartPoint;
    float Distance = Direction.Size();
    Direction.Normalize();

    // Position the visual at midpoint
    GrappleVisualActor->SetActorLocation(MidPoint);

    // Rotate to point from start to end
    FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(StartPoint, EndPoint);
    // Adjust rotation because cylinder's default orientation is Z-up, but we want it along the grapple line
    FRotator AdjustedRotation = LookAtRotation + FRotator(90.0f, 0.0f, 0.0f);
    GrappleVisualActor->SetActorRotation(AdjustedRotation);

    // Scale the cylinder to match the distance
    FVector Scale = GrappleVisualMesh->GetComponentScale();
    Scale.Z = Distance / 100.0f; // Adjust the divisor based on your mesh size
    GrappleVisualMesh->SetWorldScale3D(Scale);
}





