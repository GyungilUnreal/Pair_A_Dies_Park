// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle_TutorialButton.generated.h"

class UBoxComponent;
class ATutorialPlatform;

UCLASS()
class APuzzle_TutorialButton : public AActor
{
	GENERATED_BODY()
	
public:
	APuzzle_TutorialButton();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	// 레벨에 배치하고 연결할 액터
	UPROPERTY(EditInstanceOnly, Category = "Puzzle Logic")
	TObjectPtr<ATutorialPlatform> TargetPlatform;

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

	//// 버튼 눌림 효과.
	//UFUNCTION(NetMulticast)
	//void Multicast_PlayButtonEffects();
};
