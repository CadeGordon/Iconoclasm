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



    CurrentFOV = OriginalFOV;
    TargetFOV = OriginalFOV;
    
    // Initialize grapple visual component
    GrappleVisualMesh = nullptr;

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

    if (IsGrappleActive)
    {
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
    if (GrappleOnCooldown || !OwningCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("Peyton is still super fucking faggy"));
        return;
    }

    // Get the player's viewpoint
    FVector ViewPointLocation;
    FRotator ViewPointRotation;
    OwningCharacter->GetActorEyesViewPoint(ViewPointLocation, ViewPointRotation);

    // Calculate the end point of the grapple
    FVector EndPoint = ViewPointLocation + ViewPointRotation.Vector() * GrappleLength;

    // Perform a line trace to detect hit point
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwningCharacter);
    if (GetWorld()->LineTraceSingleByChannel(HitResult, ViewPointLocation, EndPoint, ECC_Visibility, QueryParams))
    {
        // If we hit something, store the grapple location
        GrappleLocation = HitResult.ImpactPoint;
        IsGrappleActive = true;
        

        // Calculate and store the grapple distance
        GrappleDistance = FVector::Dist(OwningCharacter->GetActorLocation(), GrappleLocation);

        // Immediately start swing physics - reduce gravity for better swing feel
        if (UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement())
        {
            CharacterMovement->GravityScale = 0.4f;
        }

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
        // If the line trace does not hit anything, do not fire the grapple
        IsGrappleActive = false;
    }
    
}

void UGrappleComponent::ReleaseGrapple()
{
    IsGrappleActive = false;

    TargetFOV = OriginalFOV;

    // Restore normal gravity
    if (OwningCharacter)
    {
        if (UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement())
        {
            CharacterMovement->GravityScale = 2.0f;
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

    UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement();
    if (!CharacterMovement)
        return;

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

    // 1. PULLING FORCE - Always pulls toward grapple point
    FVector PullForce = ToGrapplePoint * GrappleSpeed;
    CurrentVelocity += PullForce * DeltaTime;

    // 2. PENDULUM CONSTRAINT - Maintains rope length for swinging
    float DistanceError = CurrentDistance - GrappleDistance;
    if (DistanceError > 0) // Only prevent stretching, allow compression
    {
        FVector ConstraintForce = ToGrapplePoint * DistanceError * SwingForce;
        CurrentVelocity += ConstraintForce * DeltaTime;
    }

    // 3. SWING INPUT - Allow player to add momentum perpendicular to rope
    ApplySwingInput(CurrentVelocity, ToGrapplePoint, DeltaTime);

    // 4. Apply damping to prevent excessive speed buildup
    CurrentVelocity *= SwingDamping;

    // 5. Clamp maximum speed
    if (CurrentVelocity.Size() > MaxSwingSpeed)
    {
        CurrentVelocity = CurrentVelocity.GetSafeNormal() * MaxSwingSpeed;
    }

    // Apply the final velocity
    CharacterMovement->Velocity = CurrentVelocity;

    // Check for auto-release if rope stretches too much
    if (CurrentDistance > GrappleDistance + GrappleReleaseThreshold)
    {
        ReleaseGrapple();
    }
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



