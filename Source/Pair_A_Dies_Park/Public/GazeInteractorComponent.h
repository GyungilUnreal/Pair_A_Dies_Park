#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GazeInteractorComponent.generated.h"

class UUserWidget;
class UWidgetComponent;
class UCameraComponent;

USTRUCT(BlueprintType)
struct FGazeDetectSet
{
	GENERATED_BODY()

	// 이 컴포넌트가 있으면 "인터랙션 대상"으로 본다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gaze")
	TSubclassOf<UActorComponent> TargetComponentClass;

	// 실제로 타깃이 됐을 때 띄울 UI
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gaze")
	TSubclassOf<UUserWidget> WidgetClass;

	// "너도 인터랙션 가능해" 라고 후보에게 보여줄 UI
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gaze")
	TSubclassOf<UUserWidget> CandidateWidgetClass;
};

// 후보 정보
USTRUCT()
struct FGazeCandidate
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AActor> Actor;

	// 어떤 DetectSet에 맞았는지
	UPROPERTY()
	int32 SetIndex = INDEX_NONE;

	// 우선순위 비교용으로 시선 시작점에서의 거리 보관
	UPROPERTY()
	float DistSqFromView = 0.f;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
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

	// 플레이어가 실제로 "사용"을 눌렀을 때
	UFUNCTION(BlueprintCallable, Category = "Gaze|Interact")
	void TryInteract(AActor* InstigatorActor);

	UFUNCTION(Server, Reliable)
	void ServerTryInteract(AActor* InstigatorActor, AActor* TargetActor, int32 SetIndex);

protected:
	void PerformGazeTrace();

	// 액터 안에서 DetectSets에 맞는 컴포넌트 찾기
	bool FindMatchedComponentInActor(AActor* Actor, UActorComponent*& OutMatchedComp, int32& OutSetIndex) const;

	// 시선 라인을 여러 구간으로 스윕해서 후보 수집
	void CollectCandidatesAlongGaze(const FVector& Start, const FVector& Forward, TArray<FGazeCandidate>& InOutCandidates, FGazeCandidate& InOutBest, const FCollisionQueryParams& Params);

	// 플레이어 주변을 한 번에 오버랩해서 후보 수집 (1단계에서 못 찾았을 때)
	void CollectCandidatesAroundPlayer(const FVector& Origin, TArray<FGazeCandidate>& InOutCandidates, FGazeCandidate& InOutBest, const FCollisionQueryParams& Params);

	// 최종 타깃 위젯 표시
	void ShowWidgetForSet(AActor* TargetActor, int32 SetIndex);

	// 후보들 위젯 표시/정리
	void UpdateCandidateWidgets(const TArray<FGazeCandidate>& Candidates, const FGazeCandidate& FinalTarget);

	void HideCurrentWidget();

	void ProcessInteract(AActor* InstigatorActor, AActor* TargetActor, int32 SetIndex);

	bool IsOwnerLocal() const;

	// 액터에 위젯 컴포넌트 하나 붙이는 헬퍼
	UWidgetComponent* SpawnWidgetOnActor(AActor* TargetActor, TSubclassOf<UUserWidget> WidgetClass) const;

private:
	// 현재 바라보고 있는 최종 타깃
	UPROPERTY(Transient)
	AActor* CurrentTargetActor;

	// 타깃이 어떤 DetectSet인지
	int32 CurrentSetIndex;

	// 타깃이 가진 실제 컴포넌트
	TWeakObjectPtr<UActorComponent> CurrentMatchedComponent;

	// 메인 위젯
	UPROPERTY(Transient)
	UWidgetComponent* CurrentWidgetComp;

	// 후보 UI들을 관리할 맵
	UPROPERTY(Transient)
	TMap<TWeakObjectPtr<AActor>, UWidgetComponent*> CandidateWidgetMap;

public:
	// 시선 최대 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gaze")
	float TraceDistance;

	// 시선 라인을 몇 구간으로 나눌지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gaze")
	int32 GazeSweepSteps;

	// 각 구간에서 스윕할 반경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gaze")
	float ComponentDetectRadius;

	// 2단계: 플레이어 주변 오버랩 반경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gaze")
	float PlayerOverlapRadius;

	// 어떤 채널로 트레이스/스윕할지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gaze")
	TEnumAsByte<ECollisionChannel> TraceChannel;

	// 감지 세트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gaze")
	TArray<FGazeDetectSet> DetectSets;

	// 디버그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gaze|Debug")
	bool bDrawDebugLine;

	// 로컬만
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gaze|Debug")
	bool bOnlyLocal;
};