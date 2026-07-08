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
    AttackCooldown = 0.9f; // was 1.5f — much faster follow-up hits

    bCanJump = true;
    JumpCooldown = 1.8f; // was 2.5f

    // Lunge system - near-constant pressure
    bCanLunge = true;
    LungeCooldown = FMath::RandRange(1.5f, 2.5f); // was 2.5–4.5
    LungeChance = FMath::RandRange(0.75f, 1.0f); // was 0.6–0.9
    NextLungeCheckTime = 0.0f;

    bIsCircling = false;
    CircleDirection = FMath::RandBool() ? 1.0f : -1.0f;
    CircleDuration = 0.0f;
    CircleTimer = 0.0f;

    bCanDodge = true;
    DodgeCooldown = 3.5f; // was 5.0f — dodges more, harder to punish

    bCanFeint = true;
    FeintCooldown = FMath::RandRange(6.0f, 10.0f); // was 12–18 — alphas feint way more

    bIsRetreating = false;
    RetreatTimer = 0.0f;

    MyFlankAngle = FMath::RandRange(0.0f, 360.0f);
    FlankDistance = FMath::RandRange(120.0f, 200.0f); // was 150–250 — tighter swarm radius

    Aggression = FMath::RandRange(0.85f, 1.0f); // was 0.7–1.0
    Caution = FMath::RandRange(0.0f, 0.15f); // was 0.1–0.3
    bIsAlpha = FMath::RandRange(0.0f, 1.0f) < 0.25f; // was 0.2 — more alphas

    if (bIsAlpha)
    {
        Aggression = 1.0f;
        LungeChance = 1.0f;
        AttackCooldown = 0.7f;
        Caution = 0.0f;
        FlankDistance *= 0.8f; // alphas crowd in even tighter
    }

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
            RepositionTimer = FMath::RandRange(1.5f, 3.0f); // Less frequent repositioning
        }

        // Update lunge check timer
        NextLungeCheckTime -= DeltaTime;

        // Update circle timer
        if (bIsCircling)
        {
            CircleTimer -= DeltaTime;
            if (CircleTimer <= 0.0f)
            {
                StopCircling();
            }
        }

        // Update retreat timer
        if (bIsRetreating)
        {
            RetreatTimer -= DeltaTime;
            if (RetreatTimer <= 0.0f)
            {
                bIsRetreating = false;
            }
        }

        // Check behaviors - prioritize aggression
        CheckPackBehavior();
        TryLungeAtPlayer(); // Check lunge first - highest priority

        if (bIsAlpha || FMath::FRand() < 0.6f)
        {
            TryCircleStrafe();
            TryDodge();
            if (bIsAlpha) TryFeint();
        }

        CheckRetreat(); // Still check, but very rare

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

    // Randomize initial lunge check time
    NextLungeCheckTime = FMath::RandRange(0.2f, 1.0f); // Shorter initial delay

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

    // Handle retreating - still back away while facing player
    if (bIsRetreating)
    {
        FVector DirectionToPlayer = PlayerLocation - MyLocation;
        DirectionToPlayer.Z = 0.0f;
        FRotator LookAtRotation = DirectionToPlayer.Rotation();
        GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(), LookAtRotation, GetWorld()->GetDeltaSeconds(), 5.0f));

        FVector AwayDirection = (MyLocation - PlayerLocation).GetSafeNormal();
        TargetLocation = MyLocation + (AwayDirection * 300.0f);
        TargetLocation.Z = MyLocation.Z;

        FVector Direction = (TargetLocation - MyLocation).GetSafeNormal();
        MovementComp->AddInputVector(Direction * 0.7f); // Slightly faster retreat
        MoveToLocation(TargetLocation, 50.0f, true, true, false, true, 0, true);
        return;
    }

    // Handle circling - but move forward while circling
    if (bIsCircling)
    {
        FVector ToPlayer = PlayerLocation - MyLocation;
        ToPlayer.Z = 0.0f;
        FVector RightVector = FVector::CrossProduct(ToPlayer, FVector::UpVector).GetSafeNormal();

        // Circle while moving closer
        FVector CircleOffset = RightVector * CircleDirection * 200.0f;
        FVector ForwardBias = ToPlayer.GetSafeNormal() * 100.0f; // Push forward while circling
        TargetLocation = PlayerLocation + CircleOffset + ForwardBias;
        TargetLocation.Z = MyLocation.Z;

        FVector Direction = (TargetLocation - MyLocation).GetSafeNormal();
        MovementComp->AddInputVector(Direction * 1.2f); // Move faster while circling
        MoveToLocation(TargetLocation, 50.0f, true, true, false, true, 0, true);
        return;
    }

    // AGGRESSIVE MOVEMENT - Always push toward player
    if (ShouldHangBack() && !bIsAlpha)
    {
        // Even when hanging back, stay closer
        TargetLocation = TargetFlankPosition;

        // But if too far, push in
        if (DistanceToPlayer > FlankDistance * 1.2f)
        {
            TargetLocation = FMath::Lerp(PlayerLocation, TargetFlankPosition, 0.3f);
        }
    }
    else if (DistanceToPlayer > 300.0f) // Increased threshold
    {
        // Move more directly toward player when far
        float BlendFactor = FMath::Clamp((DistanceToPlayer - 150.0f) / 300.0f, 0.0f, 0.5f);
        TargetLocation = FMath::Lerp(PlayerLocation, TargetFlankPosition, BlendFactor);
    }
    else
    {
        // Close range - go straight for the player
        TargetLocation = PlayerLocation;
    }

    TargetLocation.Z = MyLocation.Z;

    // Aggressive movement input - move faster
    FVector Direction = (TargetLocation - MyLocation).GetSafeNormal();
    MovementComp->AddInputVector(Direction * 1.3f); // Boost movement speed

    MoveToLocation(TargetLocation, 5.0f, true, true, false, true, 0, true);

    // Attack check
    float AttackRange = 150.0f;
    float MaxAttackHeight = 200.0f;
    float FullDistance = FVector::Dist(MyLocation, PlayerLocation);
    float HeightDifference = FMath::Abs(PlayerLocation.Z - MyLocation.Z);

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

    UCharacterMovementComponent* PlayerMovement = PlayerCharacter->GetCharacterMovement();
    if (!PlayerMovement || !PlayerMovement->IsFalling()) return;

    UCharacterMovementComponent* GruntMovement = GruntCharacter->GetCharacterMovement();
    if (!GruntMovement || GruntMovement->IsFalling()) return;

    if (!GruntCharacter->CanAIJump()) return;

    FVector GruntLocation = GruntCharacter->GetActorLocation();
    FVector PlayerLocation = PlayerCharacter->GetActorLocation();

    FVector HorizontalDiff = PlayerLocation - GruntLocation;
    HorizontalDiff.Z = 0.0f;
    float HorizontalDistance = HorizontalDiff.Size();

    float VerticalDistance = PlayerLocation.Z - GruntLocation.Z;

    float MaxJumpRange = 700.0f; // Increased range
    float MinVerticalDiff = 80.0f; // Lower threshold

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

