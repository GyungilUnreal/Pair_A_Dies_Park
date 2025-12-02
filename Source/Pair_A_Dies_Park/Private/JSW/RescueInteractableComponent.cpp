// Fill out your copyright notice in the Description page of Project Settings.


#include "JSW/RescueInteractableComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JSW/PlayerFallComponent.h"

URescueInteractableComponent::URescueInteractableComponent()
{
	// 기본적으로는 상호작용 불가능
	bIsInteractable = false;
}

void URescueInteractableComponent::GazeInteract_Implementation(AActor* InstigatorActor)
{
	ACharacter* RescuerChar = Cast<ACharacter>(InstigatorActor);
	if (RescuerChar)
	{
		UCharacterMovementComponent* Movement = RescuerChar->GetCharacterMovement();

		// 땅을 밟고 있지 않다면
		if (Movement && !Movement->IsMovingOnGround())
		{
			return;
		}
	}

	// 나(구출 대상)를 가져옴
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	// 내 FallComponent를 찾아서 구조 명령 실행
	UPlayerFallComponent* FallComp = OwnerCharacter->FindComponentByClass<UPlayerFallComponent>();
	if (FallComp)
	{
		// 구조 실행 (위치 이동 등)
		FallComp->TryRescue(InstigatorActor);

		// 변경사항을 모든 클라이언트에 전파(Replication)
		SetIsInteractable(false);
	}
}