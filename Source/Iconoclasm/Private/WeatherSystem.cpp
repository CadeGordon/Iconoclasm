// Fill out your copyright notice in the Description page of Project Settings.


#include "WeatherSystem.h"

// Sets default values
AWeatherSystem::AWeatherSystem()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create root component
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootSceneComponent;

	// Set default values
	GridWidth = 10;
	GridHeight = 10;
	GridSpacing = 500.0f;
	CullingDistance = 8000.0f;
	bEnableFrustumCulling = true;
	ParticleIntensityMultiplier = 1.0f;

}

// Called when the game starts or when spawned
void AWeatherSystem::BeginPlay()
{
	Super::BeginPlay();

	GetPlayerCamera();
	CreateParticleGrid();
	
}

// Called every frame
void AWeatherSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateParticleVisibility();
}

void AWeatherSystem::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Regenerate grid when properties change in editor
	RegenerateGrid();
}

void AWeatherSystem::CreateParticleGrid()
{
    if (!WeatherParticleSystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("WeatherSystem: No particle system assigned!"));
        return;
    }

    // Clear existing particles
    DestroyParticleGrid();

    // Create new particle grid
    for (int32 X = 0; X < GridWidth; X++)
    {
        for (int32 Y = 0; Y < GridHeight; Y++)
        {
            // Create particle system component
            UParticleSystemComponent* ParticleComp = CreateDefaultSubobject<UParticleSystemComponent>(
                *FString::Printf(TEXT("ParticleComponent_%d_%d"), X, Y)
            );

            if (ParticleComp)
            {
                ParticleComp->SetupAttachment(RootComponent);
                ParticleComp->SetTemplate(WeatherParticleSystem);

                // Set position
                FVector GridPos = GetGridPosition(X, Y);
                ParticleComp->SetRelativeLocation(GridPos);

                // Initially set to not auto-activate
                ParticleComp->bAutoActivate = false;

                ParticleComponents.Add(ParticleComp);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("WeatherSystem: Created %d particle components"), ParticleComponents.Num());
}

void AWeatherSystem::DestroyParticleGrid()
{
    for (UParticleSystemComponent* ParticleComp : ParticleComponents)
    {
        if (ParticleComp)
        {
            ParticleComp->DeactivateSystem();
            ParticleComp->DestroyComponent();
        }
    }
    ParticleComponents.Empty();
}

void AWeatherSystem::UpdateParticleVisibility()
{
    if (!PlayerCamera || ParticleComponents.Num() == 0)
        return;

    for (UParticleSystemComponent* ParticleComp : ParticleComponents)
    {
        if (!ParticleComp)
            continue;

        FVector ParticleWorldLocation = ParticleComp->GetComponentLocation();
        bool bShouldBeVisible = false;

        // Check distance culling
        if (IsLocationWithinDistance(ParticleWorldLocation))
        {
            // Check frustum culling if enabled
            if (!bEnableFrustumCulling || IsLocationInCameraFrustum(ParticleWorldLocation))
            {
                bShouldBeVisible = true;
            }
        }

        // Activate/Deactivate based on visibility
        if (bShouldBeVisible && !ParticleComp->IsActive())
        {
            ParticleComp->ActivateSystem();
        }
        else if (!bShouldBeVisible && ParticleComp->IsActive())
        {
            ParticleComp->DeactivateSystem();
        }

        // Update particle intensity if active
        if (ParticleComp->IsActive())
        {
            // You can adjust particle parameters here based on ParticleIntensityMultiplier
            // For example: ParticleComp->SetFloatParameter(TEXT("SpawnRate"), BaseSpawnRate * ParticleIntensityMultiplier);
        }
    }
}

void AWeatherSystem::GetPlayerCamera()
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    APlayerController* PlayerController = World->GetFirstPlayerController();
    if (PlayerController)
    {
        APawn* PlayerPawn = PlayerController->GetPawn();
        if (PlayerPawn)
        {
            // Try to find camera component on the pawn
            UCameraComponent* CameraComp = PlayerPawn->FindComponentByClass<UCameraComponent>();
            if (CameraComp)
            {
                // Store reference to the pawn for camera access
                return;
            }
        }
    }
}

bool AWeatherSystem::IsLocationInCameraFrustum(const FVector& Location)
{
    UWorld* World = GetWorld();
    if (!World)
        return false;

    APlayerController* PlayerController = World->GetFirstPlayerController();
    if (!PlayerController)
        return false;

    // Get camera location and rotation
    FVector CameraLocation;
    FRotator CameraRotation;
    PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

    // Simple frustum check using dot product
    FVector DirectionToLocation = (Location - CameraLocation).GetSafeNormal();
    FVector CameraForward = CameraRotation.Vector();

    float DotProduct = FVector::DotProduct(DirectionToLocation, CameraForward);

    // If the location is roughly in front of the camera (adjust angle as needed)
    // FOV of 90 degrees means dot product should be > 0.0 (180 degrees)
    // For tighter culling, use something like 0.3 (roughly 70 degree cone)
    return DotProduct > -0.2f; // Allow slightly behind camera for better coverage
}

bool AWeatherSystem::IsLocationWithinDistance(const FVector& Location)
{
    UWorld* World = GetWorld();
    if (!World)
        return false;

    APlayerController* PlayerController = World->GetFirstPlayerController();
    if (!PlayerController)
        return false;

    FVector CameraLocation;
    FRotator CameraRotation;
    PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

    float DistanceSquared = FVector::DistSquared(Location, CameraLocation);
    float CullingDistanceSquared = CullingDistance * CullingDistance;

    return DistanceSquared <= CullingDistanceSquared;
}

void AWeatherSystem::RegenerateGrid()
{
    if (HasActorBegunPlay())
    {
        CreateParticleGrid();
    }
}

FVector AWeatherSystem::GetGridPosition(int32 X, int32 Y) const
{
    // Center the grid around the actor's location
    float StartX = -(GridWidth - 1) * GridSpacing * 0.5f;
    float StartY = -(GridHeight - 1) * GridSpacing * 0.5f;

    return FVector(
        StartX + X * GridSpacing,
        StartY + Y * GridSpacing,
        0.0f
    );
}

#if WITH_EDITOR
void AWeatherSystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property)
    {
        FName PropertyName = PropertyChangedEvent.Property->GetFName();

        // Regenerate grid if grid-related properties changed
        if (PropertyName == TEXT("GridWidth") ||
            PropertyName == TEXT("GridHeight") ||
            PropertyName == TEXT("GridSpacing") ||
            PropertyName == TEXT("WeatherParticleSystem"))
        {
            RegenerateGrid();
        }
    }
}
#endif
