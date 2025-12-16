#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SessionSubsystem.generated.h"

class USteamSessionSubsystem;

// 세션 생성 결과를 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSessionCreateComplete, bool, bWasSuccessful);
// 세션 검색 결과를 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSessionFindComplete, bool, bWasSuccessful);
// 세션 참가 결과를 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSessionJoinComplete, bool, bWasSuccessful);
// 세션 삭제 결과를 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSessionDestroyComplete, bool, bWasSuccessful);

/**
 * (레거시) 온라인 세션을 생성, 검색, 참가, 삭제하는 GameInstanceSubsystem
 * - 이제 모든 실제 로직은 USteamSessionSubsystem에서 관리
 * - 이 클래스는 단순 포워더(호환 레이어)
 */
UCLASS()
class USessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	USessionSubsystem();

	// Subsystem 초기화
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 세션 생성(=Host)
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

	// 델리게이트들(레거시)
	UPROPERTY(BlueprintAssignable, Category = "Session")
	FSessionCreateComplete OnCreateSessionCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FSessionFindComplete OnFindSessionsCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FSessionJoinComplete OnJoinSessionCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FSessionDestroyComplete OnDestroySessionCompleteEvent;

private:
	// 실제 구현 서브시스템
	TWeakObjectPtr<USteamSessionSubsystem> SteamSubsystem;

	USteamSessionSubsystem* ResolveSteamSubsystem() const;

	// SteamSubsystem 델리게이트 브리지
	UFUNCTION()
	void HandleHostComplete(bool bWasSuccessful);

	UFUNCTION()
	void HandleFindComplete(bool bWasSuccessful);

	UFUNCTION()
	void HandleJoinComplete(bool bWasSuccessful);

	UFUNCTION()
	void HandleDestroyComplete(bool bWasSuccessful);
};