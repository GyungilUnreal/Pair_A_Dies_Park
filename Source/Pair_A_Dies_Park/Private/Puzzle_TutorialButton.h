// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JHS/Puzzle/Trigger/PuzzleTriggerBase.h"
#include "Puzzle_TutorialButton.generated.h"

class UBoxComponent;
class ASplit_Character;

UCLASS()
class APuzzle_TutorialButton : public APuzzleTriggerBase
{
	GENERATED_BODY()
	
public:
	APuzzle_TutorialButton();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

protected:
	// 부모의 BeginPlay를 막기위해.
	virtual void BeginPlay() override;

	// 플레이어가 밟았을때.
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// 통과를 위한 플레이어 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle Logic")
	int32 RequiredPlayerCount = 1;

	//// 버튼 눌림 효과.
	//UFUNCTION(NetMulticast)
	//void Multicast_PlayButtonEffects();

private:
	// 존 안에 있는 플레이어 목록.
	UPROPERTY()
	TArray<TObjectPtr<ASplit_Character>> PlayerInZone;
};
