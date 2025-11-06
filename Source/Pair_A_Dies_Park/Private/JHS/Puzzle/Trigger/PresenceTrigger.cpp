// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Puzzle/Trigger/PresenceTrigger.h"
#include "Components/BoxComponent.h"
#include "Split_Character.h"

APresenceTrigger::APresenceTrigger()
{
    PrimaryActorTick.bCanEverTick = false;

    _boxTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxTrigger"));
    _boxTrigger->SetupAttachment(RootComponent);

    // 오버랩 이벤트에 함수 바인딩
    _boxTrigger->OnComponentBeginOverlap.AddDynamic(this, &APresenceTrigger::OnOverlapBegin);
    _boxTrigger->OnComponentEndOverlap.AddDynamic(this, &APresenceTrigger::OnOverlapEnd);

    bReplicates = true;
}

void APresenceTrigger::BeginPlay()
{
	Super::BeginPlay();
}

void APresenceTrigger::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ASplit_Character* Player = Cast<ASplit_Character>(OtherActor);
	
	// 밟은 액터가 플레이어인지 확인
	if (Player == nullptr)
		return;

	if (HasAuthority())
	{
		// 중복 추가 방지하며 배열에 추가
		if (!_playerInZone.Contains(Player))
		{
			_playerInZone.Add(Player);
			UE_LOG(LogTemp, Warning, TEXT("%s entered. Current players in zone: %d"), *Player->GetName(), _playerInZone.Num());
		}

		// 인원 수 체크
		if (_playerInZone.Num() >= _requirePlayerCount)
		{
			UE_LOG(LogTemp, Warning, TEXT("Goal Zone Puzzle COMPLETED! (Required: %d, Current: %d)"), _requirePlayerCount, _playerInZone.Num());
		
			// 퍼즐 완료
			OnTriggerEnter(); 
		}
	}
}

void APresenceTrigger::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ASplit_Character* Player = Cast<ASplit_Character>(OtherActor);

	// 뗀 액터가 플레이어인지 확인
	if (Player == nullptr)
		return;

	// 서버에서 실행하는지
	if (HasAuthority())
	{
		// 플레이어 제거 로직
		_playerInZone.Remove(Player);
		UE_LOG(LogTemp, Warning, TEXT("%s left. Current players in zone: %d"), *Player->GetName(), _playerInZone.Num());

		// 인원 수가 모자라면 '트리거 꺼짐' 신호 전송
		if (_playerInZone.Num() < _requirePlayerCount)
		{
			OnTriggerExit();
		}
	}
}