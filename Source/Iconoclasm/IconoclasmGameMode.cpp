// Copyright Epic Games, Inc. All Rights Reserved.

#include "IconoclasmGameMode.h"
#include "IconoclasmCharacter.h"
#include "UObject/ConstructorHelpers.h"

AIconoclasmGameMode::AIconoclasmGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
