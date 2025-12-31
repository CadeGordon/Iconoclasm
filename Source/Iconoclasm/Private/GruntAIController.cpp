// Fill out your copyright notice in the Description page of Project Settings.


#include "GruntAIController.h"
#include "Kismet/GameplayStatics.h"
#include "GruntEnemyCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "EngineUtils.h"
#include "Perception/AIPerceptionComponent.h"

AGruntAIController::AGruntAIController()
{
    PrimaryActorTick.bCanEverTick = true;

    bCanAttack = true;
    AttackCooldown = 2.0f;

    bCanJump = true;
    JumpCooldown = 3.0f;

    // Lunge system initialization
    bCanLunge = true;
    LungeCooldown = FMath::RandRange(4.0f, 7.0f);
    LungeChance = FMath::RandRange(0.3f, 0.7f);
    NextLungeCheckTime = 0.0f;

    // NEW: Circle strafe initialization
    bIsCircling = false;
    CircleDirection = FMath::RandBool() ? 1.0f : -1.0f;
    CircleDuration = 0.0f;
    CircleTimer = 0.0f;

    // NEW: Dodge initialization
    bCanDodge = true;
    DodgeCooldown = 2.5f;
    LastPlayerForward = FVector::ZeroVector;

    // NEW: Feint initialization
    bCanFeint = true;
    FeintCooldown = FMath::RandRange(6.0f, 10.0f);

    // NEW: Retreat initialization
    bIsRetreating = false;
    RetreatTimer = 0.0f;

    // Flanking initialization
    MyFlankAngle = FMath::RandRange(0.0f, 360.0f);
    FlankDistance = FMath::RandRange(200.0f, 400.0f);
    RepositionTimer = 0.0f;

    // NEW: Personality traits - gives each grunt unique behavior
    Aggression = FMath::RandRange(0.3f, 1.0f);
    Caution = FMath::RandRange(0.2f, 0.8f);
    bIsAlpha = FMath::RandRange(0.0f, 1.0f) < 0.15f; // 15% chance to be alpha

    // Alpha grunts are more aggressive
    if (bIsAlpha)
    {
        Aggression = FMath::RandRange(0.8f, 1.0f);
        LungeChance = FMath::RandRange(0.6f, 0.9f);
        AttackCooldown = 1.5f; // Faster attacks
    }

    // Make AI more responsive and aggressive
    SetPerceptionComponent(*CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp")));
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
            RepositionTimer = FMath::RandRange(1.0f, 2.0f);
        }

        // Update lunge check timer
        NextLungeCheckTime -= DeltaTime;

        // NEW: Update circle timer
        if (bIsCircling)
        {
            CircleTimer -= DeltaTime;
            if (CircleTimer <= 0.0f)
            {
                StopCircling();
            }
        }

        // NEW: Update retreat timer
        if (bIsRetreating)
        {
            RetreatTimer -= DeltaTime;
            if (RetreatTimer <= 0.0f)
            {
                bIsRetreating = false;
            }
        }

        // NEW: Check for pack behavior
        CheckPackBehavior();

        // NEW: Try various behaviors
        TryCircleStrafe();
        TryDodge();
        TryFeint();
        CheckRetreat();

        MoveToPlayer();
        TryJumpToPlayer();
        TryLungeAtPlayer();
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

    // Randomize initial lunge check time to prevent synchronized lunges
    NextLungeCheckTime = FMath::RandRange(0.5f, 2.0f);

    // Enable continuous movement updates - prevents stopping
    SetFocus(PlayerPawn);
}

