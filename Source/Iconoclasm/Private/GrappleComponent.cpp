// Fill out your copyright notice in the Description page of Project Settings.


#include "GrappleComponent.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"

// Sets default values for this component's properties
UGrappleComponent::UGrappleComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

    IsGrappleActive = false;
    ObjectTag = "Grabbable";
   

    

	// ...
}

// Called when the game starts
void UGrappleComponent::BeginPlay()
{
    Super::BeginPlay();

    OwningCharacter = Cast<ACharacter>(GetOwner());

}

void UGrappleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (IsGrappleActive)
    {
        PullCharacterToLocation(GrappleLocation);


    }

    PullGrapple();
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
    }
    else
    {
        // If the line trace does not hit anything, do not fire the grapple
        IsGrappleActive = false;
    }

    DrawDebugLine(GetWorld(), ViewPointLocation, GrappleLocation, FColor::Green, false, 5.0f, 0, 5.0f);

    GrappleOnCooldown = true;
    GetWorld()->GetTimerManager().SetTimer(GrappleCooldownTimerHandle, this, &UGrappleComponent::ResetGrappleCooldown, GrappleCooldownDuration, false);
}

void UGrappleComponent::ReleaseGrapple()
{
    IsGrappleActive = false;

}

void UGrappleComponent::PullGrapple()
{
    if (!OwningCharacter)
    {
        return;
    }

    FVector CharacterLocation = OwningCharacter->GetActorLocation();

    // Perform a sphere trace to detect pullable objects
    TArray<FHitResult> HitResults;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwningCharacter);
    GetWorld()->SweepMultiByChannel(HitResults, CharacterLocation, CharacterLocation, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(PullRadius), QueryParams);

    // Iterate through each hit result and pull pullable objects towards the player's location
    for (const FHitResult& HitResult : HitResults)
    {
        AActor* HitActor = HitResult.GetActor();
        if (HitActor && HitActor->ActorHasTag("Pullable"))
        {
            FVector ObjectLocation = HitActor->GetActorLocation();
            FVector Direction = CharacterLocation - ObjectLocation;
            Direction.Normalize();

            // Apply a force to pull the object towards the player's location
            UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(HitActor->GetRootComponent());
            if (PrimitiveComponent && HitActor != OwningCharacter) // Exclude the player character from being affected
            {
                float DistanceSquared = FVector::DistSquared(CharacterLocation, ObjectLocation);
                float ForceMagnitude = PullForce * FMath::Clamp(1.0f - DistanceSquared / (PullRadius * PullRadius), 0.0f, 1.0f);
                FVector Force = Direction * ForceMagnitude;
                PrimitiveComponent->AddForce(Force, NAME_None, bUseVelocityChange);
            }
        }
    }
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









