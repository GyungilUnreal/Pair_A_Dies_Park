// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JSW/GA/GA_BossAttack.h"
#include "GA_BossPunch1.generated.h"


UCLASS()
class UGA_BossPunch1 : public UGA_BossAttack
{
	GENERATED_BODY()
	
public:
	UGA_BossPunch1();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 몽타주 재생이 완료되거나 중단시
	UFUNCTION()
	void OnMontageCompleted();
	// 타이머가 주기적으로 호출할 판정 함수
	UFUNCTION()
	void PerformOverlapCheck();


	// 공격이 발생할 위치
	UPROPERTY(EditAnywhere, Category = "Punch")
	float PunchLength = 500.f;
	// 공격을 발생시킬 소켓이름
	UPROPERTY(EditAnywhere, Category = "Punch")
	FName SocketName = FName("R_HandAttack");
	// 수평으로 밀치는 힘
	UPROPERTY(EditAnywhere, Category = "Punch")
	float HorizontalKnockback = 1500.f;
	// 위로 띄우는 힘
	UPROPERTY(EditAnywhere, Category = "Punch")
	float UpwardKnockback = 500.f;
	// 박스 크기
	UPROPERTY(EditAnywhere, Category = "Punch")
	FVector HalfSize = FVector(300.f, 300.f, 300.f);


	// 판정을 위한 타이머 함수
	FTimerHandle OverlapTimerHandle;

	// 이전 프레임의 소켓 위치를 저장하는 변수.
	FVector PreciousSocketLocation;


};
