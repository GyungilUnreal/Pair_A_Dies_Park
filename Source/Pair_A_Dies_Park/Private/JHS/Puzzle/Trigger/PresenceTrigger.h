// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JHS/Puzzle/Trigger/PuzzleTriggerBase.h"
#include "PresenceTrigger.generated.h"

class UBoxComponent;
class ASplit_Character;

UCLASS()
class APresenceTrigger : public APuzzleTriggerBase
{
	GENERATED_BODY()

public:
	APresenceTrigger();

private:
	TObjectPtr<UBoxComponent> _boxTrigger = nullptr;

	TArray<TObjectPtr<ASplit_Character>> _playerInZone;

	TObjectPtr<UStaticMeshComponent> _triggerMesh = nullptr;
	
	// 복제되는 트리거 활성화 상태
	UPROPERTY(ReplicatedUsing=OnRep_IsActivated)
	bool _isActivated = false;
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puzzle|Components")
	TObjectPtr<UStaticMeshComponent> _rootComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presence Trigger|Debug")
	bool _isDebugLog = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presence Trigger|Player Count")
	int32 _requirePlayerCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presence Trigger|Material")
	FName _triggerMeshName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presence Trigger|Material")
	TObjectPtr<UMaterial> _matDeactivate = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presence Trigger|Material")
	TObjectPtr<UMaterial> _matActivate = nullptr;

protected:
	virtual void BeginPlay() override;

	virtual void TriggerEnterEffect() override;

	virtual void TriggerExitEffect() override;

private:
	// 마테리얼 변경 실행 함수
	void ChangeMaterial(bool IsActivated);
	
	// 복제 상태가 변경되었을 때 호출되는 함수
	UFUNCTION()
	void OnRep_IsActivated();
	
	// 네트워크 복제 설정
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
