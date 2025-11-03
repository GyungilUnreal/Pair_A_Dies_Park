// Copyright Epic Games, Inc. All Rights Reserved.

#include "Pair_A_Dies_ParkGameMode.h"
#include "Pair_A_Dies_ParkCharacter.h"
#include "UObject/ConstructorHelpers.h"

APair_A_Dies_ParkGameMode::APair_A_Dies_ParkGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
