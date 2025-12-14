#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SessionEntryWidget.generated.h"

class UButton;
class UTextBlock;
class USteamSessionSubsystem;

UCLASS()
class USessionEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 초기화 함수: C++에서 생성 후 바로 호출
    void InitEntry(USteamSessionSubsystem* InSubsystem, int32 InSessionIndex, const FString& InOwnerName);

protected:
    virtual void NativeConstruct() override;

    // 블루프린트 디자이너에서 Button / TextBlock 을 BindWidget 으로 연결
    UPROPERTY(meta = (BindWidget))
    UButton* JoinButton;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* SessionText;

    // 연결된 Subsystem (JoinSession 호출용)
    UPROPERTY()
    USteamSessionSubsystem* Subsystem;

    // 이 항목이 나타내는 세션 인덱스
    int32 SessionIndex;

    // 버튼 클릭 핸들러
    UFUNCTION()
    void OnJoinButtonClicked();

private:
    bool bJoinInProgress = false;

    UFUNCTION()
    void HandleJoinFinished(int32 FinishedIndex, bool bSuccess);
};
