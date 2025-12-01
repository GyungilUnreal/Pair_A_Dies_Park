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

	TObjectPtr<UStaticMeshComponent> _endPosition = _isActivate ? _activatePosition : _deactivatePosition;

	_startMovementLocation = _movementMesh->GetComponentLocation();
	_endMovementLocation = _endPosition->GetComponentLocation();
	_startMovementRotator = _movementMesh->GetComponentRotation();
	_endMovementRotator = _endPosition->GetComponentRotation();

	// 타이머 시작
	UpdateMovementTimer();
}

void AMovementAction::UpdateMovementTimer()
{
	float _deltaTime = GetWorld()->GetDeltaSeconds();
	_currentMovementTime += _deltaTime;

	if (_currentMovementTime >= _movementDuration)
	{
		// 최종 위치와 회전 설정
		MovePosition(_endMovementLocation, _endMovementRotator);

		// 타이머 중지
		StopMovement();
		return;
	}

	// 위치와 회전 보간
	FVector _nextLocation = FMath::Lerp(_startMovementLocation, _endMovementLocation, _currentMovementTime / _movementDuration);
	FRotator _nextRotator = FMath::Lerp(_startMovementRotator, _endMovementRotator, _currentMovementTime / _movementDuration);
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
	_movementMesh->SetWorldLocation(Location);
	_movementMesh->SetWorldRotation(Rotator);
}