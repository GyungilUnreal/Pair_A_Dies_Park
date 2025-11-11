#include "GazeInteractableComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "GazeInteractorComponent.h"

UGazeInteractableComponent::UGazeInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGazeInteractableComponent::NotifyGazeInteracted(AActor* InstigatorActor, int32 SetIndex)
{
	// 보통 서버에서만 부르도록
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Multicast_NotifyGazeInteracted(InstigatorActor, SetIndex);
	}
	else
	{
		// 필요하면 여기서 Server RPC 추가해서 서버로 보내게 만들 수도 있음
	}
}

void UGazeInteractableComponent::Multicast_NotifyGazeInteracted_Implementation(AActor* InstigatorActor, int32 SetIndex)
{
	// 1) BP용 이벤트
	OnGazeInteracted(InstigatorActor, SetIndex);

	// 2) 이 클라에 있는 모든 플레이어(1P, 2P ...)의 GazeInteractorComponent를 훑어서
	//    "내가 지금 보고 있는 게 이 액터였으면 위젯 내려"라고 시킨다.
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

		// GazeInteractorComponent에 아래에서 만드는 헬퍼를 호출
		GazeComp->ClearIfCurrentTarget(OwnerActor, SetIndex);
		GazeComp->ClearCandidateForTarget(OwnerActor);
	}
}
