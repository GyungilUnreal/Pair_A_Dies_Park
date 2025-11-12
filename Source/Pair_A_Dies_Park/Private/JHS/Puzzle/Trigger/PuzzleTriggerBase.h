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
	bool _isTriggered = false;

protected:
	int32 _puzzleKey = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presence Trigger|Trigger")
	bool _isDeactiveOnTrigger = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	void InitializePuzzleTrigger(TObjectPtr<ARoomController> RoomController, int32 PuzzleKey);

	void DeactiveTrigger();

protected:
	void OnTriggerEnter();

	void OnTriggerExit();

private:
	void OnChangeTriggered(bool IsTriggered);
};
