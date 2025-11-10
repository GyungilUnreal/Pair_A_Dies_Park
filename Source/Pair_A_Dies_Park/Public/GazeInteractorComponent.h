#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GazeInteractableInterface.h"
#include "GazeInteractorComponent.generated.h"

class UUserWidget;

// 감지할 컴포넌트와 띄울 UI를 한 쌍으로 묶은 구조체
USTRUCT(BlueprintType)
struct FGazeDetectSet
{
	GENERATED_BODY()

	// 이 컴포넌트를 가진 액터를 보면
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gaze")
	TSubclassOf<UActorComponent> TargetComponentClass;

	// 이 위젯을 띄운다 (없으면 UI 안 띄움)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gaze")
	TSubclassOf<UUserWidget> WidgetClass;
};

// 협동(멀티) 게임용 Gaze 인터랙터 컴포넌트
// - 로컬에서만 감지해서 UI 띄우기 가능 (bOnlyLocal)
// - 디버그 라인 on/off 가능 (bDrawDebugLine)
// - 클라 입력 -> 서버 RPC -> 대상 인터페이스 호출 구조
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UGazeInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGazeInteractorComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 캐릭터가 입력으로 호출하는 함수 (클라에서 호출)
	UFUNCTION(BlueprintCallable, Category="Gaze|Interact")
	void TryInteract(AActor* InstigatorActor);

	// 서버에서 실제 상호작용을 수행하는 RPC
	UFUNCTION(Server, Reliable)
	void ServerTryInteract(AActor* InstigatorActor, AActor* TargetActor, int32 SetIndex);

private:
	// 시선 트레이스
	void PerformGazeTrace();

	// 어떤 세트와 맞는지 찾기
	int32 FindMatchedSetIndex(AActor* Actor) const;

	// UI 띄우기 (로컬)
	void ShowWidgetForSet(AActor* TargetActor, int32 SetIndex);

	// UI 숨기기 (로컬)
	void HideCurrentWidget();

	// 서버/클라 공통 인터랙트 처리
	void ProcessInteract(AActor* InstigatorActor, AActor* TargetActor, int32 SetIndex);

	// 이 컴포넌트를 가진 오너가 로컬 컨트롤러인지 확인
	bool IsOwnerLocal() const;

private:
	// 현재 감지한 액터
	UPROPERTY(Transient)
	AActor* CurrentTargetActor;

	// 현재 감지한 세트 인덱스
	int32 CurrentSetIndex;

	// 감지한 실제 컴포넌트 (인터랙트용) - 약한 참조로 보관
	TWeakObjectPtr<UActorComponent> CurrentMatchedComponent;

	UPROPERTY(Transient)
	class UWidgetComponent* CurrentWidgetComp;

public:
	// 트레이스 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gaze")
	float TraceDistance;

	// 트레이스 채널
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gaze")
	TEnumAsByte<ECollisionChannel> TraceChannel;

	// 감지 세트들
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gaze")
	TArray<FGazeDetectSet> DetectSets;

	// 디버그 라인 그릴지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gaze|Debug")
	bool bDrawDebugLine;

	// 이 컴포넌트가 붙은 오너가 로컬일 때만 UI/트레이스를 처리할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gaze|Debug")
	bool bOnlyLocal;
};
