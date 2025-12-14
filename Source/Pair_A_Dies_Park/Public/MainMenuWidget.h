#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UPanelWidget;
class USessionEntryWidget;
class USteamSessionSubsystem;

/**
 * 메인 메뉴 UI 위젯
 * - USteamSessionSubsystem 의 HostSession / FindSessions / JoinSessionByIndex 를 호출해서 세션을 제어
 * - 블루프린트에서 세션 리스트 UI를 만들 수 있도록 최소한의 함수/변수만 제공
 */
UCLASS()
class UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMainMenuWidget(const FObjectInitializer& ObjectInitializer);

protected:
	// 위젯이 화면에 생성될 때 호출됨
	virtual void NativeConstruct() override;

	/* ========================= 위젯 바인딩용 변수 ========================= */

	// 방 만들기 버튼 (블루프린트 디자이너에서 같은 이름의 버튼과 BindWidget)
	UPROPERTY(meta = (BindWidget))
	UButton* HostButton;

	// 세션 검색(새로고침) 버튼
	UPROPERTY(meta = (BindWidget))
	UButton* RefreshButton;

	// 선택된 세션에 참가 버튼
	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton;

	// 내 스팀 닉네임을 보여줄 텍스트
	UPROPERTY(meta = (BindWidget))
	UTextBlock* NicknameText;

	// 세션 리스트를 담을 패널 (VerticalBox, ScrollBox 등)
	UPROPERTY(meta = (BindWidget))
	UPanelWidget* SessionListPanel;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* StatusText;

	// 필터 버튼 2개
	UPROPERTY(meta = (BindWidget))
	UButton* FilterAllButton;

	UPROPERTY(meta = (BindWidget))
	UButton* FilterFriendsButton;

	/* ========================= 내부 상태 ========================= */

	// 세션 항목으로 사용할 위젯 클래스 (C++의 USessionEntryWidget 기반)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steam|Session", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USessionEntryWidget> SessionEntryClass;

	// 선택된 세션 인덱스 (세션 리스트 UI에서 설정해 줌)
	UPROPERTY(BlueprintReadOnly, Category = "Steam|Session", meta = (AllowPrivateAccess = "true"))
	int32 SelectedSessionIndex;

	// 편의를 위해 Subsystem 캐시 (필수는 아님)
	USteamSessionSubsystem* CachedSubsystem;

	/* ========================= 델리게이트용 함수 ========================= */

	// Host 버튼 클릭 시 호출
	UFUNCTION()
	void OnHostButtonClicked();

	// Refresh 버튼 클릭 시 호출
	UFUNCTION()
	void OnRefreshButtonClicked();

	// Join 버튼 클릭 시 호출
	UFUNCTION()
	void OnJoinButtonClicked();

	// Subsystem의 OnSessionListUpdated 델리게이트에 바인딩될 함수
	UFUNCTION()
	void HandleSessionListUpdated();

	// Subsystem의 OnSessionJoinFinished 델리게이트에 바인딩될 함수
	UFUNCTION()
	void HandleJoinFinished(int32 FinishedIndex, bool bSuccess);

	// SessionEntryWidget의 Join 클릭을 받는 함수 (StatusText 안내 및 버튼 잠금)
	UFUNCTION()
	void HandleEntryJoinClicked(int32 SessionIndex, const FString& OwnerName);

	// 실제로 패널(SessionListPanel)에 세션 항목들을 채우는 함수
	void RebuildSessionList();

	// 상태 메시지를 변경하는 헬퍼 함수
	void SetStatusMessage(const FString& Message);

	// Join 중 UI 잠금/해제 (버튼 비활성화 + 메시지)
	void SetJoinUIBusy(bool bBusy, const FString& Message);

	// 필터 버튼 핸들러
	UFUNCTION()
	void OnFilterAllClicked();

	UFUNCTION()
	void OnFilterFriendsClicked();

private:
	// Join 진행 상태 (메뉴 Join 버튼/Entry Join 버튼 모두 공통)
	bool bJoinInProgress = false;
	int32 PendingJoinIndex = -1;

public:
	/* ========================= 블루프린트에서 호출할 함수들 ========================= */

	// 현재 Subsystem(USteamSessionSubsystem) 를 반환 (캐스팅까지 수행)
	UFUNCTION(BlueprintPure, Category = "Steam|GI")
	USteamSessionSubsystem* GetSteamSessionSubsystem() const;

	// 세션 검색 결과 개수 반환 (세션 리스트 만들 때 사용)
	UFUNCTION(BlueprintPure, Category = "Steam|Session")
	int32 GetSessionResultCount() const;

	// 인덱스로 세션 호스트 이름 반환 (세션 리스트 UI에서 사용)
	UFUNCTION(BlueprintPure, Category = "Steam|Session")
	FString GetSessionOwnerNameBP(int32 Index) const;

	// 블루프린트에서 세션 리스트 버튼 클릭 시 이 함수를 호출해서 선택 인덱스 설정
	UFUNCTION(BlueprintCallable, Category = "Steam|Session")
	void SetSelectedSessionIndex(int32 Index);

	// 스팀 닉네임 텍스트를 갱신 (게임 시작 시 한 번, 필요하면 버튼으로 추가 호출 가능)
	UFUNCTION(BlueprintCallable, Category = "Steam|SteamInfo")
	void UpdateSteamNickname();
};