float AGruntAIController::FindBestFlankAngle()
{
    if (!PlayerPawn) return MyFlankAngle;

    TArray<AActor*> AllEnemies;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Enemy"), AllEnemies);

    float BestAngle = MyFlankAngle;
    float BestScore = -1.0f;

    int32 NumAngles = 8;
    for (int32 i = 0; i < NumAngles; i++)
    {
        float TestAngle = (360.0f / NumAngles) * i;
        float Score = 1000.0f;

        for (AActor* Enemy : AllEnemies)
        {
            if (!Enemy || Enemy == GetPawn()) continue;

            FVector ToEnemy = Enemy->GetActorLocation() - PlayerPawn->GetActorLocation();
            ToEnemy.Z = 0.0f;
            float EnemyAngle = FMath::Atan2(ToEnemy.Y, ToEnemy.X) * (180.0f / PI);
            if (EnemyAngle < 0.0f) EnemyAngle += 360.0f;

            float AngleDiff = FMath::Abs(TestAngle - EnemyAngle);
            if (AngleDiff > 180.0f) AngleDiff = 360.0f - AngleDiff;

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

    MyFlankAngle = FindBestFlankAngle();

    float AngleRad = FMath::DegreesToRadians(MyFlankAngle);

    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    FVector Offset;
    Offset.X = FMath::Cos(AngleRad) * FlankDistance;
    Offset.Y = FMath::Sin(AngleRad) * FlankDistance;
    Offset.Z = 0.0f;

    TargetFlankPosition = PlayerLocation + Offset;

    if (IsPositionOccupied(TargetFlankPosition, 100.0f))
    {
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

    AGruntEnemyCharacter* GruntCharacter = Cast<AGruntEnemyCharacter>(GetPawn());
    if (!GruntCharacter) return;

    UCharacterMovementComponent* MovementComp = GruntCharacter->GetCharacterMovement();
    if (!MovementComp) return;

    FVector MyLocation = GetPawn()->GetActorLocation();
    FVector PlayerLocation = PlayerPawn->GetActorLocation();

    float DistanceToPlayer = FVector::Dist2D(MyLocation, PlayerLocation);

    FVector TargetLocation;

    // NEW: Handle retreating - back away while facing player
    if (bIsRetreating)
    {
        // Always face the player while retreating
        FVector DirectionToPlayer = PlayerLocation - MyLocation;
        DirectionToPlayer.Z = 0.0f;
        FRotator LookAtRotation = DirectionToPlayer.Rotation();
        GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), LookAtRotation, GetWorld()->GetDeltaSeconds(), 5.0f));

        // Move away from player (backing up)
        FVector AwayDirection = (MyLocation - PlayerLocation).GetSafeNormal();
        TargetLocation = MyLocation + (AwayDirection * 300.0f); // Retreat distance
        TargetLocation.Z = MyLocation.Z;

        // Use slower movement for backing away
        FVector Direction = (TargetLocation - MyLocation).GetSafeNormal();
        MovementComp->AddInputVector(Direction * 0.6f); // 60% speed for retreating
        MoveToLocation(TargetLocation, 50.0f, true, true, false, true, 0, true);

        // Stop attacking while retreating
        return;
    }

    // NEW: Handle circling
    if (bIsCircling)
    {
        FVector ToPlayer = PlayerLocation - MyLocation;
        ToPlayer.Z = 0.0f;
        FVector RightVector = FVector::CrossProduct(ToPlayer, FVector::UpVector).GetSafeNormal();

        // Circle around player
        FVector CircleOffset = RightVector * CircleDirection * 300.0f;
        TargetLocation = PlayerLocation + CircleOffset;
        TargetLocation.Z = MyLocation.Z;

        FVector Direction = (TargetLocation - MyLocation).GetSafeNormal();
        MovementComp->AddInputVector(Direction);
        MoveToLocation(TargetLocation, 50.0f, true, true, false, true, 0, true);
        return;
    }

    // NEW: Check if should hang back
    if (ShouldHangBack())
    {
        // Stay at flank distance
        TargetLocation = TargetFlankPosition;
    }
    else if (DistanceToPlayer > FlankDistance * 1.5f)
    {
        TargetLocation = TargetFlankPosition;
    }
    else if (DistanceToPlayer > 200.0f)
    {
        // Alpha grunts push in more aggressively
        float BlendFactor = (DistanceToPlayer - 200.0f) / (FlankDistance * 1.5f - 200.0f);
        if (bIsAlpha)
        {
            BlendFactor *= 0.5f; // Alphas stay closer
        }
        TargetLocation = FMath::Lerp(PlayerLocation, TargetFlankPosition, BlendFactor);
    }
    else
    {
        TargetLocation = PlayerLocation;
    }

    TargetLocation.Z = MyLocation.Z;

    // Use direct movement input for more responsive behavior
    FVector Direction = (TargetLocation - MyLocation).GetSafeNormal();
    MovementComp->AddInputVector(Direction);

    // Still use MoveToLocation for pathfinding, but with tighter acceptance radius
    MoveToLocation(TargetLocation, 5.0f, true, true, false, true, 0, true);

    // Check attack range with FULL 3D distance (not just horizontal)
    float AttackRange = 150.0f;
    float MaxAttackHeight = 200.0f;
    float FullDistance = FVector::Dist(MyLocation, PlayerLocation);
    float HeightDifference = FMath::Abs(PlayerLocation.Z - MyLocation.Z);

    // Only attack if within range AND within reasonable height
    if (FullDistance <= AttackRange && HeightDifference <= MaxAttackHeight)
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

    if (!GruntCharacter->CanAIJump()) return;

    FVector GruntLocation = GruntCharacter->GetActorLocation();
    FVector PlayerLocation = PlayerCharacter->GetActorLocation();

    FVector HorizontalDiff = PlayerLocation - GruntLocation;
    HorizontalDiff.Z = 0.0f;
    float HorizontalDistance = HorizontalDiff.Size();

    float VerticalDistance = PlayerLocation.Z - GruntLocation.Z;

    float MaxJumpRange = 600.0f;
    float MinVerticalDiff = 100.0f;

    if (HorizontalDistance <= MaxJumpRange && VerticalDistance >= MinVerticalDiff)
    {
        float LaunchSpeed;
        FVector LaunchVelocity;

        if (CalculateJumpVelocity(GruntLocation, PlayerLocation, LaunchSpeed, LaunchVelocity))
        {
            GruntCharacter->Jump();

            FTimerHandle LaunchTimerHandle;
            GetWorld()->GetTimerManager().SetTimer(LaunchTimerHandle, [GruntMovement, LaunchVelocity]()
                {
                    if (GruntMovement)
                    {
                        GruntMovement->Velocity = LaunchVelocity;
                    }
                }, 0.05f, false);

            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green,
                FString::Printf(TEXT("Grunt jumping at airborne player! Speed: %f"), LaunchSpeed));

            bCanJump = false;
            GetWorld()->GetTimerManager().SetTimer(JumpCooldownTimerHandle,
                this, &AGruntAIController::ResetJump, JumpCooldown);
        }
    }
}

