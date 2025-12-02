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

void APuzzleTriggerBase::OnRep_IsTriggered()
{
	// 클라이언트에서 실행 - 복제된 상태에 따라 트리거 효과 적용
	OnChangeIsTrigger(_isTriggered);
		
	// 클라이언트에서 직접 효과 적용 (추가 안전장치)
	if (IsNetMode(NM_Client))
	{
		if (_isTriggered)
			TriggerEnterEffect();
		else
			TriggerExitEffect();
	}
}

void APuzzleTriggerBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 복제할 속성 등록 (모든 클라이언트에 복제)
	DOREPLIFETIME(APuzzleTriggerBase, _isTriggered);
}

void APuzzleTriggerBase::OnChangeIsTrigger(bool IsTriggered)
{
	if (IsTriggered)
	{
		if (_isChangeImmediately)
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

void APuzzleTriggerBase::InitializePuzzleTrigger(TObjectPtr<ARoomController> RoomController, int32 PuzzleKey)
{
	_roomController = RoomController;
	_puzzleKey = PuzzleKey;
	ResetTrigger();
}

void APuzzleTriggerBase::Multicast_ResetTrigger_Implementation()
{
	// 모든 클라이언트에서 실행
	bool _wasTriggered = _isTriggered;
	_isTriggered = false;
	ChangeTriggerVisibility(true);
	
	// 서버나 리슨 서버에서는 OnRep_IsTriggered가 호출되지 않으므로 직접 효과 적용
	// 일반 클라이언트에서는 _isTriggered 변경으로 OnRep_IsTriggered가 자동 호출됨
	if (HasAuthority() && _wasTriggered)
	{
		OnChangeIsTrigger(false);
	}
}

void APuzzleTriggerBase::ResetTrigger()
{
	// 서버에서만 실행
	if (HasAuthority())
	{
		// 모든 클라이언트에 멀티캐스트 호출
		Multicast_ResetTrigger();
	}
}

void APuzzleTriggerBase::ChangeTriggerVisibility(bool IsVisible)
{
	if (!IsVisible && !_isDeactiveOnTrigger)
		return;

	SetActorHiddenInGame(!IsVisible);
	SetActorEnableCollision(IsVisible);
	SetActorTickEnabled(IsVisible);
}

void APuzzleTriggerBase::ChangeTriggered(bool IsTriggered)
{
	// 서버에서만 상태 변경 (복제를 통해 클라이언트에 전파)
	if (!HasAuthority())
		return;
	
	// 이미 같은 상태면 중복 호출 방지
	if (_isTriggered == IsTriggered)
		return;
		
	// 상태 변경
	_isTriggered = IsTriggered;
		
	// RoomController에 상태 변경 알림
	if (_roomController == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("ChangeTriggered: _roomController is nullptr"));
		return;
	}

	_roomController->ChangePuzzleTriggerState(_puzzleKey, _isTriggered);
	
	// 서버에서 직접 효과 적용
	OnChangeIsTrigger(_isTriggered);
}