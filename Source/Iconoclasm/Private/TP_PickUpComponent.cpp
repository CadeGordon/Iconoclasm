// Copyright Epic Games, Inc. All Rights Reserved.

#include "TP_PickUpComponent.h"
#include "IconoclasmCharacter.h"
#include "TP_WeaponComponent.h"

UTP_PickUpComponent::UTP_PickUpComponent()
{
	// Setup the Sphere Collision
	SphereRadius = 32.f;
}

void UTP_PickUpComponent::BeginPlay()
{
	Super::BeginPlay();

	// Register our Overlap Event
	OnComponentBeginOverlap.AddDynamic(this, &UTP_PickUpComponent::OnSphereBeginOverlap);
}

void UTP_PickUpComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    UE_LOG(LogTemp, Warning, TEXT("Overlap event triggered!"));  // Debug log

    // Checking if it is a First Person Character overlapping
    AIconoclasmCharacter* Character = Cast<AIconoclasmCharacter>(OtherActor);
    if (Character != nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Character overlapped with weapon!"));  // Debug log

        // Get the weapon component attached to this actor
        UTP_WeaponComponent* LocalWeaponComponent = GetOwner()->FindComponentByClass<UTP_WeaponComponent>();

        if (LocalWeaponComponent != nullptr)
        {
            if (!Character->HasWeaponEquipped())
            {
                OnPickUp.Broadcast(Character);
                Character->EquipWeapon(LocalWeaponComponent);
            }
            else
            {
                Character->AddWeaponToInventory(LocalWeaponComponent);
            }

            OnComponentBeginOverlap.RemoveAll(this);

            
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("WeaponComponent not found on actor!"));  // Debug log
        }
    }
}