// NEW: Lunge attack function
void AGruntAIController::TryLungeAtPlayer()
{
    if (!bCanLunge || !PlayerPawn || NextLungeCheckTime > 0.0f) return;

    AGruntEnemyCharacter* GruntCharacter = Cast<AGruntEnemyCharacter>(GetPawn());
    ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerPawn);

    if (!GruntCharacter || !PlayerCharacter) return;

    // Check if player is on the ground (not airborne)
    UCharacterMovementComponent* PlayerMovement = PlayerCharacter->GetCharacterMovement();
    if (!PlayerMovement || PlayerMovement->IsFalling()) return;

    // Check if grunt is on the ground
    UCharacterMovementComponent* GruntMovement = GruntCharacter->GetCharacterMovement();
    if (!GruntMovement || GruntMovement->IsFalling()) return;

    if (!GruntCharacter->CanAIJump()) return;

    FVector GruntLocation = GruntCharacter->GetActorLocation();
    FVector PlayerLocation = PlayerCharacter->GetActorLocation();

    // Calculate horizontal distance
    FVector HorizontalDiff = PlayerLocation - GruntLocation;
    HorizontalDiff.Z = 0.0f;
    float HorizontalDistance = HorizontalDiff.Size();

    // Lunge range: not too close, not too far
    float MinLungeRange = 300.0f;
    float MaxLungeRange = 800.0f;

    if (HorizontalDistance >= MinLungeRange && HorizontalDistance <= MaxLungeRange)
    {
        // Random chance to lunge - creates unpredictability
        if (FMath::FRand() <= LungeChance)
        {
            // Calculate lunge velocity (lower arc than jump, more horizontal)
            FVector Direction = HorizontalDiff.GetSafeNormal();

            // Lunge parameters - fast and low
            float LungeSpeed = FMath::Clamp(HorizontalDistance * 1.8f, 600.0f, 1200.0f);
            float LungeAngle = 25.0f; // Lower angle = more aggressive, ground-hugging lunge
            float AngleRad = FMath::DegreesToRadians(LungeAngle);

            FVector LungeVelocity;
            LungeVelocity.X = Direction.X * LungeSpeed * FMath::Cos(AngleRad);
            LungeVelocity.Y = Direction.Y * LungeSpeed * FMath::Cos(AngleRad);
            LungeVelocity.Z = LungeSpeed * FMath::Sin(AngleRad);

            // Execute lunge
            GruntCharacter->Jump();

            FTimerHandle LungeTimerHandle;
            GetWorld()->GetTimerManager().SetTimer(LungeTimerHandle, [GruntMovement, LungeVelocity]()
                {
                    if (GruntMovement)
                    {
                        GruntMovement->Velocity = LungeVelocity;
                    }
                }, 0.05f, false);

            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow,
                FString::Printf(TEXT("Grunt lunging! Distance: %.0f"), HorizontalDistance));

            // Start cooldown with randomization
            bCanLunge = false;
            float RandomizedCooldown = LungeCooldown + FMath::RandRange(-1.0f, 1.0f);
            GetWorld()->GetTimerManager().SetTimer(LungeCooldownTimerHandle,
                this, &AGruntAIController::ResetLunge, RandomizedCooldown);
        }

        // Reset check timer with randomization to prevent synchronized attempts
        NextLungeCheckTime = FMath::RandRange(0.3f, 0.8f);
    }
}

