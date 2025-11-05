// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PuzzleBase.h"
#include "Puzzle_GoalZone.generated.h"

class UBoxComponent;
class ASplit_Character;

UCLASS()
class APuzzle_GoalZone : public APuzzleBase
{
	GENERATED_BODY()
	
public:
	APuzzle_GoalZone();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerVolume;
	
	// 통과를 위한 플레이어 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle Logic")
	int32 RequiredPlayerCount = 1;

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	// 존 안에 있는 플레이어 목록.
	UPROPERTY()
	TArray<TObjectPtr<ASplit_Character>> PlayerInZone;

};
