// Fill out your copyright notice in the Description page of Project Settings.


#include "Puzzle_TutorialButton.h"
#include "Components/BoxComponent.h"
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
	ASplit_Character* Player = Cast<ASplit_Character>(OtherActor);
	
	// 밟은 액터가 플레이어인지 확인
	if (Player == nullptr)
		return;

	if (HasAuthority())
	{
		// 중복 추가 방지하며 배열에 추가
		if (!PlayerInZone.Contains(Player))
		{
			PlayerInZone.Add(Player);
			UE_LOG(LogTemp, Warning, TEXT("%s entered. Current players in zone: %d"), *Player->GetName(), PlayerInZone.Num());
		}

		// 인원 수 체크
		if (PlayerInZone.Num() >= RequiredPlayerCount)
		{
			UE_LOG(LogTemp, Warning, TEXT("Goal Zone Puzzle COMPLETED! (Required: %d, Current: %d)"), RequiredPlayerCount, PlayerInZone.Num());
		
			// 퍼즐 완료
			OnTriggerEnter(); 
		}
	}
}

void APuzzle_TutorialButton::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ASplit_Character* Player = Cast<ASplit_Character>(OtherActor);

	// 뗀 액터가 플레이어인지 확인
	if (Player == nullptr)
		return;

	// 서버에서 실행하는지
	if (HasAuthority())
	{
		// 플레이어 제거 로직
		PlayerInZone.Remove(Player);
		UE_LOG(LogTemp, Warning, TEXT("%s left. Current players in zone: %d"), *Player->GetName(), PlayerInZone.Num());

		// 인원 수가 모자라면 '트리거 꺼짐' 신호 전송
		if (PlayerInZone.Num() < RequiredPlayerCount)
		{
			OnTriggerExit();
		}
	}
}
