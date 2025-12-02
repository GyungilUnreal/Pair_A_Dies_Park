// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Puzzle/Action/MovementAction.h"
#include "Components/StaticMeshComponent.h"

AMovementAction::AMovementAction()
{
    _rootComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RootComponent"));
    _rootComponent->SetupAttachment(RootComponent);

    _deactivatePosition = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DeactivatePosition"));
    _deactivatePosition->SetupAttachment(_rootComponent);

    _activatePosition = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ActivatePosition"));
    _activatePosition->SetupAttachment(_rootComponent);

	_movementMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MovementMesh"));
	_movementMesh->SetupAttachment(_rootComponent);
}

void AMovementAction::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// 이동할 액터가 설정되어 있는 경우
	if (_moveWithActor != nullptr)
	{
		// 이동할 액터의 초기 상대 위치 계산 (_deactivatePosition 기준)
		_moveWithActorOffset = _moveWithActor->GetActorLocation() - _deactivatePosition->GetComponentLocation();

		// 이동 메시와 액터 간의 회전 차이 계산
		_moveWithActorRotationDiff = _moveWithActor->GetActorRotation() - _movementMesh->GetComponentRotation();
	}
}

void AMovementAction::BeginPlay()
{
	Super::BeginPlay();
}

void AMovementAction::OnActivatePuzzleAction()
{
    Super::OnActivatePuzzleAction();

    _isActivate = true;
	StopMovement();
	StartMovement();
}

void AMovementAction::OnDeactivatePuzzleAction()
{
    Super::OnDeactivatePuzzleAction();

    _isActivate = false;
	StopMovement();
	StartMovement();
}

void AMovementAction::StartMovement()
{
	_currentMovementTime = 0.0f;

	// 시작 위치와 회전값 설정
	_startMovementLocation = _movementMesh->GetComponentLocation();
	_startMovementRotator = _movementMesh->GetComponentRotation();
	
	// 활성화 상태에 따라 목표 위치와 회전값 설정
	if (_isActivate)
	{
		// 활성화 위치로 이동
		_endMovementLocation = _activatePosition->GetComponentLocation();
		_endMovementRotator = _activatePosition->GetComponentRotation();
	}
	else
	{
		// 비활성화 위치로 이동 (활성화 상태의 역방향 회전)
		_endMovementLocation = _deactivatePosition->GetComponentLocation();
		_endMovementRotator = _deactivatePosition->GetComponentRotation();
	}

	// 타이머 시작
	UpdateMovementTimer();
}

void AMovementAction::UpdateMovementTimer()
{
	float _deltaTime = GetWorld()->GetDeltaSeconds();
	_currentMovementTime += _deltaTime;
	float _alpha = _currentMovementTime / _movementDuration;

	if (_currentMovementTime >= _movementDuration)
	{
		// 최종 위치와 회전 설정
		MovePosition(_endMovementLocation, _endMovementRotator);

		// 타이머 중지
		StopMovement();
		return;
	}

	// 위치 보간
	FVector _nextLocation = FMath::Lerp(_startMovementLocation, _endMovementLocation, _alpha);
	
	// 회전 보간 (일관된 방향으로 회전하도록 Slerp 사용)
	FRotator _nextRotator;
	
	if (_isActivate)
	{
		// 활성화 방향으로 이동 시 정방향 회전
		_nextRotator = FMath::Lerp(_startMovementRotator, _endMovementRotator, _alpha);
	}
	else
	{
		// 비활성화 방향으로 이동 시 역방향 회전 (활성화 방향의 역재생)
		_nextRotator = FMath::Lerp(_startMovementRotator, _endMovementRotator, _alpha);
	}
	
	// 위치와 회전 적용
	MovePosition(_nextLocation, _nextRotator);

	// 다음 업데이트를 위한 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(_movementTimerHandle, this, &AMovementAction::UpdateMovementTimer, _deltaTime, false);
}

void AMovementAction::StopMovement()
{
	GetWorld()->GetTimerManager().ClearTimer(_movementTimerHandle);
}

void AMovementAction::MovePosition(FVector Location, FRotator Rotator)
{
	// 이동 메시의 이전 상태 저장
	FVector _oldMeshLocation = _movementMesh->GetComponentLocation();
	FQuat _oldMeshRotation = _movementMesh->GetComponentRotation().Quaternion();
	
	// 이동 메시의 위치와 회전 설정
	_movementMesh->SetWorldLocation(Location);
	_movementMesh->SetWorldRotation(Rotator);

	// 이동할 액터가 설정되어 있는 경우
	if (_moveWithActor != nullptr)
	{
		// 회전 변화량 계산
		FQuat _deltaRotation = _movementMesh->GetComponentRotation().Quaternion() * _oldMeshRotation.Inverse();
		
		// _moveWithActor의 현재 위치를 _movementMesh 기준 상대 위치로 변환
		FVector _relativePos = _moveWithActor->GetActorLocation() - _oldMeshLocation;
		
		// 회전 적용 (_movementMesh의 중심점을 기준으로 회전)
		FVector _rotatedPos = _deltaRotation.RotateVector(_relativePos);
		
		// 새 위치 계산 (_movementMesh의 새 위치 + 회전된 상대 위치)
		FVector _newActorLocation = Location + _rotatedPos;
		
		// 회전 계산 (기존 회전에 변화량 적용)
		FQuat _newActorRotation = _deltaRotation * _moveWithActor->GetActorRotation().Quaternion();
		
		// 이동할 액터의 위치와 회전 설정
		_moveWithActor->SetActorLocation(_newActorLocation);
		_moveWithActor->SetActorRotation(_newActorRotation.Rotator());
	}
}