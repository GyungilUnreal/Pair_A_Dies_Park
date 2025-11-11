#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GazeInteractableComponent.generated.h"

class UGazeInteractorComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGazeInteractedSignature, AActor*, InstigatorActor, int32, SetIndex);

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

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	/** 서버에서만 호출: 이 액터가 지금 시선 인터랙트 되었다고 알림 */
	UFUNCTION()
	void NotifyGazeInteracted(AActor* InstigatorActor, int32 SetIndex);

	/** BP에서 바인딩 가능한 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Gaze")
	FOnGazeInteractedSignature OnGazeInteracted;

	// 텍스트 갱신용: 클라에서도 부를 수 있게 Server RPC로 열어준다
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Gaze")
	void Server_RefreshGazeText();

	// 실제로 각 클라의 GazeInteractor들을 훑어서 텍스트 다시 적용시키는 멀티캐스트
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_RefreshGazeText();

protected:
	/** 모든 클라에서 호출돼서 실제로 UI 내리는 부분 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyGazeInteracted(AActor* InstigatorActor, int32 SetIndex);

public:
	UFUNCTION(BlueprintCallable, Category = "Gaze")
	void SetIsInteractable(bool bNew);

	UFUNCTION(BlueprintCallable, Category = "Gaze")
	bool GetIsInteractable() const { return bIsInteractable; }

protected:
	UFUNCTION()
	void OnRep_IsInteractable();

	void HandleInteractableChanged();

	// 인터랙트 가능 여부
	UPROPERTY(ReplicatedUsing = OnRep_IsInteractable)
	bool bIsInteractable;
};