void AGruntAIController::ResetLunge()
{
    bCanLunge = true;
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

// NEW: Circle strafe behavior - makes grunts circle around player
void AGruntAIController::TryCircleStrafe()
{
    if (bIsCircling || !PlayerPawn || !GetPawn()) return;

    FVector MyLocation = GetPawn()->GetActorLocation();
    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    float Distance = FVector::Dist2D(MyLocation, PlayerLocation);

    // Circle if at medium range and based on aggression
    if (Distance > 250.0f && Distance < 500.0f && FMath::FRand() < (0.05f * Aggression))
    {
        bIsCircling = true;
        CircleTimer = FMath::RandRange(1.5f, 3.0f);
        CircleDirection = FMath::RandBool() ? 1.0f : -1.0f;

        GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan,
            TEXT("Grunt circling!"));
    }
}

void AGruntAIController::StopCircling()
{
    bIsCircling = false;
    CircleTimer = 0.0f;
}

// NEW: Dodge/sidestep when player is looking at them
void AGruntAIController::TryDodge()
{
    if (!bCanDodge || !PlayerPawn || bIsCircling) return;

    ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerPawn);
    if (!PlayerCharacter) return;

    FVector PlayerForward = PlayerCharacter->GetActorForwardVector();
    FVector ToGrunt = (GetPawn()->GetActorLocation() - PlayerCharacter->GetActorLocation()).GetSafeNormal();

    // Check if player is facing the grunt
    float DotProduct = FVector::DotProduct(PlayerForward, ToGrunt);

    // If player suddenly aims at grunt and grunt is cautious
    if (DotProduct > 0.8f && FMath::FRand() < (Caution * 0.3f))
    {
        AGruntEnemyCharacter* GruntCharacter = Cast<AGruntEnemyCharacter>(GetPawn());
        if (GruntCharacter)
        {
            UCharacterMovementComponent* MovementComp = GruntCharacter->GetCharacterMovement();
            if (MovementComp && !MovementComp->IsFalling())
            {
                // Quick ground-based sidestep dodge (no jumping)
                FVector RightVector = PlayerCharacter->GetActorRightVector();
                FVector DodgeDirection = RightVector * (FMath::RandBool() ? 1.0f : -1.0f);

                // Ground dash - just horizontal velocity, no vertical component
                FVector DodgeVelocity = DodgeDirection * 800.0f;
                DodgeVelocity.Z = 0.0f; // Keep on ground

                // Apply velocity directly without jumping
                MovementComp->AddImpulse(DodgeVelocity, true);

                GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange,
                    TEXT("Grunt dodging!"));

                bCanDodge = false;
                GetWorld()->GetTimerManager().SetTimer(DodgeCooldownTimerHandle,
                    this, &AGruntAIController::ResetDodge, DodgeCooldown);
            }
        }
    }

    LastPlayerForward = PlayerForward;
}

