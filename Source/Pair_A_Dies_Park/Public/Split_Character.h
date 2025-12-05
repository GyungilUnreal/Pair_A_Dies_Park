// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Split_Character.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class ABaseWeapon;
struct FInputActionValue;

UCLASS(config=Game)
class ASplit_Character : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

public:
	ASplit_Character();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EquipWeapon(ABaseWeapon* NewWeapon);

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Anim")
	float GetAO_Pitch();

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	// 공격 핸들러
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Input_Attack_Start();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Input_Attack_End();

	// 핸들러 함수
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Input_Aim_Start(); // 누름

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Input_Aim_Stop();  // 뗌

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

	// Ability System Component (Pawn이 직접 소유)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComp;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	ABaseWeapon* EquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_IsAiming, VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsAiming = false;

	UFUNCTION(Server, Reliable)
	void Server_SetAiming(bool bNewState);

	UFUNCTION()
	void OnRep_IsAiming();

	void UpdateAimingState();

	// Pawn이 사용할 기본 Ability 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	// Pawn이 시작 시 Ability를 부여받았는지 여부
	bool bAbilitiesGranted = false;

	// Ability 초기화 함수
	void InitializeAbilities();

	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_PlayerState() override;

	// 좌클릭 공격 입력 액션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AttackAction;
	// 조준 입력 액션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AimAction;

	// 줌 관련 설정
	float DefaultFOV = 90.0f; // 평소 시야각
	float AimFOV = 60.0f;     // 줌 했을 때 시야각
	float ZoomInterpSpeed = 15.0f; // 줌 속도
	
public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float MouseSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bInvertY = false;
};

