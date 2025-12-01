// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JHS/Room/RoomDataTable.h"
#include "JHS/GameControll/RoomSubsystem.h"
#include "Net/UnrealNetwork.h"

#include "RoomController.generated.h"

class APuzzleTriggerBase;
class APresenceTrigger;
class APuzzleActionBase;

USTRUCT(BlueprintType)
struct FPuzzleData
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle Data")
	bool IsPuzzleActivated = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle Data")
	bool IsLockActivatedAction = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle Data")
	TArray<TObjectPtr<APuzzleTriggerBase>> PuzzleTriggerArray;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle Data")
	TArray<TObjectPtr<APuzzleActionBase>> PuzzleActionArray;
};

UCLASS()
class ARoomController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARoomController();

private:
	UPROPERTY(Replicated)
	FRoomData _roomData;

	static const int32 PUZZLE_DATA_RATE = 100;

	static const int32 ROOM_CLEAR_DOOR_KEY = -1;
	
	// 액션 활성화 상태를 클라이언트에 알리기 위한 변수
	UPROPERTY(ReplicatedUsing=OnRep_ActivePuzzleIndices)
	TArray<int32> _activePuzzleIndices;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Room Controller|Room Data")
	E_ROOM_TYPE _roomType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Room Controller|Puzzle")
	TArray<FPuzzleData> _puzzleDataArray = TArray<FPuzzleData>();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Room Controller|Puzzle")
	TObjectPtr<APresenceTrigger> _roomClearDoor = nullptr;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	// 네트워크 복제 설정
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 클라이언트에서 서버로 트리거 상태 변경 요청
	UFUNCTION(Server, Reliable)
	void Server_ChangePuzzleTriggerState(int32 TriggerKey, bool IsTriggered);
	
	void ChangePuzzleTriggerState(int32 TriggerKey, bool IsTriggered);

	void OnActionDeactivated(int32 PuzzleKey);

private:
	void InitializeRoomController();

	// 클라이언트에서 서버로 액션 상태 변경 요청
	UFUNCTION(Server, Reliable)
	void Server_ChangePuzzleActionState(int32 PuzzleIndex, bool IsActive);
	
	void ChangePuzzleActionState(int32 PuzzleIndex, bool IsActive);
	
	// 액션 활성화 상태가 변경되었을 때 호출되는 함수
	UFUNCTION()
	void OnRep_ActivePuzzleIndices();
	
	// 퍼즐 액션 상태를 적용하는 공통 함수
	void ApplyPuzzleActionState(int32 PuzzleIndex, bool IsActive);

	bool TryGetPuzzleData(int32 PuzzleKey, FPuzzleData*& OutPuzzleData, int32& OutPuzzleIndex, int32& OutRawIndex);

	//bool TryGetValue(int32 PuzzleKey, bool*& OutValue);
};
