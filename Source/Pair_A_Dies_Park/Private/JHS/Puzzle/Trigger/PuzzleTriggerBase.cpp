// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Puzzle/Trigger/PuzzleTriggerBase.h"
#include "JHS/Room/RoomController.h"
#include "Net/UnrealNetwork.h"

// Sets default values
APuzzleTriggerBase::APuzzleTriggerBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	// 네트워크 복제 활성화
	bReplicates = true;
	SetReplicateMovement(true);
}

// Called when the game starts or when spawned
void APuzzleTriggerBase::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void APuzzleTriggerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APuzzleTriggerBase::OnTriggerEnter()
{
	ChangeTriggered(true);
}

void APuzzleTriggerBase::OnTriggerExit()
{
	if (_isLockOnTrigger && _isTriggered)
		return;

	ChangeTriggered(false);
}

void APuzzleTriggerBase::InitializePuzzleTrigger(TObjectPtr<ARoomController> RoomController, int32 PuzzleKey)
{
	_roomController = RoomController;
	_puzzleKey = PuzzleKey;
	_isTriggered = false;
	ChangeTriggered(false);
}

void APuzzleTriggerBase::ChangeTriggered(bool IsTriggered)
{
	// 클라이언트에서 호출된 경우 서버에 요청
	if (!HasAuthority())
	{
		Server_ChangeTriggered(IsTriggered);
		return;
	}

	// 서버에서 상태를 먼저 업데이트 (RoomController에서 IsTriggered() 호출 시 올바른 값 반환)
	_isTriggered = IsTriggered;

	// 서버에서만 RoomController에 상태 변경 알림
	if (_roomController == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("ChangeTriggered: _roomController is nullptr"));
		return;
	}
	_roomController->ChangePuzzleTriggerState(_puzzleKey, IsTriggered);

	// 멀티캐스트로 모든 클라이언트에 상태 변경 전파
	Multicast_ChangeTriggered(IsTriggered);
}

void APuzzleTriggerBase::Server_ChangeTriggered_Implementation(bool IsTriggered)
{
	ChangeTriggered(IsTriggered);
}

void APuzzleTriggerBase::Multicast_ChangeTriggered_Implementation(bool IsTriggered)
{
	_isTriggered = IsTriggered;

	if (_isTriggered)
	{
		if (_isHideImmediately && _isDeactiveOnTrigger)
		{
			ChangeTriggerVisibility(false);
		}

		TriggerEnterEffect();
	}
	else
	{
		TriggerExitEffect();
	}
}

void APuzzleTriggerBase::OnChangeAction(bool IsActionActivate)
{
	if (IsActionActivate)
	{
		if (_isDeactiveOnTrigger)
		{
			ChangeTriggerVisibility(false);
		}
	}
	else
	{

	}
}

void APuzzleTriggerBase::ResetTrigger()
{
	// 클라이언트에서 호출된 경우 서버에 요청
	if (!HasAuthority())
	{
		Server_ResetTrigger();
		return;
	}

	Multicast_ResetTrigger();
}

void APuzzleTriggerBase::Server_ResetTrigger_Implementation()
{
	ResetTrigger();
}

void APuzzleTriggerBase::Multicast_ResetTrigger_Implementation()
{
	ChangeTriggered(false);
	ResetTriggerOverride();
	ChangeTriggerVisibility(true);
}

void APuzzleTriggerBase::ChangeTriggerVisibility(bool IsVisible)
{
	if (!IsVisible && !_isDeactiveOnTrigger)
		return;

	SetActorHiddenInGame(!IsVisible);
	SetActorEnableCollision(IsVisible);
	SetActorTickEnabled(IsVisible);
}