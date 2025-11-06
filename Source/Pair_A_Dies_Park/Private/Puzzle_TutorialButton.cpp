// Fill out your copyright notice in the Description page of Project Settings.


#include "Puzzle_TutorialButton.h"
#include "Components/BoxComponent.h"
#include "TutorialPlatform.h"
#include "Split_Character.h"
#include "JHS/Room/RoomController.h"

APuzzle_TutorialButton::APuzzle_TutorialButton()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	RootComponent = TriggerVolume;

	// 오버랩 이벤트에 함수 바인딩
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &APuzzle_TutorialButton::OnOverlapBegin);

	TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &APuzzle_TutorialButton::OnOverlapEnd);

	bReplicates = true;
}

void APuzzle_TutorialButton::BeginPlay()
{
}

void APuzzle_TutorialButton::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 밟은 액터가 플레이어인지 확인
	if (Cast<ASplit_Character>(OtherActor) == nullptr)
		return;
	
	// 서버에서 실행하는지
	if (HasAuthority())
	{
		// 발판이 없는지
		if (TargetPlatform == nullptr)
			return;

		UE_LOG(LogTemp, Warning, TEXT("Tutorial Button Puzzle Activated"));

		// 발판 활성화
		TargetPlatform->ActivatePlatform();

		//Multicast_PlayButtonEffects();

	}
}

void APuzzle_TutorialButton::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// 뗀 액터가 플레이어인지 확인
	if (Cast<ASplit_Character>(OtherActor) == nullptr)
		return;

	// 서버에서 실행하는지
	if (HasAuthority())
	{
		if (TargetPlatform)
		{
			// 발판 비활성화.
			TargetPlatform->DeactivatePlatform();
		}

	}
}
