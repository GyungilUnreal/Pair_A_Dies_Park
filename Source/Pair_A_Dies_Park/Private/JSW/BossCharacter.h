// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Split_Character.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BossCharacter.generated.h"

// UI 알림용 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChangedSignature, float, NewHealth, float, MaxHealth);

UCLASS()
class ABossCharacter : public ASplit_Character
{
	GENERATED_BODY()
	
public:
	ABossCharacter();
	// 퍼즐 액션이 호출할 공개 함수. (서버, 클라)
	void ApplyPuzzleDamage(float DamageAmount);

	// UI가 바인딩할 보스 델리게이트 변수.
	UPROPERTY(BlueprintAssignable, Category = "Boss|Health")
	FOnHealthChangedSignature OnHealthChanged;

	UFUNCTION(BlueprintCallable, Category = "Boss")
	void WakeUpBoss();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BossDeath();
protected:
	// 플레이어 BeginPlay 로직 막기위함.
	virtual void BeginPlay() override;
	// 플레이어 인풋 막기위함.
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// 보스 체력 복제
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
	float CurrentHealth;
	
	// 보스 최대 체력 복제
	UPROPERTY(EditDefaultsOnly, Replicated, Category = "Boss|Health")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, Category = "Boss|ClearBP")
	TSubclassOf<AActor> ClearItemBPClass;

	// 체력이 변경될 때 클라에서 호출.
	UFUNCTION()
	void OnRep_CurrentHealth();

	// 체력 복제 설정.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// 기믹 데미지를 받는 함수. (서버)
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Boss")
	void Server_TakePuzzleDamage(float DamageAmount);

	// 보스 사망시 몽타주.
	UPROPERTY(EditAnywhere, Category = "Boss|Combat")
	TObjectPtr<UAnimMontage> DeathMontage;
	// 페이즈 로직용.
	//bool bIsInPhase2;

	bool bIsAwake = false;

	// 깨어날 때 재생할 몽타주 (포효)
	UPROPERTY(EditAnywhere, Category = "Boss")
	UAnimMontage* WakeUpMontage;
};
