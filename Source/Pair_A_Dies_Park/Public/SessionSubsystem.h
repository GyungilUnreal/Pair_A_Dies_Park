#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "SessionSubsystem.generated.h"

// 세션 생성 결과를 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSessionCreateComplete, bool, bWasSuccessful);
// 세션 검색 결과를 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSessionFindComplete, bool, bWasSuccessful);
// 세션 참가 결과를 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSessionJoinComplete, bool, bWasSuccessful);
// 세션 삭제 결과를 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSessionDestroyComplete, bool, bWasSuccessful);

/**
 * 온라인 세션을 생성, 검색, 참가, 삭제하는 GameInstanceSubsystem
 */
UCLASS(Config = Game)
class USessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	USessionSubsystem();

	// Subsystem 초기화
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 세션 생성
	UFUNCTION(BlueprintCallable, Category = "Session")
	void CreateSession(int32 NumPublicConnections, bool bIsLANMatch, FString ServerName);

	// 세션 검색
	UFUNCTION(BlueprintCallable, Category = "Session")
	void FindSessions(int32 MaxSearchResults, bool bIsLANMatch);

	// 세션 참가
	UFUNCTION(BlueprintCallable, Category = "Session")
	void JoinSession(int32 SessionIndex);

	// 세션 삭제
	UFUNCTION(BlueprintCallable, Category = "Session")
	void DestroySession();

	// 검색된 세션 결과 가져오기
	UFUNCTION(BlueprintCallable, Category = "Session")
	TArray<FString> GetSessionSearchResults();

	// 델리게이트들
	UPROPERTY(BlueprintAssignable, Category = "Session")
	FSessionCreateComplete OnCreateSessionCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FSessionFindComplete OnFindSessionsCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FSessionJoinComplete OnJoinSessionCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FSessionDestroyComplete OnDestroySessionCompleteEvent;

protected:
	// 세션 인터페이스 콜백 함수들
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

private:
	// 온라인 세션 인터페이스
	IOnlineSessionPtr SessionInterface;

	// 세션 검색 결과
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	// 델리게이트 핸들들
	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle DestroySessionCompleteHandle;

	// 마지막 세션 설정 저장 (재시도용)
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;

	UPROPERTY(Config)
	bool bEnableLegacySessionSubsystem = false;

	FORCEINLINE bool IsLegacyEnabled() const { return bEnableLegacySessionSubsystem; }
};