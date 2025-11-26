// Fill out your copyright notice in the Description page of Project Settings.


#include "JSW/RescueInteractableComponent.h"
#include "GameFramework/Character.h"
#include "JSW/PlayerFallComponent.h"

URescueInteractableComponent::URescueInteractableComponent()
{
	// 기본적으로는 상호작용 불가능
	bIsInteractable = false;
}

void URescueInteractableComponent::GazeInteract_Implementation(AActor* InstigatorActor)
{
	UE_LOG(LogTemp, Error, TEXT(">>> GazeInteract Called! Instigator: %s <<<"), *InstigatorActor->GetName());
	// 나(구출 대상)를 가져옴
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	// 내 FallComponent를 찾아서 구조 명령 실행
	UPlayerFallComponent* FallComp = OwnerCharacter->FindComponentByClass<UPlayerFallComponent>();
	if (FallComp)
	{
		UE_LOG(LogTemp, Error, TEXT(">>> Found FallComponent, Trying Rescue... <<<"));
		// 구조 실행 (위치 이동 등)
		FallComp->TryRescue(InstigatorActor);

		// 구조되었으니 다시 상호작용 불가 상태로 변경하고,
		// 변경사항을 모든 클라이언트에 전파(Replication)
		SetIsInteractable(false);
	}
	else {
		UE_LOG(LogTemp, Error, TEXT(">>> FallComponent NOT Found! <<<"));
	}
	// GazeSystem의 기본 로직(델리게이트 방송 등)도 수행하고 싶다면 호출
	// NotifyGazeInteracted(InstigatorActor, 0); // 필요 시 호출
}