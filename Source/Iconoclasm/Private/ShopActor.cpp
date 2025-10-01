// Fill out your copyright notice in the Description page of Project Settings.


#include "ShopActor.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Character.h"

AShopActor::AShopActor()
{
    PrimaryActorTick.bCanEverTick = false;

    InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
    RootComponent = InteractionBox;
    InteractionBox->SetBoxExtent(FVector(100.f));
    InteractionBox->OnComponentBeginOverlap.AddDynamic(this, &AShopActor::OnOverlapBegin);
    InteractionBox->OnComponentEndOverlap.AddDynamic(this, &AShopActor::OnOverlapEnd);

    ShopWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("ShopWidget"));
    ShopWidgetComponent->SetupAttachment(RootComponent);
    ShopWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
    ShopWidgetComponent->SetDrawSize(FVector2D(400, 300));
    ShopWidgetComponent->SetVisibility(false);
}

void AShopActor::BeginPlay()
{
    Super::BeginPlay();
}

void AShopActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (ACharacter* Player = Cast<ACharacter>(OtherActor))
    {
        ShowShopMenu(Player);
    }
}

void AShopActor::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (ACharacter* Player = Cast<ACharacter>(OtherActor))
    {
        HideShopMenu();
    }
}

void AShopActor::ShowShopMenu(AActor* PlayerActor)
{
    ShopWidgetComponent->SetVisibility(true);
}

void AShopActor::HideShopMenu()
{
    ShopWidgetComponent->SetVisibility(false);
}

