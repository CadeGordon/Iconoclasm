// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPack.h"
#include "TimerManager.h"

// Sets default values
AHealthPack::AHealthPack()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    // Create and set up the collision component
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    RootComponent = CollisionComponent;
    CollisionComponent->SetSphereRadius(100.0f);
    CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AHealthPack::OnOverlapBegin);

    // Create and set up the mesh component
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(RootComponent);

}

// Called when the game starts or when spawned
void AHealthPack::BeginPlay()
{
	Super::BeginPlay();


    // Set a timer to destroy this health pack after 5 seconds
    GetWorldTimerManager().SetTimer(
        DestroyTimerHandle,
        this,
        &AHealthPack::DestroyHealthPack,
        5.0f,
        false
    );
	
}

// Called every frame
void AHealthPack::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    // Move towards the player if close enough
    ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (Player && FVector::Dist(GetActorLocation(), Player->GetActorLocation()) < 500.0f) // Adjust pull distance
    {
        FVector Direction = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        SetActorLocation(GetActorLocation() + Direction * PullSpeed * DeltaTime);
    }
}

// When player overlaps, heal and destroy the health pack
void AHealthPack::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    AIconoclasmCharacter* Player = Cast<AIconoclasmCharacter>(OtherActor);
    if (Player)
    {
        // Heal the player
        UHealthComponent* PlayerHealth = Player->FindComponentByClass<UHealthComponent>();
        if (PlayerHealth)
        {
            PlayerHealth->Heal(HealAmount);
        }

        // Clear the timer before destroying (to avoid calling Destroy twice)
        GetWorldTimerManager().ClearTimer(DestroyTimerHandle);

        // Destroy the health pack
        Destroy();
    }
}

void AHealthPack::DestroyHealthPack()
{
    Destroy();
}