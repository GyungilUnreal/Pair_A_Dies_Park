#pragma once

#include "CoreMinimal.h"
#include "InteractableComponent.h"
#include "PickupInteractableComponent.generated.h"

/**
 * 물건을 집어드는 상호작용을 위한 컴포넌트 예시입니다.
 * 기본 상호작용 컴포넌트(UInteractableComponent)를 상속받아 실제 집기 로직을 구현하도록 합니다.
 * 실제로는 플레이어의 인벤토리 시스템이나 손에 부착하는 시스템과 연동해서 사용하세요.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UPickupInteractableComponent : public UInteractableComponent
{
	GENERATED_BODY()

public:
	UPickupInteractableComponent();

protected:
	// 실제 상호작용 처리
	virtual void HandleInteract_Implementation(AActor* Interactor) override;

	// 집었는지 여부 (한 번만 집게 하고 싶을 때 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Pickup")
	bool bCanBePickedUp;

	// 플레이어가 집었을 때 어떤 소켓 이름에 붙일지 (예: "Hand_R")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Pickup")
	FName AttachSocketName;
};
