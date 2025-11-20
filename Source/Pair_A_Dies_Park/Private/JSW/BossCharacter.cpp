// Fill out your copyright notice in the Description page of Project Settings.


#include "JSW/BossCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"

ABossCharacter::ABossCharacter()
{
	// AI가 조종하도록 설정.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// 카메라 컴포넌트 제거.
	if (USpringArmComponent* Boom = GetCameraBoom())
	{
		Boom->DestroyComponent();
	}
	if (UCameraComponent* Camera = GetFollowCamera())
	{
		Camera->DestroyComponent();
	}
}

void ABossCharacter::ApplyPuzzleDamage(float DamageAmount)
{
	Server_TakePuzzleDamage(DamageAmount);
}

void ABossCharacter::BeginPlay()
{
    // InitializeAbilities() 호출.
    Super::BeginPlay(); 

    // 서버에서만 체력을 MaxHealth로 초기화.
    if (HasAuthority())
    {
        CurrentHealth = MaxHealth;

        // OnRep 함수를 수동으로 호출.
        OnRep_CurrentHealth();
    }
}

void ABossCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
}

void ABossCharacter::OnRep_CurrentHealth()
{
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void ABossCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABossCharacter, CurrentHealth);
    DOREPLIFETIME(ABossCharacter, MaxHealth);
}

void ABossCharacter::Server_TakePuzzleDamage_Implementation(float DamageAmount)
{
    // 이미 죽었거나, 데미지가 없으면 종료
    if (CurrentHealth <= 0.f || DamageAmount <= 0.f) return;

    CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.f, MaxHealth);

    // 서버 자신도 OnRep을 수동 호출해야 함
    OnRep_CurrentHealth();

    if (CurrentHealth <= 0.f)
    {
        // AI 컨트롤러 정지
        AAIController* AIC = Cast<AAIController>(GetController());
        if (AIC)
        {
            AIC->StopMovement();
            AIC->GetBrainComponent()->StopLogic(TEXT("Boss Died"));
        }
        // 죽는 몽타주 재생
        if (DeathMontage)
        {
            PlayAnimMontage(DeathMontage);
        }

        // 충돌 끄기
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GetCharacterMovement()->DisableMovement();

        // 몽타주 길이만큼 시간이 지난 후 액터 파괴
        SetLifeSpan(DeathMontage ? DeathMontage->GetPlayLength() : 5.0f);
    }
    // 페이즈 관련 코드. 일단 주석처리.
    //else
    //{
    //    // 2페이즈에 아직 진입 안 했고, 체력이 50% 이하가 되었는지 확인
    //    if (!bIsInPhase2 && (CurrentHealth / MaxHealth <= 0.5f))
    //    {
    //        bIsInPhase2 = true; // 2페이즈 진입 플래그 (중복 실행 방지)

    //        // 2페이즈 진입 로직 (예: 포효 몽타주, 새 스킬 BT에 추가)
    //        // PlayAnimMontage(PhaseChangeMontage);

    //        // (팁) 블랙보드에 bool 값을 세팅하여 BT가 새 패턴을 쓰도록 할 수 있습니다.
    //        // AAIController* AIC = Cast<AAIController>(GetController());
    //        // if (AIC && AIC->GetBlackboardComponent())
    //        // {
    //        //     AIC->GetBlackboardComponent()->SetValueAsBool(TEXT("IsInPhase2"), true);
    //        // }
    //    }
    //}
}
