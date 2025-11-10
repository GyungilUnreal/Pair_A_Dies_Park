#include "PickupInteractableComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

UPickupInteractableComponent::UPickupInteractableComponent()
{
	// 기본값 설정
	bCanBePickedUp = true;
	AttachSocketName = TEXT("hand_r"); // 프로젝트에 맞게 소켓 이름을 맞춰주세요.
}

void UPickupInteractableComponent::HandleInteract_Implementation(AActor* Interactor)
{
	// 더 이상 집을 수 없는 상태면 무시
	if (!bCanBePickedUp)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !Interactor)
	{
		return;
	}

	// 플레이어가 캐릭터인지 확인 (손 소켓에 붙일 예정이므로)
	ACharacter* Character = Cast<ACharacter>(Interactor);
	if (Character)
	{
		// 메시에 붙이기 위한 SkeletalMesh 찾기
		USkeletalMeshComponent* MeshComp = Character->GetMesh();
		if (MeshComp)
		{
			// 소켓에 부착
			OwnerActor->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocketName);
			// 한 번 집었으면 다시 못 집게 하고 싶을 때 플래그 끄기
			bCanBePickedUp = false;
			bIsInteractable = false;
		}
	}

	// 만약 인벤토리 시스템과 연동한다면 여기서 OwnerActor를 숨기거나 제거하는 로직을 넣으면 됩니다.
	// OwnerActor->SetActorHiddenInGame(true);
	// OwnerActor->SetActorEnableCollision(false);
}