void AGruntAIController::ResetDodge()
{
    bCanDodge = true;
}

// NEW: Feint - fake lunge to bait player reactions
void AGruntAIController::TryFeint()
{
    if (!bCanFeint || !PlayerPawn || !GetPawn()) return;

    FVector MyLocation = GetPawn()->GetActorLocation();
    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    float Distance = FVector::Dist2D(MyLocation, PlayerLocation);

    // Feint at medium-close range
    if (Distance > 300.0f && Distance < 600.0f && FMath::FRand() < (0.02f * Aggression))
    {
        AGruntEnemyCharacter* GruntCharacter = Cast<AGruntEnemyCharacter>(GetPawn());
        if (GruntCharacter && GruntCharacter->CanAIJump())
        {
            UCharacterMovementComponent* MovementComp = GruntCharacter->GetCharacterMovement();
            if (MovementComp && !MovementComp->IsFalling())
            {
                // Small forward hop (looks like starting a lunge)
                FVector Direction = (PlayerLocation - MyLocation).GetSafeNormal();
                FVector FeintVelocity = Direction * 400.0f;
                FeintVelocity.Z = 150.0f;

                GruntCharacter->Jump();
                MovementComp->AddImpulse(FeintVelocity, true);

                GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Magenta,
                    TEXT("Grunt feinting!"));

                bCanFeint = false;
                GetWorld()->GetTimerManager().SetTimer(FeintCooldownTimerHandle,
                    this, &AGruntAIController::ResetFeint, FeintCooldown);
            }
        }
    }
}

void AGruntAIController::ResetFeint()
{
    bCanFeint = true;
}

// NEW: Pack behavior - coordinate with other grunts
void AGruntAIController::CheckPackBehavior()
{
    // Implementation for pack tactics
}

bool AGruntAIController::ShouldHangBack()
{
    if (!PlayerPawn) return false;

    // Count allies within attack range of player
    int32 NearbyAllies = GetNearbyAlliesCount(300.0f);

    // If 3+ allies are close, hang back unless alpha
    if (NearbyAllies >= 3 && !bIsAlpha)
    {
        return FMath::FRand() < 0.6f;
    }

    return false;
}

int32 AGruntAIController::GetNearbyAlliesCount(float Radius)
{
    if (!PlayerPawn) return 0;

    TArray<AActor*> AllEnemies;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Enemy"), AllEnemies);

    int32 Count = 0;
    FVector PlayerLocation = PlayerPawn->GetActorLocation();

    for (AActor* Enemy : AllEnemies)
    {
        if (!Enemy || Enemy == GetPawn()) continue;

        float Distance = FVector::Dist(Enemy->GetActorLocation(), PlayerLocation);
        if (Distance <= Radius)
        {
            Count++;
        }
    }

    return Count;
}

// NEW: Tactical retreat when last enemy standing
void AGruntAIController::CheckRetreat()
{
    if (!PlayerPawn || !GetPawn()) return;

    // Count remaining allies
    int32 AllyCount = 0;
    TArray<AActor*> AllEnemies;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Enemy"), AllEnemies);

    for (AActor* Enemy : AllEnemies)
    {
        if (Enemy && Enemy != GetPawn())
        {
            AllyCount++;
        }
    }

    // If this is the last enemy or only 1-2 left, cautious grunts retreat
    bool bShouldRetreat = false;

    if (AllyCount == 0) // Last one standing
    {
        // 50% chance to retreat if cautious, alphas never retreat
        if (!bIsAlpha && Caution > 0.4f)
        {
            bShouldRetreat = FMath::FRand() < 0.15f;
        }
    }
    else if (AllyCount <= 2 && !bIsAlpha) // 1-2 allies left and not an alpha
    {
        // Only very cautious grunts retreat, and only sometimes
        bShouldRetreat = (Caution > 0.6f && FMath::FRand() < 0.05f);
    }

    // Start retreat if conditions met
    if (bShouldRetreat && !bIsRetreating)
    {
        bIsRetreating = true;
        RetreatTimer = FMath::RandRange(3.0f, 6.0f); // Retreat for a while

        if (AllyCount == 0)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Purple,
                TEXT("Last grunt standing - retreating!"));
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Purple,
                TEXT("Grunt retreating - outnumbered!"));
        }
    }
}