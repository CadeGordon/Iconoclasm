// Fill out your copyright notice in the Description page of Project Settings.


#include "GruntAIController.h"
#include "Kismet/GameplayStatics.h"
#include "GruntEnemyCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "EngineUtils.h"

AGruntAIController::AGruntAIController()
{
    bCanAttack = true;
    AttackCooldown = 2.0f;

    bCanJump = true;
    JumpCooldown = 3.0f;

    // Flanking initialization
    MyFlankAngle = FMath::RandRange(0.0f, 360.0f); // Random starting angle
    FlankDistance = FMath::RandRange(200.0f, 400.0f); // Random distance preference
    RepositionTimer = 0.0f;
}

void AGruntAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (GetPawn() && PlayerPawn)
    {
        // Update reposition timer
        RepositionTimer -= DeltaTime;
        if (RepositionTimer <= 0.0f)
        {
            CalculateFlankPosition();
            RepositionTimer = FMath::RandRange(1.0f, 2.0f); // Recalculate every 1-2 seconds
        }

        MoveToPlayer();
        TryJumpToPlayer();
    }
}

void AGruntAIController::BeginPlay()
{
    Super::BeginPlay();

    PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

    if (GetPawn())
    {
        CalculateFlankPosition();
        MoveToPlayer();
    }
}

float AGruntAIController::FindBestFlankAngle()
{
    if (!PlayerPawn) return MyFlankAngle;

    // Get all other grunt enemies in the world
    TArray<AActor*> AllEnemies;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Enemy"), AllEnemies);

    // Try to find an angle that's not too close to other enemies
    float BestAngle = MyFlankAngle;
    float BestScore = -1.0f;

    // Test several angles around the player
    int32 NumAngles = 8; // Test 8 directions (45 degree increments)
    for (int32 i = 0; i < NumAngles; i++)
    {
        float TestAngle = (360.0f / NumAngles) * i;
        float Score = 1000.0f; // Start with high score

        // Check against other enemies
        for (AActor* Enemy : AllEnemies)
        {
            if (!Enemy || Enemy == GetPawn()) continue;

            // Calculate this enemy's angle relative to player
            FVector ToEnemy = Enemy->GetActorLocation() - PlayerPawn->GetActorLocation();
            ToEnemy.Z = 0.0f;
            float EnemyAngle = FMath::Atan2(ToEnemy.Y, ToEnemy.X) * (180.0f / PI);
            if (EnemyAngle < 0.0f) EnemyAngle += 360.0f;

            // Calculate angle difference
            float AngleDiff = FMath::Abs(TestAngle - EnemyAngle);
            if (AngleDiff > 180.0f) AngleDiff = 360.0f - AngleDiff;

            // Penalize angles close to other enemies
            Score -= FMath::Max(0.0f, 90.0f - AngleDiff);
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            BestAngle = TestAngle;
        }
    }

    return BestAngle;
}

bool AGruntAIController::IsPositionOccupied(const FVector& Position, float Radius)
{
    TArray<AActor*> AllEnemies;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Enemy"), AllEnemies);

    for (AActor* Enemy : AllEnemies)
    {
        if (!Enemy || Enemy == GetPawn()) continue;

        float Distance = FVector::Dist(Enemy->GetActorLocation(), Position);
        if (Distance < Radius)
        {
            return true;
        }
    }

    return false;
}

void AGruntAIController::CalculateFlankPosition()
{
    if (!PlayerPawn || !GetPawn()) return;

    // Find the best angle to approach from
    MyFlankAngle = FindBestFlankAngle();

    // Convert angle to radians
    float AngleRad = FMath::DegreesToRadians(MyFlankAngle);

    // Calculate position around the player
    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    FVector Offset;
    Offset.X = FMath::Cos(AngleRad) * FlankDistance;
    Offset.Y = FMath::Sin(AngleRad) * FlankDistance;
    Offset.Z = 0.0f;

    TargetFlankPosition = PlayerLocation + Offset;

    // If this position is occupied, try a slightly different angle
    if (IsPositionOccupied(TargetFlankPosition, 100.0f))
    {
        // Offset by a random amount
        float AngleOffset = FMath::RandRange(-45.0f, 45.0f);
        MyFlankAngle += AngleOffset;
        if (MyFlankAngle >= 360.0f) MyFlankAngle -= 360.0f;
        if (MyFlankAngle < 0.0f) MyFlankAngle += 360.0f;

        AngleRad = FMath::DegreesToRadians(MyFlankAngle);
        Offset.X = FMath::Cos(AngleRad) * FlankDistance;
        Offset.Y = FMath::Sin(AngleRad) * FlankDistance;
        TargetFlankPosition = PlayerLocation + Offset;
    }
}

void AGruntAIController::MoveToPlayer()
{
    if (!PlayerPawn || !GetPawn()) return;

    FVector MyLocation = GetPawn()->GetActorLocation();
    FVector PlayerLocation = PlayerPawn->GetActorLocation();

    // Calculate distance to player
    float DistanceToPlayer = FVector::Dist2D(MyLocation, PlayerLocation);

    // If we're far from player, move to flank position
    // If we're close, move directly to player for attack
    FVector TargetLocation;

    if (DistanceToPlayer > FlankDistance * 1.5f)
    {
        // Far away - use flanking position
        TargetLocation = TargetFlankPosition;
    }
    else if (DistanceToPlayer > 200.0f)
    {
        // Medium range - blend between flank and direct
        float BlendFactor = (DistanceToPlayer - 200.0f) / (FlankDistance * 1.5f - 200.0f);
        TargetLocation = FMath::Lerp(PlayerLocation, TargetFlankPosition, BlendFactor);
    }
    else
    {
        // Close range - move directly to player
        TargetLocation = PlayerLocation;
    }

    // Keep target at ground level
    TargetLocation.Z = MyLocation.Z;

    // Move to the calculated position
    MoveToLocation(TargetLocation, 5.0f);

    // Check if close enough to attack
    float AttackRange = 150.0f;
    if (DistanceToPlayer <= AttackRange)
    {
        AttackPlayer();
    }
}

