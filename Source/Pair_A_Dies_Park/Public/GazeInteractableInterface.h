#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GazeInteractableInterface.generated.h"

// 시선으로 감지된 대상이 실제로 어떤 상호작용을 할지 정의하는 인터페이스
// 액터든 컴포넌트든 이 인터페이스만 구현해두면 Gaze 컴포넌트에서 호출할 수 있음
UINTERFACE(BlueprintType)
class UGazeInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class IGazeInteractableInterface
{
	GENERATED_BODY()

public:
	// 상호작용 시 호출되는 함수
	// InstigatorActor: 상호작용을 시도한 플레이어(캐릭터)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GazeInterface|Interact")
	void GazeInteract(AActor* InstigatorActor);
};
