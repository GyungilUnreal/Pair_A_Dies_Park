// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Puzzle/Trigger/PresenceTrigger.h"
#include "Components/BoxComponent.h"
#include "Split_Character.h"
#include "Net/UnrealNetwork.h"

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
	
	// _triggerMeshName으로 _rootComponent 하위의 UStaticMeshComponent 찾기
	if (!_triggerMeshName.IsNone())
	{
		TArray<USceneComponent*> _childComponents;
		_rootComponent->GetChildrenComponents(true, _childComponents);
		
		for (USceneComponent* _childComponent : _childComponents)
		{
			UStaticMeshComponent* _staticMeshComponent = Cast<UStaticMeshComponent>(_childComponent);
			if (_staticMeshComponent && _staticMeshComponent->GetFName() == _triggerMeshName)
			{
				_triggerMesh = _staticMeshComponent;
				break;
			}
		}
		
		// 직접 자식 컴포넌트에서 찾지 못한 경우 루트 컴포넌트 자체를 확인
		if (_triggerMesh == nullptr && _rootComponent->GetFName() == _triggerMeshName)
		{
			_triggerMesh = Cast<UStaticMeshComponent>(_rootComponent);
		}
	}
}

void APresenceTrigger::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// 복제할 속성 등록
	DOREPLIFETIME(APresenceTrigger, _isActivated);
}

void APresenceTrigger::TriggerEnterEffect()
{
	// 서버에서만 상태 변경
	if (HasAuthority())
	{
		_isActivated = true;
		ChangeMaterial(true);
	}
}

void APresenceTrigger::TriggerExitEffect()
{
	// 서버에서만 상태 변경
	if (HasAuthority())
	{
		_isActivated = false;
		ChangeMaterial(false);
	}
}

void APresenceTrigger::OnRep_IsActivated()
{
	// 클라이언트에서 실행 - 복제된 상태에 따라 마테리얼 변경
	ChangeMaterial(_isActivated);
	
	if (_isDebugLog)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnRep_IsActivated: %s"), _isActivated ? TEXT("Activated") : TEXT("Deactivated"));
	}
}

void APresenceTrigger::ChangeMaterial(bool IsActivated)
{
	// 실제 마테리얼 변경 로직
	if (!_triggerMesh)
		return;

	if (IsActivated && _matActivate)
	{
		_triggerMesh->SetMaterial(0, _matActivate);
		
		if (_isDebugLog)
		{
			UE_LOG(LogTemp, Warning, TEXT("ChangeMaterial: Changed material to Activate"));
		}
	}
	else if (!IsActivated && _matDeactivate)
	{
		_triggerMesh->SetMaterial(0, _matDeactivate);
		
		if (_isDebugLog)
		{
			UE_LOG(LogTemp, Warning, TEXT("ChangeMaterial: Changed material to Deactivate"));
		}
	}
}

void APresenceTrigger::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// TODO : 플레이어 스크립트 통일
	ASplit_Character* _player = Cast<ASplit_Character>(OtherActor);
	
	if (_player == nullptr)
		return;

	// 서버인지 확인
	if (!HasAuthority())
		return;
	
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

void APresenceTrigger::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ASplit_Character* _player = Cast<ASplit_Character>(OtherActor);

	if (_player == nullptr)
		return;

	// 서버인지 확인
	if (!HasAuthority())
		return;
	
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