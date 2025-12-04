#include "JSW/PlayerFallComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JSW/FloorManager.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

void UPlayerFallComponent::TryRescue(AActor* RescuerActor)
{
	if (CurrentState != EFallState::Hanging) return;
	if (!OwnerCharacter) return;

	FVector MyLoc = OwnerCharacter->GetActorLocation();
	FVector MyForward = OwnerCharacter->GetActorForwardVector();
	FVector MyUp = FVector::UpVector;
	TargetSafeLocation = MyLoc + (MyForward * 130.0f) + (MyUp * 190.0f);
	TargetSafeRotation = OwnerCharacter->GetActorRotation();

	ChangeState(EFallState::Climbing);

	ACharacter* RescuerChar = Cast<ACharacter>(RescuerActor);
	if (RescuerChar)
	{
		float VictimDuration = (ClimbUpMontage) ? ClimbUpMontage->GetPlayLength() : 1.5f;
		Multicast_StartRescuerAction(RescuerChar, VictimDuration);
	}
}

void UPlayerFallComponent::Multicast_StartRescuerAction_Implementation(ACharacter* RescuerChar, float Duration)
{
	if (!RescuerChar) return;

	RescuerChar->GetCharacterMovement()->DisableMovement();
	UAnimInstance* AnimInst = RescuerChar->GetMesh()->GetAnimInstance();
	if (AnimInst && RescueMontage)
	{
		RescuerChar->PlayAnimMontage(RescueMontage, 1.0f);
	}

	TWeakObjectPtr<ACharacter> WeakRescuer(RescuerChar);
	FTimerHandle RescuerFinishTimer;
	GetWorld()->GetTimerManager().SetTimer(RescuerFinishTimer, [this, WeakRescuer]()
	{
		if (WeakRescuer.IsValid()) FinishRescuerAction(WeakRescuer.Get());
	}, Duration, false);
}

void UPlayerFallComponent::FinishRescuerAction(ACharacter* RescuerChar)
{
	if (!RescuerChar) return;
	UAnimInstance* AnimInst = RescuerChar->GetMesh()->GetAnimInstance();

	if (AnimInst && RescueMontage && AnimInst->Montage_IsPlaying(RescueMontage))
	{
		AnimInst->Montage_SetPlayRate(RescueMontage, -1.5f);
		if (AnimInst->Montage_GetIsStopped(RescueMontage))
		{
			RescuerChar->PlayAnimMontage(RescueMontage, -1.5f, NAME_None);
			AnimInst->Montage_SetPosition(RescueMontage, RescueMontage->GetPlayLength());
		}
	}
	RescuerChar->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void UPlayerFallComponent::FinishClimbing()
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority()) return;
	OwnerCharacter->SetActorLocationAndRotation(TargetSafeLocation, TargetSafeRotation);
	ChangeState(EFallState::Normal);
}

void UPlayerFallComponent::Input_MashF()
{
	if (CurrentState != EFallState::Hanging) return;

	float MyZ = OwnerCharacter->GetActorLocation().Z;
	float Floor2_Threshold = 46000.0f;

	if (FloorManager)
	{
		Floor2_Threshold = (FloorManager->Floor1_Height + FloorManager->Floor2_Height) * 0.5f;
	}
	// 1층이라면 F키 입력 무시
	if (MyZ < Floor2_Threshold)	return;

	// 게이지 증가
	CurrentClimbGauge = FMath::Clamp(CurrentClimbGauge + GaugeIncreasePerPress, 0.0f, MaxClimbGauge);

	// UI 갱신 알림
	OnClimbGaugeChanged.Broadcast(CurrentClimbGauge / MaxClimbGauge);

	UE_LOG(LogTemp, Log, TEXT("Climb Gauge: %f"), CurrentClimbGauge);

	// 게이지 꽉 찼는지 확인
	if (CurrentClimbGauge >= MaxClimbGauge)
	{
		// 자력 등반 실행
		ClimbUpSelf();

		// 게이지 초기화
		CurrentClimbGauge = 0.0f;
		OnClimbGaugeChanged.Broadcast(0.0f);
	}
}

float UPlayerFallComponent::GetRemainingHangTime() const
{
	if (CurrentState != EFallState::Hanging) return 0.0f;

	if (GetWorld())
	{
		return GetWorld()->GetTimerManager().GetTimerRemaining(HangTimerHandle);
	}
	return 0.0f;
}

void UPlayerFallComponent::LetGo()
{
	if (CurrentState != EFallState::Hanging) return;
	if (!OwnerCharacter) return;
	if (OwnerCharacter->HasAuthority()) PerformWallDrop();
	else Server_LetGo();
}

void UPlayerFallComponent::Server_LetGo_Implementation()
{
	PerformWallDrop();
}

void UPlayerFallComponent::ClimbUpSelf()
{
	Server_ClimbUpSelf();
}

void UPlayerFallComponent::Server_ClimbUpSelf_Implementation()
{
	TryRescue(nullptr);
}