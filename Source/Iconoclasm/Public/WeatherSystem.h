// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "WeatherSystem.generated.h"

UCLASS()
class ICONOCLASM_API AWeatherSystem : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeatherSystem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void OnConstruction(const FTransform& Transform) override;

    // Root component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USceneComponent* RootSceneComponent;

    // Particle system template to spawn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Settings")
    class UParticleSystem* WeatherParticleSystem;

    // Grid settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings", meta = (ClampMin = "1", ClampMax = "50"))
    int32 GridWidth = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings", meta = (ClampMin = "1", ClampMax = "50"))
    int32 GridHeight = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Settings", meta = (ClampMin = "100.0", ClampMax = "2000.0"))
    float GridSpacing = 500.0f;

    // Culling settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Culling Settings", meta = (ClampMin = "1000.0", ClampMax = "20000.0"))
    float CullingDistance = 8000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Culling Settings")
    bool bEnableFrustumCulling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Settings")
    float ParticleIntensityMultiplier = 1.0f;

private:
    // Array to store particle system components
    UPROPERTY()
    TArray<class UParticleSystemComponent*> ParticleComponents;

    // Camera reference for culling
    UPROPERTY()
    class ACameraActor* PlayerCamera;

    // Helper functions
    void CreateParticleGrid();
    void DestroyParticleGrid();
    void UpdateParticleVisibility();
    void GetPlayerCamera();
    bool IsLocationInCameraFrustum(const FVector& Location);
    bool IsLocationWithinDistance(const FVector& Location);

    // Grid management
    void RegenerateGrid();
    FVector GetGridPosition(int32 X, int32 Y) const;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif



};
