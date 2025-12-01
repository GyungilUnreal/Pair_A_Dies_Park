// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Puzzle/Action/PuzzleActionBase.h"
#include "Net/UnrealNetwork.h"

// Sets default values
APuzzleActionBase::APuzzleActionBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	// 네트워크 복제 활성화
	bReplicates = true;
	SetReplicateMovement(true);
}

// Called when the game starts or when spawned
void APuzzleActionBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APuzzleActionBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APuzzleActionBase::OnRep_IsActivate()
{
	// 클라이언트에서 실행 - 복제된 상태에 따라 효과 적용
	if (_isActivate)
		OnActivatePuzzleAction();
	else
		OnDeactivatePuzzleAction();
}

void APuzzleActionBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// 복제할 속성 등록
	DOREPLIFETIME(APuzzleActionBase, _isActivate);
}

void APuzzleActionBase::InitializePuzzleAction()
{
	_isActivate = false;
	OnDeactivatePuzzleAction();
}

void APuzzleActionBase::ActivatePuzzleAction()
{
	// 서버에서만 상태 변경
	if (!HasAuthority())
		return;
		
	if (_isActivate)
		return;

	_isActivate = true;
	OnActivatePuzzleAction();

	if (_isDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("Activate Puzzle Action %d"), _actionIndex);
	}
}

void APuzzleActionBase::DeactivatePuzzleAction()
{
	// 서버에서만 상태 변경
	if (!HasAuthority())
		return;
		
	if (!_isActivate)
		return;

	_isActivate = false;
	OnDeactivatePuzzleAction();

	if (_isDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("Deactivate Puzzle Action %d"), _actionIndex);
	}
}