void AGruntAIController::TryLungeAtPlayer()
{
    if (!bCanLunge || !PlayerPawn || NextLungeCheckTime > 0.0f) return;

    AGruntEnemyCharacter* GruntCharacter = Cast<AGruntEnemyCharacter>(GetPawn());
    ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerPawn);

    if (!GruntCharacter || !PlayerCharacter) return;

    UCharacterMovementComponent* PlayerMovement = PlayerCharacter->GetCharacterMovement();
    if (!PlayerMovement || PlayerMovement->IsFalling()) return;

    UCharacterMovementComponent* GruntMovement = GruntCharacter->GetCharacterMovement();
    if (!GruntMovement || GruntMovement->IsFalling()) return;

    if (!GruntCharacter->CanAIJump()) return;

    FVector GruntLocation = GruntCharacter->GetActorLocation();
    FVector PlayerLocation = PlayerCharacter->GetActorLocation();

    FVector HorizontalDiff = PlayerLocation - GruntLocation;
    HorizontalDiff.Z = 0.0f;
    float HorizontalDistance = HorizontalDiff.Size();

    // More aggressive lunge range
    float MinLungeRange = 200.0f; // Can lunge from closer
    float MaxLungeRange = 900.0f; // Can lunge from farther

    if (HorizontalDistance >= MinLungeRange && HorizontalDistance <= MaxLungeRange)
    {
        // Much higher chance to lunge
        if (FMath::FRand() <= LungeChance)
        {
            FVector Direction = HorizontalDiff.GetSafeNormal();

            // Faster, more aggressive lunge
            float LungeSpeed = FMath::Clamp(HorizontalDistance * 2.0f, 700.0f, 1400.0f);
            float LungeAngle = 22.0f; // Even lower, more aggressive angle
            float AngleRad = FMath::DegreesToRadians(LungeAngle);

            FVector LungeVelocity;
            LungeVelocity.X = Direction.X * LungeSpeed * FMath::Cos(AngleRad);
            LungeVelocity.Y = Direction.Y * LungeSpeed * FMath::Cos(AngleRad);
            LungeVelocity.Z = LungeSpeed * FMath::Sin(AngleRad);

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

            bCanLunge = false;
            float RandomizedCooldown = LungeCooldown + FMath::RandRange(-0.5f, 0.5f);
            GetWorld()->GetTimerManager().SetTimer(LungeCooldownTimerHandle,
                this, &AGruntAIController::ResetLunge, RandomizedCooldown);
        }

        // Check more frequently
        NextLungeCheckTime = FMath::RandRange(0.2f, 0.5f);
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
            if (bIsAlpha) DamageAmount *= 1.5f;
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

void AGruntAIController::TryCircleStrafe()
{
    if (bIsCircling || !PlayerPawn || !GetPawn()) return;

    FVector MyLocation = GetPawn()->GetActorLocation();
    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    float Distance = FVector::Dist2D(MyLocation, PlayerLocation);

    // Only circle if already close and occasionally
    if (Distance > 200.0f && Distance < 350.0f && FMath::FRand() < (0.02f * Aggression))
    {
        bIsCircling = true;
        CircleTimer = FMath::RandRange(1.0f, 2.0f); // Shorter circle time
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

void AGruntAIController::TryDodge()
{
    if (!bCanDodge || !PlayerPawn || bIsCircling) return;

    ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerPawn);
    if (!PlayerCharacter) return;

    FVector PlayerForward = PlayerCharacter->GetActorForwardVector();
    FVector ToGrunt = (GetPawn()->GetActorLocation() - PlayerCharacter->GetActorLocation()).GetSafeNormal();

    float DotProduct = FVector::DotProduct(PlayerForward, ToGrunt);

    // Only dodge if very cautious and player is aiming directly
    if (DotProduct > 0.95f && FMath::FRand() < (Caution * 0.15f)) // Much lower chance
    {
        AGruntEnemyCharacter* GruntCharacter = Cast<AGruntEnemyCharacter>(GetPawn());
        if (GruntCharacter)
        {
            UCharacterMovementComponent* MovementComp = GruntCharacter->GetCharacterMovement();
            if (MovementComp && !MovementComp->IsFalling())
            {
                FVector RightVector = PlayerCharacter->GetActorRightVector();
                FVector DodgeDirection = RightVector * (FMath::RandBool() ? 1.0f : -1.0f);

                // Quick sidestep
                FVector DodgeVelocity = DodgeDirection * 700.0f;
                DodgeVelocity.Z = 0.0f;

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

void AGruntAIController::TryFeint()
{
    if (!bCanFeint || !PlayerPawn || !GetPawn() || !bIsAlpha) return; // Only alphas feint

    FVector MyLocation = GetPawn()->GetActorLocation();
    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    float Distance = FVector::Dist2D(MyLocation, PlayerLocation);

    // Rare feint at medium range
    if (Distance > 350.0f && Distance < 600.0f && FMath::FRand() < 0.01f)
    {
        AGruntEnemyCharacter* GruntCharacter = Cast<AGruntEnemyCharacter>(GetPawn());
        if (GruntCharacter && GruntCharacter->CanAIJump())
        {
            UCharacterMovementComponent* MovementComp = GruntCharacter->GetCharacterMovement();
            if (MovementComp && !MovementComp->IsFalling())
            {
                FVector Direction = (PlayerLocation - MyLocation).GetSafeNormal();
                FVector FeintVelocity = Direction * 500.0f;
                FeintVelocity.Z = 200.0f;

                GruntCharacter->Jump();
                MovementComp->AddImpulse(FeintVelocity, true);

                GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Magenta,
                    TEXT("Alpha grunt feinting!"));

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

void AGruntAIController::CheckPackBehavior()
{
    if (!PlayerPawn || !GetPawn()) return;

    // Find nearby allies and occasionally sync a lunge burst
    TArray<AActor*> AllEnemies;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Enemy"), AllEnemies);

    int32 NearbyCount = GetNearbyAlliesCount(400.0f);

    // If a pack has formed, increase this grunt's lunge urgency temporarily
    if (NearbyCount >= 2 && bCanLunge && NextLungeCheckTime > 0.3f)
    {
        // Shrink the wait so pack members pile pressure on faster
        NextLungeCheckTime = FMath::Min(NextLungeCheckTime, 0.3f);
    }

    // Alphas rally nearby grunts by briefly boosting their aggression window
    if (bIsAlpha && NearbyCount >= 1)
    {
        for (AActor* Enemy : AllEnemies)
        {
            AGruntEnemyCharacter* Ally = Cast<AGruntEnemyCharacter>(Enemy);
            if (!Ally || Enemy == GetPawn()) continue;

            AGruntAIController* AllyController = Cast<AGruntAIController>(Ally->GetController());
            if (AllyController && !AllyController->bIsAlpha)
            {
                AllyController->NextLungeCheckTime = FMath::Min(AllyController->NextLungeCheckTime, 0.4f);
            }
        }
    }
}

bool AGruntAIController::ShouldHangBack()
{
    if (!PlayerPawn || bIsAlpha) return false; // Alphas never hang back

    // Count allies within attack range of player
    int32 NearbyAllies = GetNearbyAlliesCount(250.0f);

    // Only hang back if 4+ allies are already swarming
    if (NearbyAllies >= 4)
    {
        return FMath::FRand() < 0.4f; // 40% chance to hang back briefly
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

void AGruntAIController::CheckRetreat()
{
    if (!PlayerPawn || !GetPawn() || bIsAlpha) return; // Alphas NEVER retreat

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

    bool bShouldRetreat = false;

    // Only retreat if last one standing AND very cautious
    if (AllyCount == 0 && Caution > 0.5f)
    {
        bShouldRetreat = FMath::FRand() < 0.08f; // 8% chance - rare
    }

    if (bShouldRetreat && !bIsRetreating)
    {
        bIsRetreating = true;
        RetreatTimer = FMath::RandRange(2.0f, 4.0f); // Shorter retreat duration

        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Purple,
            TEXT("Last grunt standing - brief retreat!"));
    }
}