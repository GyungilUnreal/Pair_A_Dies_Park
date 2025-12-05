// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleTriggerBase.generated.h"

class ARoomController;

UENUM(BlueprintType)
enum class E_PUZZLE_TRIGGER_TYPE : uint8
{
	Presence = 0 UMETA(DisplayName = "Presence"),			// 위치
	Button UMETA(DisplayName = "Button"),					// 버튼
	Destroy UMETA(DisplayName = "Destroy"),					// 파괴

	SIZE UMETA(DisplayName = "SIZE")
};

UCLASS()
class APuzzleTriggerBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APuzzleTriggerBase();

private:
	TObjectPtr<ARoomController> _roomController = nullptr;

protected:
	// 트리거 활성화 상태 (멀티캐스트로 동기화)
	UPROPERTY()
	bool _isTriggered = false;

protected:
	int32 _puzzleKey = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger|Trigger")
	bool _isLockOnTrigger = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger|Timer")
	bool _isTimerTrigger = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger|Visibility")
	bool _isChangeImmediately = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger|Visibility")
	bool _isDeactiveOnTrigger = false;

public:
	bool IsTriggered() { return _isTriggered; }

	bool IsTimerTrigger() { return _isTimerTrigger; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void TriggerEnterEffect() { }

	virtual void TriggerExitEffect() { }

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	void OnTriggerEnter();

	void OnTriggerExit();

public:
	void InitializePuzzleTrigger(TObjectPtr<ARoomController> RoomController, int32 PuzzleKey);

#pragma region Change Triggered
private:
	void ChangeTriggered(bool IsTriggered);

public:
	// 트리거 상태를 변경하는 서버 RPC 함수
	UFUNCTION(Server, Reliable)
	void Server_ChangeTriggered(bool IsTriggered);
	
	// 트리거 상태를 변경하는 멀티캐스트 함수
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ChangeTriggered(bool IsTriggered);
#pragma endregion Change Triggered

public:
	void OnChangeAction(bool IsActionActivate);

	void ResetTrigger();

private:
	void ChangeTriggerVisibility(bool IsVisible);
};
