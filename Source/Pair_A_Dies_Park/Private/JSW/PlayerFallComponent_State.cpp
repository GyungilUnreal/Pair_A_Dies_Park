#include "JSW/PlayerFallComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JSW/FloorManager.h"
#include "JSW/RescueInteractableComponent.h"
#include "TimerManager.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

void UPlayerFallComponent::OnRep_CurrentState()
{
	ApplyStateLogic();
}

void UPlayerFallComponent::ChangeState(EFallState NewState)
{
	if (CurrentState == NewState) return;
	CurrentState = NewState;
	ApplyStateLogic();
}

void UPlayerFallComponent::ApplyStateLogic()
{
	GetWorld()->GetTimerManager().ClearTimer(HangTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(ClimbUpTimerHandle);

	if (!OwnerCharacter) return;
	UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement();
	URescueInteractableComponent* RescueComp = OwnerCharacter->FindComponentByClass<URescueInteractableComponent>();

	CurrentClimbGauge = 0.0f;
	OnClimbGaugeChanged.Broadcast(0.0f);

	switch (CurrentState)
	{
	case EFallState::Normal:
		OwnerCharacter->SetActorEnableCollision(true);
		if (Movement) Movement->SetMovementMode(MOVE_Walking);
		if (OwnerCharacter->GetMesh()->GetAnimInstance()->Montage_IsPlaying(HangingMontage))
		{
			OwnerCharacter->StopAnimMontage(HangingMontage);
		}
		if (RescueComp) RescueComp->SetIsInteractable(false);
		break;

	case EFallState::Hanging:
		if (Movement)
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_Flying);
			Movement->DisableMovement();
		}
		if (HangingMontage) OwnerCharacter->PlayAnimMontage(HangingMontage);
		if (RescueComp) RescueComp->SetIsInteractable(true);

		if (OwnerCharacter->HasAuthority())
		{
			GetWorld()->GetTimerManager().SetTimer(HangTimerHandle, this, &UPlayerFallComponent::OnHangTimerExpired, MaxHangTime, false);
		}
		break;

	case EFallState::Climbing:
	{
		if (Movement)
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_Flying);
		}

		float Duration = 1.5f;
		if (ClimbUpMontage) Duration = OwnerCharacter->PlayAnimMontage(ClimbUpMontage);
		if (RescueComp) RescueComp->SetIsInteractable(false);

		if (OwnerCharacter->HasAuthority())
		{
			float WaitTime = FMath::Max(0.1f, Duration - 0.2f);
			GetWorld()->GetTimerManager().SetTimer(ClimbUpTimerHandle, this, &UPlayerFallComponent::FinishClimbing, WaitTime, false);
		}
		break;
	}

	case EFallState::Falling:
		if (Movement) Movement->SetMovementMode(MOVE_Falling);
		OwnerCharacter->SetActorEnableCollision(true);
		break;
	}
}

void UPlayerFallComponent::RespawnAtFloor1()
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority()) return;

	FVector RespawnLoc;

	if (FloorManager)
	{
		if (MinKillZThreshold > 0.0f)
		{
			RespawnLoc = FloorManager->GetRandomSafeFloorLocation();
			FloorManager->ModifyTeamLife(1);
		}
		else
		{
			RespawnLoc = InitialSpawnLocation;
			FloorManager->IncrementFallCount();
		}
	}
	else
	{
		RespawnLoc = InitialSpawnLocation;
		UE_LOG(LogTemp, Warning, TEXT("No FloorManager found. Respawning at Initial Location."));
	}
	OwnerCharacter->GetCharacterMovement()->Velocity = FVector::ZeroVector;
	OwnerCharacter->SetActorLocation(RespawnLoc);
	ChangeState(EFallState::Normal);
}