void AGruntAIController::TryJumpToPlayer()
{
    if (!bCanJump || !PlayerPawn) return;

    AGruntEnemyCharacter* GruntCharacter = Cast<AGruntEnemyCharacter>(GetPawn());
    ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerPawn);

    if (!GruntCharacter || !PlayerCharacter) return;

    // Check if player is in the air
    UCharacterMovementComponent* PlayerMovement = PlayerCharacter->GetCharacterMovement();
    if (!PlayerMovement || !PlayerMovement->IsFalling()) return;

    // Check if grunt is on the ground
    UCharacterMovementComponent* GruntMovement = GruntCharacter->GetCharacterMovement();
    if (!GruntMovement || GruntMovement->IsFalling()) return;

    // Check if the character can jump using our custom function
    if (!GruntCharacter->CanAIJump()) return;

    FVector GruntLocation = GruntCharacter->GetActorLocation();
    FVector PlayerLocation = PlayerCharacter->GetActorLocation();

    // Calculate horizontal distance
    FVector HorizontalDiff = PlayerLocation - GruntLocation;
    HorizontalDiff.Z = 0.0f;
    float HorizontalDistance = HorizontalDiff.Size();

    // Calculate vertical distance
    float VerticalDistance = PlayerLocation.Z - GruntLocation.Z;

    // Only jump if player is within reasonable range and above us
    float MaxJumpRange = 600.0f;
    float MinVerticalDiff = 100.0f;

    if (HorizontalDistance <= MaxJumpRange && VerticalDistance >= MinVerticalDiff)
    {
        float LaunchSpeed;
        FVector LaunchVelocity;

        if (CalculateJumpVelocity(GruntLocation, PlayerLocation, LaunchSpeed, LaunchVelocity))
        {
            // Make the character jump first to leave the ground
            GruntCharacter->Jump();

            // Then apply the calculated velocity on the next frame
            FTimerHandle LaunchTimerHandle;
            GetWorld()->GetTimerManager().SetTimer(LaunchTimerHandle, [GruntMovement, LaunchVelocity]()
                {
                    if (GruntMovement)
                    {
                        GruntMovement->Velocity = LaunchVelocity;
                    }
                }, 0.05f, false);

            // Debug visualization
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green,
                FString::Printf(TEXT("Grunt jumping! Speed: %f"), LaunchSpeed));

            // Start cooldown
            bCanJump = false;
            GetWorld()->GetTimerManager().SetTimer(JumpCooldownTimerHandle,
                this, &AGruntAIController::ResetJump, JumpCooldown);
        }
    }
}

bool AGruntAIController::CalculateJumpVelocity(const FVector& StartLocation, const FVector& TargetLocation,
    float& OutLaunchSpeed, FVector& OutLaunchVelocity)
{
    ACharacter* GruntCharacter = Cast<ACharacter>(GetPawn());
    if (!GruntCharacter) return false;

    UCharacterMovementComponent* Movement = GruntCharacter->GetCharacterMovement();
    if (!Movement) return false;

    float Gravity = FMath::Abs(Movement->GetGravityZ());

    FVector Difference = TargetLocation - StartLocation;
    FVector HorizontalDiff = Difference;
    HorizontalDiff.Z = 0.0f;
    float HorizontalDistance = HorizontalDiff.Size();
    float VerticalDistance = Difference.Z;

    if (HorizontalDistance < 1.0f) return false;

    float OptimalAngle = 45.0f;
    float AngleRad = FMath::DegreesToRadians(OptimalAngle);

    float LaunchSpeed = FMath::Sqrt((HorizontalDistance * Gravity) / FMath::Sin(2.0f * AngleRad));

    if (VerticalDistance > 0.0f)
    {
        float ExtraSpeed = FMath::Sqrt(2.0f * Gravity * VerticalDistance);
        LaunchSpeed += ExtraSpeed * 0.5f;
    }

    LaunchSpeed = FMath::Clamp(LaunchSpeed, 300.0f, 1500.0f);

    FVector HorizontalDirection = HorizontalDiff.GetSafeNormal();

    float VerticalComponent = LaunchSpeed * FMath::Sin(AngleRad);
    float HorizontalComponent = LaunchSpeed * FMath::Cos(AngleRad);

    OutLaunchVelocity = HorizontalDirection * HorizontalComponent;
    OutLaunchVelocity.Z = VerticalComponent;

    OutLaunchSpeed = LaunchSpeed;

    return true;
}

void AGruntAIController::ResetJump()
{
    bCanJump = true;
}

void AGruntAIController::AttackPlayer()
{
    if (bCanAttack && PlayerPawn)
    {
        AGruntEnemyCharacter* GruntPawn = Cast<AGruntEnemyCharacter>(GetPawn());
        float DamageAmount = 20.0f;

        if (GruntPawn)
        {
            DamageAmount = GruntPawn->DamageAmount;
        }

        UGameplayStatics::ApplyDamage(
            PlayerPawn,
            DamageAmount,
            GetPawn()->GetController(),
            GetPawn(),
            UDamageType::StaticClass()
        );

        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red,
            FString::Printf(TEXT("Player hit - %f Damage dealt!"), DamageAmount));

        bCanAttack = false;
        GetWorld()->GetTimerManager().SetTimer(AttackCooldownTimerHandle,
            this, &AGruntAIController::ResetAttack, AttackCooldown);
    }
}

void AGruntAIController::ResetAttack()
{
    bCanAttack = true;
}