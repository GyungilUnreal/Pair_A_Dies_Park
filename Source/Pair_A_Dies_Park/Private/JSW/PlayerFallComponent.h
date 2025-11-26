// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerFallComponent.generated.h"

UENUM(BlueprintType)
enum class EFallState : uint8
{
	Normal,
	Hanging,
	Falling,
	Climbing
};


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UPlayerFallComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UPlayerFallComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentState)
	EFallState CurrentState = EFallState::Normal;

	UFUNCTION()
	void OnRep_CurrentState();

	void ApplyStateLogic();

	UPROPERTY()
	class AFloorManager* FloorManager;

	UPROPERTY()
	class ACharacter* OwnerCharacter;
	// 낙하 감지
	void CheckFallingCondition();
	// 상태 변경
	void ChangeState(EFallState NewState);
	// 트레이스로 매달릴 벽 찾기
	bool CheckLedgeTrace(FVector& OutLedgeLoc, FRotator& OutLedgeRot, FHitResult& OutWallHit);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall|Animation")
	TObjectPtr<UAnimMontage> HangingMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall|Animation")
	TObjectPtr<UAnimMontage> ClimbUpMontage;

	// 기준 손 소켓
	UPROPERTY(EditAnywhere, Category = "Fall|Animation")
	FName HangingHandSocket = FName("Hand_r");

	// 위치 보정용 타이머 핸들
	FTimerHandle CorrectionTimerHandle;
	// 매달리기 시간용 타이머 핸들
	FTimerHandle HangTimerHandle;
	
	FTimerHandle ClimbUpTimerHandle;

	FVector TargetSafeLocation;
	FRotator TargetSafeRotation;

	void FinishClimbing();

	// 보정 함수
	void SnapActorToLedge(FVector LedgeLocation, FRotator LedgeRotation);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall|Settings")
	float MaxHangTime = 5.0f;

	void OnHangTimerExpired();

	UPROPERTY(EditAnywhere, Category = "Fall|Settings")
	float LedgeRegrabCooldown = 3.f;

	float LastDetachTime = -FLT_MAX;

	bool bCanGrabLedge = true;

public:
	// 외부에서 호출할 함수.
	UFUNCTION(BlueprintCallable, Category = "Fall|Interaction")
	void TryRescue(AActor* RescuerActor);

};