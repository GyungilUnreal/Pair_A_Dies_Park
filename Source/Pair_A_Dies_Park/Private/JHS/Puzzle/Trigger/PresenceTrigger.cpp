// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Puzzle/Trigger/PresenceTrigger.h"
#include "Components/BoxComponent.h"
#include "Split_Character.h"

APresenceTrigger::APresenceTrigger()
{
    PrimaryActorTick.bCanEverTick = false;

	_rootComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RootComponent"));
	_rootComponent->SetupAttachment(RootComponent);

    _boxTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxTrigger"));
    _boxTrigger->SetupAttachment(_rootComponent);

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
	ASplit_Character* _player = Cast<ASplit_Character>(OtherActor);
	
	// 밟은 액터가 플레이어인지 확인
	if (_player == nullptr)
		return;

	// 서버인지 확인
	if (HasAuthority())
	{
		if (!_playerInZone.Contains(_player))
		{
			_playerInZone.Add(_player);
			if (_isDebugLog)
			{
				UE_LOG(LogTemp, Warning, TEXT("%s entered. Current players in zone: %d"), *_player->GetName(), _playerInZone.Num());
			}
		}

		if (_playerInZone.Num() >= _requirePlayerCount)
		{
			if (_isDebugLog)
			{
				UE_LOG(LogTemp, Warning, TEXT("Goal Zone Puzzle COMPLETED! (Required: %d, Current: %d)"), _requirePlayerCount, _playerInZone.Num());
			}
		
			// 퍼즐 완료
			OnTriggerEnter(); 
		}
	}
}

void APresenceTrigger::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ASplit_Character* _player = Cast<ASplit_Character>(OtherActor);

	if (_player == nullptr)
		return;

	// 서버인지 확인
	if (HasAuthority())
	{
		_playerInZone.Remove(_player);
		if (_isDebugLog)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s left. Current players in zone: %d"), *_player->GetName(), _playerInZone.Num());
		}

		if (_playerInZone.Num() < _requirePlayerCount)
		{
			OnTriggerExit();
		}
	}
}