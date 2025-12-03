// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JHS/Puzzle/Action/PuzzleActionBase.h"
#include "MovementAction.generated.h"

class UStaticMeshComponent;

UCLASS()
class AMovementAction : public APuzzleActionBase
{
	GENERATED_BODY()

public:
	AMovementAction();

private:
#pragma region Movement Action
	FVector _startMovementLocation;

	FVector _endMovementLocation;

	FRotator _startMovementRotator;

	FRotator _endMovementRotator;

	float _currentMovementTime = 0.0f;

	FTimerHandle _movementTimerHandle;
#pragma endregion Movement Action

	// 이동할 액터의 초기 상대 위치와 회전 차이
	FVector _moveWithActorOffset;
	FRotator _moveWithActorRotationDiff;
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puzzle|Components")
	TObjectPtr<UStaticMeshComponent> _rootComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puzzle|Components")
	TObjectPtr<UStaticMeshComponent> _deactivatePosition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puzzle|Components")
	TObjectPtr<UStaticMeshComponent> _activatePosition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Movement Action")
	float _movementDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Movement Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> _movementMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Movement Action")
	TObjectPtr<AActor> _moveWithActor;

protected:
	virtual void PreInitializeComponents();

	virtual void BeginPlay() override;

	virtual void OnActivatePuzzleAction() override;

	virtual void OnDeactivatePuzzleAction() override;

private:
	void StartMovement();

	void UpdateMovementTimer();

	void StopMovement();

	void MovePosition(FVector Location, FRotator Rotator);
};
