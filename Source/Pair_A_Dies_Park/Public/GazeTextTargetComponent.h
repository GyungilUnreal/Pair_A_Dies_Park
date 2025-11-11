#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GazeTextTargetComponent.generated.h"

/**
 * 시선으로 감지됐을 때 UI에 뿌릴 문구를 에디터에서 적어두는 컴포넌트
 * 액터에 이 컴포넌트를 붙여두면 GazeInteractorComponent 쪽에서 찾아서
 * 위젯의 TextBlock에 값을 밀어 넣음.
 */
UCLASS(ClassGroup=(Gaze), meta=(BlueprintSpawnableComponent))
class UGazeTextTargetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGazeTextTargetComponent();

	// 실제로 UI에 표시할 문구
	// 예: "열기", "대화하기"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_DisplayText, Category = "Gaze")
	FText DisplayText;

	// 위젯 안에서 TextBlock 이름 (없으면 전체 위젯에 SetText를 시도)
	// 예: "Text_Action"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gaze")
	FName TextWidgetName;

	// 이 함수 하나로 BP에서 "문구 바꾸고 모든 클라에 위젯 갱신"까지 한다
	UFUNCTION(BlueprintCallable, Category = "Gaze")
	void SetDisplayTextAndNotify(const FText& NewText);

protected:
	UFUNCTION()
	void OnRep_DisplayText();   // 클라에서 값 바뀌었을 때 호출될 함수

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

};