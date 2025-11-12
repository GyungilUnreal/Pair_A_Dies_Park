// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "GameControlFunctionLibrary.generated.h"

class UMyGameInstance;
class AMyGameMode;
class URoomManager;

UCLASS()
class UGameControlFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static bool TryGetGameInstance(TObjectPtr<UMyGameInstance>& OutGameInstance);

	static bool TryGetGameMode(TObjectPtr<AMyGameMode>& OutGameMode);

	static bool TryGetRoomManager(TObjectPtr<URoomManager>& OutRoomManager);

private:
	static bool TryGetWorld(TObjectPtr<UWorld>& OutWorld);
};
