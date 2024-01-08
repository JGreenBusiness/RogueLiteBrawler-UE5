// Copyright Epic Games, Inc. All Rights Reserved.

#include "RogueliteBrawlerGameMode.h"
#include "RogueliteBrawlerCharacter.h"
#include "UObject/ConstructorHelpers.h"

ARogueliteBrawlerGameMode::ARogueliteBrawlerGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
