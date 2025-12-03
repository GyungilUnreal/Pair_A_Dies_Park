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
	UPlayerFallComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	FVector InitialSpawnLocation;

	// State Logic
	UPROPERTY(ReplicatedUsing = OnRep_CurrentState)
	EFallState CurrentState = EFallState::Normal;

	UFUNCTION()
	void OnRep_CurrentState();
	void ApplyStateLogic();
	void ChangeState(EFallState NewState);
	void RespawnAtFloor1();

	// Ledge Logic
	void CheckFallingCondition();
	bool CheckLedgeTrace(FVector& OutLedgeLoc, FRotator& OutLedgeRot, FHitResult& OutWallHit);
	void StartHanging(const FHitResult& WallHit, const FVector& LedgeLoc);
	void SnapActorToLedge(FVector LedgeLocation, FRotator LedgeRotation);
	void PerformWallDrop();
	void OnHangTimerExpired();

	// Action Logic
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StartRescuerAction(ACharacter* RescuerChar, float Duration);
	void FinishRescuerAction(ACharacter* RescuerChar);
	void FinishClimbing();

	UFUNCTION(Server, Reliable)
	void Server_LetGo();

	UFUNCTION(Server, Reliable)
	void Server_ClimbUpSelf();

public:
	UFUNCTION(BlueprintCallable, Category = "Fall|System")
	void SetFallSystemEnabled(bool bEnabled);

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

protected:
	UPROPERTY()
	class AFloorManager* FloorManager;
	UPROPERTY()
	class ACharacter* OwnerCharacter;

	bool bFallEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall|Animation")
	TObjectPtr<UAnimMontage> HangingMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall|Animation")
	TObjectPtr<UAnimMontage> ClimbUpMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall|Animation")
	TObjectPtr<UAnimMontage> RescueMontage;

	UPROPERTY(EditAnywhere, Category = "Fall|Animation")
	FName HangingHandSocket = FName("Hand_r");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall|Settings")
	float MaxHangTime = 5.0f;
	UPROPERTY(EditAnywhere, Category = "Fall|Settings")
	float LedgeRegrabCooldown = 3.f;
	UPROPERTY(EditAnywhere, Category = "Fall|Settings")
	float GaugeIncreasePerPress = 15.0f;
	UPROPERTY(EditAnywhere, Category = "Fall|Settings")
	float GaugeDecayRate = 30.0f;

	FTimerHandle CorrectionTimerHandle;
	FTimerHandle HangTimerHandle;
	FTimerHandle ClimbUpTimerHandle;
	FVector TargetSafeLocation;
	FRotator TargetSafeRotation;
	float LastDetachTime = -FLT_MAX;
	bool bCanGrabLedge = true;
	float CurrentClimbGauge = 0.0f;
	const float MaxClimbGauge = 100.0f;
};