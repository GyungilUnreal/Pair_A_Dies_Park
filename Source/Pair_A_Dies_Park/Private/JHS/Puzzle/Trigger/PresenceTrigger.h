// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JHS/Puzzle/Trigger/PuzzleTriggerBase.h"
#include "PresenceTrigger.generated.h"

class UBoxComponent;
class APlayerBase;

UCLASS()
class APresenceTrigger : public APuzzleTriggerBase
{
	GENERATED_BODY()

public:
	APresenceTrigger();

private:
	TObjectPtr<UBoxComponent> _boxTrigger = nullptr;

	TArray<TObjectPtr<APlayerBase>> _playerInZone;
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puzzle|Components")
	TObjectPtr<UStaticMeshComponent> _rootComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presence Trigger|Debug")
	bool _isDebugLog = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presence Trigger|Trigger")
	int32 _requirePlayerCount = 1;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
