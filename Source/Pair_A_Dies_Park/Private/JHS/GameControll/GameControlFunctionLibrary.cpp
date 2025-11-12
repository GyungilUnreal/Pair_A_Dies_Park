// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/GameControll/GameControlFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "JHS/GameControll/MyGameInstance.h"
#include "JHS/GameControll/MyGameMode.h"
#include "JHS/Room/RoomManager.h"

bool UGameControlFunctionLibrary::TryGetGameInstance(TObjectPtr<UMyGameInstance>& OutGameInstance)
{
    TObjectPtr<UWorld> World = nullptr;
    if (!TryGetWorld(World))
        return false;

    OutGameInstance = Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(World));
    if (!OutGameInstance)   
    {
        UE_LOG(LogTemp, Error, TEXT("GameInstance is nullptr"));
        return false;
    }

    return true;
}

bool UGameControlFunctionLibrary::TryGetGameMode(TObjectPtr<AMyGameMode>& OutGameMode)
{
    TObjectPtr<UWorld> World = nullptr;
    if (!TryGetWorld(World))
        return false;

    OutGameMode = Cast<AMyGameMode>(UGameplayStatics::GetGameMode(World));
    if (!OutGameMode)
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode is nullptr"));
        return false;
    }

    return true;
}

bool UGameControlFunctionLibrary::TryGetRoomManager(TObjectPtr<URoomManager>& OutRoomManager)
{
    TObjectPtr<AMyGameMode> OutGameMode = nullptr;
    if (!TryGetGameMode(OutGameMode))
        return false;

    OutRoomManager = OutGameMode->GetRoomManager();
    if (!OutRoomManager)
    {
        UE_LOG(LogTemp, Error, TEXT("RoomManager is nullptr"));
        return false;
    }

    return true;
}

bool UGameControlFunctionLibrary::TryGetWorld(TObjectPtr<UWorld>& OutWorld)
{
    OutWorld = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr;
    if (!OutWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("World is nullptr"));
        return false;
    }

    return true;
}