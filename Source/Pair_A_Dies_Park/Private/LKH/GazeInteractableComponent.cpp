#include "GazeInteractableComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "GazeInteractorComponent.h"
#include "Net/UnrealNetwork.h"

UGazeInteractableComponent::UGazeInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

	bIsInteractable = true;
}

void UGazeInteractableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGazeInteractableComponent, bIsInteractable);
}

void UGazeInteractableComponent::NotifyGazeInteracted(AActor* InstigatorActor, int32 SetIndex)
{
	if (AActor* Owner = GetOwner())
	{
		if (Owner->HasAuthority())
		{
			Multicast_NotifyGazeInteracted(InstigatorActor, SetIndex);
		}
		else
		{
			// 클라에서 호출된 경우면 여기서 서버 RPC 날리는 걸로 확장 가능
		}
	}
}

void UGazeInteractableComponent::Multicast_NotifyGazeInteracted_Implementation(AActor* InstigatorActor, int32 SetIndex)
{
	// 1) BP에서 바인딩된 델리게이트 호출
	OnGazeInteracted.Broadcast(InstigatorActor, SetIndex);

	// 2) 각 클라의 GazeInteractorComponent 갱신
	UWorld* World = GetWorld();
	if (!World)
		return;

	AActor* OwnerActor = GetOwner();

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
			continue;

		APawn* Pawn = PC->GetPawn();
		if (!Pawn)
			continue;

		UGazeInteractorComponent* GazeComp = Pawn->FindComponentByClass<UGazeInteractorComponent>();
		if (!GazeComp)
			continue;
	}
}

void UGazeInteractableComponent::Server_RefreshGazeText_Implementation()
{
	// 서버에서만 실행 → 전파
	Multicast_RefreshGazeText();
}

void UGazeInteractableComponent::Multicast_RefreshGazeText_Implementation()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	AActor* OwnerActor = GetOwner();

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
			continue;

		APawn* Pawn = PC->GetPawn();
		if (!Pawn)
			continue;

		if (UGazeInteractorComponent* GazeComp = Pawn->FindComponentByClass<UGazeInteractorComponent>())
		{
			// 우리가 앞에서 만들었던 “이 액터 위젯이면 텍스트 다시 넣어” 헬퍼를 여기서 부른다고 가정
			GazeComp->RefreshTextForTargetActor(OwnerActor);
		}
	}
}

void UGazeInteractableComponent::SetIsInteractable(bool bNew)
{
	bIsInteractable = bNew;
	HandleInteractableChanged();
}

void UGazeInteractableComponent::OnRep_IsInteractable()
{
	HandleInteractableChanged();
}

void UGazeInteractableComponent::HandleInteractableChanged()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
		return;

	UWorld* World = OwnerActor->GetWorld();
	if (!World)
		return;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (UGazeInteractorComponent* Gaze = Pawn->FindComponentByClass<UGazeInteractorComponent>())
				{
					Gaze->OnTargetInteractableStateChanged(OwnerActor, bIsInteractable);
				}
			}
		}
	}
}