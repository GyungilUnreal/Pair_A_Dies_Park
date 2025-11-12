// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"
#include "JHS/GameControll/GameControlFunctionLibrary.h"
#include "JHS/GameControll/MyGameInstance.h"
#include "JHS/Room/RoomDataTable.h"
#include "Kismet/GameplayStatics.h"
#include "CharacterFunctionLibrary.h"

void AMyPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    TObjectPtr<UMyGameInstance> _gameInstance = nullptr;
    if (!UGameControlFunctionLibrary::TryGetGameInstance(_gameInstance))
        return;

    FRoomData _roomData = _gameInstance->GetCurrentRoomData();
    float _playerScale = _roomData.PlayerScale;
    TObjectPtr<ACharacter> _character = Cast<ACharacter>(InPawn);
    if (!_character)
    {
        UE_LOG(LogTemp, Error, TEXT("Character is nullptr"));
        return;
    }

    UCharacterFunctionLibrary::SetPlayerScale(_character, _playerScale);
    UCharacterFunctionLibrary::SetCameraDistance(_character, _roomData.CameraDistance);
}