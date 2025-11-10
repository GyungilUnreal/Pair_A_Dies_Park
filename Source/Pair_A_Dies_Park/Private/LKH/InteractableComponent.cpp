#include "InteractableComponent.h"
#include "GameFramework/Actor.h"
#include "Components/WidgetComponent.h"

UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 기본값으로 상호작용을 가능하게 설정
	bIsInteractable = true;
}

void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UInteractableComponent::CanInteract(AActor* Interactor) const
{
	// 여기서 Interactor가 유효한지, 거리가 너무 멀지는 않은지 등을 추가로 검사할 수 있음
	// 지금은 단순히 플래그만 확인
	return bIsInteractable;
}

void UInteractableComponent::Interact(AActor* Interactor)
{
	// 상호작용 가능한 상태가 아니면 무시
	if (!CanInteract(Interactor))
	{
		return;
	}

	// 실제 상호작용 처리 (자식에서 오버라이드 가능)
	HandleInteract(Interactor);
}

void UInteractableComponent::HandleInteract_Implementation(AActor* Interactor)
{
	// 베이스 컴포넌트에서는 아무 것도 하지 않음
	// 자식 클래스에서 구체적인 행동(문 열기, 아이템 줍기 등)을 구현하세요.
}