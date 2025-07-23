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
        PullCharacterToLocation(GrappleLocation);
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



