#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractableComponent.generated.h"

/**
 * 플레이어가 상호작용할 수 있는 가장 기본이 되는 컴포넌트입니다.
 * 이후 집기, 버튼 누르기 등 구체적인 상호작용 컴포넌트들이 이 클래스를 상속받아 확장하도록 합니다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractableComponent();

protected:
	// 게임 시작 시 초기화
	virtual void BeginPlay() override;

	// 현재 상호작용이 가능한지 여부
	// 외부(플레이어)에서 이 함수를 호출해 현재 상호작용 가능한 상태인지 확인합니다.
	UFUNCTION(BlueprintCallable, Category="Interaction")
	bool CanInteract(AActor* Interactor) const;

	// 실제 상호작용 로직을 처리하는 함수 (자식 컴포넌트에서 오버라이드)
	UFUNCTION(BlueprintNativeEvent, Category="Interaction")
	void HandleInteract(AActor* Interactor);
	virtual void HandleInteract_Implementation(AActor* Interactor);

public:
	// 플레이어가 상호작용을 시도할 때 외부에서 호출하는 엔트리 포인트
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void Interact(AActor* Interactor);

	// 플레이어가 이 오브젝트를 바라보기 시작했을 때(하이라이트 등) 호출할 수 있는 함수
	UFUNCTION(BlueprintNativeEvent, Category="Interaction")
	void OnBeginFocus(AActor* Interactor);
	virtual void OnBeginFocus_Implementation(AActor* Interactor);

	// 플레이어가 이 오브젝트 보기를 멈췄을 때 호출할 수 있는 함수
	UFUNCTION(BlueprintNativeEvent, Category="Interaction")
	void OnEndFocus(AActor* Interactor);
	virtual void OnEndFocus_Implementation(AActor* Interactor);

	// 블루프린트에서 그냥 호출할 수 있는 래퍼 함수
	UFUNCTION(BlueprintCallable)
	void CallBeginFocus(AActor* Interactor);

	UFUNCTION(BlueprintCallable)
	void CallEndFocus(AActor* Interactor);

protected:
	// 현재 상호작용 가능한 상태인지 표시하는 플래그
	// 예: 문이 이미 열려 있다면 false로 바꿔서 재상호작용을 막을 수 있음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	bool bIsInteractable;

	// UI에 표시할 이름 (예: "스위치", "상자")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FText InteractionName;

	// UI에 표시할 프롬프트 (예: "E 키를 눌러 상호작용")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FText InteractionPrompt;

public:
	// 이름 가져오기
	UFUNCTION(BlueprintCallable, Category="Interaction")
	FText GetInteractionName() const { return InteractionName; }

	// 프롬프트 가져오기
	UFUNCTION(BlueprintCallable, Category="Interaction")
	FText GetInteractionPrompt() const { return InteractionPrompt; }

	// 외부에서 상호작용 가능 여부를 켜고 끌 수 있도록 함
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void SetInteractable(bool bNewInteractable) { bIsInteractable = bNewInteractable; }
};
