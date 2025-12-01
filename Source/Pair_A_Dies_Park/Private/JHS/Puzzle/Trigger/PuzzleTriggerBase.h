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
	// 트리거 활성화 상태 (복제됨)
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_IsTriggered)
	bool _isTriggered = false;

protected:
	int32 _puzzleKey = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presence Trigger|Trigger")
	bool _isLockOnTrigger = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presence Trigger|Visibility")
	bool _isChangeImmediately = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presence Trigger|Visibility")
	bool _isDeactiveOnTrigger = false;

public:
	bool IsTriggered() { return _isTriggered; }

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

	// 복제 상태가 변경되었을 때 호출되는 함수
	UFUNCTION()
	void OnRep_IsTriggered();

	// 네트워크 복제 설정
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void OnChangeIsTrigger(bool IsTriggered);

public:
	void InitializePuzzleTrigger(TObjectPtr<ARoomController> RoomController, int32 PuzzleKey);

	void ChangeTriggerVisibility(bool IsVisible);

private:
	void ChangeTriggered(bool IsTriggered);
};
