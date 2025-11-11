#include "GazeTextTargetComponent.h"
#include "GazeInteractableComponent.h"
#include "GazeInteractorComponent.h"

#include "Net/UnrealNetwork.h"

UGazeTextTargetComponent::UGazeTextTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// 기본 문구를 하나 넣어둘 수도 있음
	DisplayText = FText::FromString(TEXT("Interact"));
	TextWidgetName = FName(TEXT("Interactable"));
}

void UGazeTextTargetComponent::SetDisplayTextAndNotify(const FText& NewText)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
		return;

	if (OwnerActor->HasAuthority())
	{
		// 서버일 때만 진짜 값 바꾸기 + 멀티캐스트 (기존과 동일)
		DisplayText = NewText;

		if (UGazeInteractableComponent* Interactable = OwnerActor->FindComponentByClass<UGazeInteractableComponent>())
		{
			Interactable->Multicast_RefreshGazeText();
		}
	}
	else
	{
		// 클라일 때는 로컬 위젯만 임시로 바꿔서 "바로 바뀐 것처럼" 보이게
		DisplayText = NewText;

		// 여기서 굳이 Server_RefreshGazeText()를 안 불러도 됨.
		// 그건 소유권 문제로 실패 가능성이 있기 때문.
		// 대신 나중에 서버가 진짜 값을 바꾸면 그게 Replicate돼서 OnRep_DisplayText()가 다시 한번 정확히 갱신해줄 거임.
	}
}

void UGazeTextTargetComponent::OnRep_DisplayText()
{
	// 여기서도 GazeInteractableComponent를 찾아서,
	// 지금 이 액터를 보고 있는 클라이언트 위젯들을 다시 적용시킨다.
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
		return;

	if (UGazeInteractableComponent* Interactable = OwnerActor->FindComponentByClass<UGazeInteractableComponent>())
	{
		// 서버가 아니어도 월드의 플레이어들을 돌 수 있으니
		// Multicast 말고 로컬에서 직접 갱신 호출
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
						Gaze->RefreshTextForTargetActor(OwnerActor);
					}
				}
			}
		}
	}
}

void UGazeTextTargetComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGazeTextTargetComponent, DisplayText);
}