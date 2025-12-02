// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerFallComponent.generated.h"

class UAnimMontage;
class AFloorManager;
class ACharacter;

UENUM(BlueprintType)
enum class EFallState : uint8
{
	Normal,
	Hanging,
	Falling,
	Climbing
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClimbGaugeChanged, float, GaugeRatio);

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

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StartRescuerAction(ACharacter* RescuerChar, float Duration);

	void FinishRescuerAction(ACharacter* RescuerChar);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall|Animation")
	TObjectPtr<UAnimMontage> RescueMontage;

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

	void PerformWallDrop();

	void RespawnAtFloor1();
	
	void StartHanging(const FHitResult& WallHit, const FVector& LedgeLoc);

	UPROPERTY(EditAnywhere, Category = "Fall|Settings")
	float LedgeRegrabCooldown = 3.f;

	float LastDetachTime = -FLT_MAX;

	bool bCanGrabLedge = true;

	float CurrentClimbGauge = 0.0f;
	const float MaxClimbGauge = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Fall|Settings")
	float GaugeIncreasePerPress = 15.0f; // 한 번 누를 때 차는 양

	UPROPERTY(EditAnywhere, Category = "Fall|Settings")
	float GaugeDecayRate = 30.0f; // 초당 줄어드는 양 (안 누르면 떨어짐

	UFUNCTION(Server,Reliable)
	void Server_LetGo();

	UFUNCTION(Server,Reliable)
	void Server_ClimbUpSelf();

public:
	// 외부에서 호출할 함수.
	UFUNCTION(BlueprintCallable, Category = "Fall|Interaction")
	void TryRescue(AActor* RescuerActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Fall|Action")
	void ClimbUpSelf();

	UFUNCTION(BlueprintCallable, Category = "Fall|Action")
	void LetGo();

	UFUNCTION(BlueprintCallable, Category = "Fall|Action")
	void Input_MashF();

	UPROPERTY(BlueprintAssignable, Category = "Fall|UI")
	FOnClimbGaugeChanged OnClimbGaugeChanged;
};