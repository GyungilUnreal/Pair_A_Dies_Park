#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GazeInteractableComponent.generated.h"

class UGazeInteractorComponent;

/**
 * 이 컴포넌트를 가진 액터가 시선 인터랙트되면
 * 서버에서 멀티캐스트로 모든 클라에 "이 액터에 대한 위젯 내려"라고 알린다.
 */
UCLASS(ClassGroup = (Gaze), meta = (BlueprintSpawnableComponent))
class UGazeInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGazeInteractableComponent();

	/** 서버에서 호출해: 이 액터가 지금 시선 인터랙트 되었다고 알림 */
	UFUNCTION(BlueprintCallable, Category = "Gaze")
	void NotifyGazeInteracted(AActor* InstigatorActor, int32 SetIndex);

protected:
	/** 모든 클라에서 호출돼서 실제로 UI 내리는 부분 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyGazeInteracted(AActor* InstigatorActor, int32 SetIndex);

	/** BP에서 후킹하고 싶으면 여기 구현해서 쓰면 됨 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Gaze")
	void OnGazeInteracted(AActor* InstigatorActor, int32 SetIndex);
};