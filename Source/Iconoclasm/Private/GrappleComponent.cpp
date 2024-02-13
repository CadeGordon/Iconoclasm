// Fill out your copyright notice in the Description page of Project Settings.


#include "GrappleComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"

// Sets default values for this component's properties
UGrappleComponent::UGrappleComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

    IsGrappleActive = false;

	// ...
}

void UGrappleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (IsGrappleActive)
    {
        PullCharacterToLocation(OwningCharacter->GetActorLocation());
    }
}

void UGrappleComponent::FireGrapple()
{
    if (!OwningCharacter)
    {
        return;
    }

    IsGrappleActive = true;

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
    GetWorld()->LineTraceSingleByChannel(HitResult, ViewPointLocation, EndPoint, ECC_Visibility, QueryParams);

    // Draw debug line
    DrawDebugLine(GetWorld(), ViewPointLocation, HitResult.bBlockingHit ? HitResult.ImpactPoint : EndPoint, FColor::Green, false, 0.0f, 0, 10.0f);


    // If we hit something, pull the character to that location
    if (HitResult.bBlockingHit)
    {
        PullCharacterToLocation(HitResult.ImpactPoint);
    }
}

void UGrappleComponent::ReleaseGrapple()
{
    IsGrappleActive = false;
}

void UGrappleComponent::PullCharacterToLocation(const FVector& Location)
{
    if (!OwningCharacter)
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
    GetWorld()->LineTraceSingleByChannel(HitResult, ViewPointLocation, EndPoint, ECC_Visibility, QueryParams);

    // If we hit something, pull the character to that location
    if (HitResult.bBlockingHit)
    {
        FVector Direction = HitResult.ImpactPoint - OwningCharacter->GetActorLocation();
        Direction.Normalize();
        FVector Force = Direction * GrappleSpeed;

        // Apply the force to the character's movement component
        UCharacterMovementComponent* CharacterMovement = OwningCharacter->GetCharacterMovement();
        if (CharacterMovement)
        {
            CharacterMovement->Launch(Force);
        }
    }
}


// Called when the game starts
void UGrappleComponent::BeginPlay()
{
	Super::BeginPlay();

    OwningCharacter = Cast<ACharacter>(GetOwner());
	
}




