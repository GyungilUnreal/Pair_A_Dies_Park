#include "GazeInteractorComponent.h"
#include "GazeInteractableInterface.h"
#include "GazeTextTargetComponent.h"
#include "GazeInteractableComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

UGazeInteractorComponent::UGazeInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	TraceDistance = 800.f;
	GazeSweepSteps = 8;
	TraceOffset = 30.f;
	ComponentDetectRadius = 40.f;
	PlayerOverlapRadius = 200.f;

	TraceChannel = ECC_Visibility;

	CurrentTargetActor = nullptr;
	CurrentSetIndex = INDEX_NONE;
	CurrentWidgetComp = nullptr;

	bDrawDebugLine = false;
	bOnlyLocal = true;
}

void UGazeInteractorComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UGazeInteractorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HideCurrentWidget();

	// 후보 위젯 정리
	for (TPair<TWeakObjectPtr<AActor>, UWidgetComponent*>& Pair : CandidateWidgetMap)
	{
		if (Pair.Value)
		{
			Pair.Value->DestroyComponent();
		}
	}
	CandidateWidgetMap.Empty();

	Super::EndPlay(EndPlayReason);
}

void UGazeInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bOnlyLocal && !IsOwnerLocal())
	{
		return;
	}

	PerformGazeTrace();
}

bool UGazeInteractorComponent::IsOwnerLocal() const
{
	const APawn* PawnOwner = Cast<APawn>(GetOwner());
	if (!PawnOwner) return true;

	const APlayerController* PC = Cast<APlayerController>(PawnOwner->GetController());
	return (PC && PC->IsLocalController());
}

void UGazeInteractorComponent::PerformGazeTrace()
{
	AActor* Owner = GetOwner();
	if (!Owner)
		return;

	UCameraComponent* CameraComp = Owner->FindComponentByClass<UCameraComponent>();
	if (!CameraComp)
		return;

	const FVector CameraLoc = CameraComp->GetComponentLocation();
	const FVector Forward = CameraComp->GetComponentRotation().Vector();
	const FVector Start = CameraLoc + Forward * TraceOffset;
	const FVector End = Start + Forward * TraceDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(Gaze), false, Owner);

	if (bDrawDebugLine)
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Yellow, false, 0.f, 0, 1.5f);
	}

	// 1단계/2단계에서 모은 후보들
	TArray<FGazeCandidate> AllCandidates;
	FGazeCandidate BestCandidate;
	BestCandidate.Actor = nullptr;
	BestCandidate.SetIndex = INDEX_NONE;
	BestCandidate.DistSqFromView = TNumericLimits<float>::Max();

	// 1) 시선 따라 여러 구간 스윕
	CollectCandidatesAlongGaze(Start, Forward, AllCandidates, BestCandidate, Params);

	// 2) 주변 범위 후보 수집
	CollectCandidatesAroundPlayer(Owner->GetActorLocation(), AllCandidates, Params);

	// 최종 적용
	if (BestCandidate.Actor.IsValid() && BestCandidate.SetIndex != INDEX_NONE)
	{
		// 타깃 위젯 갱신
		if (BestCandidate.Actor.Get() != CurrentTargetActor || BestCandidate.SetIndex != CurrentSetIndex)
		{
			ShowWidgetForSet(BestCandidate.Actor.Get(), BestCandidate.SetIndex);
		}
		CurrentTargetActor = BestCandidate.Actor.Get();
		CurrentSetIndex = BestCandidate.SetIndex;
	}
	else
	{
		// 타깃이 없다
		HideCurrentWidget();
		CurrentTargetActor = nullptr;
		CurrentSetIndex = INDEX_NONE;
	}

	// 후보 UI 갱신 (타깃 제외하고)
	UpdateCandidateWidgets(AllCandidates, BestCandidate);
}

void UGazeInteractorComponent::CollectCandidatesAlongGaze(const FVector& Start, const FVector& Forward, TArray<FGazeCandidate>& InOutCandidates, FGazeCandidate& InOutBest, const FCollisionQueryParams& Params)
{
	const float StepSize = TraceDistance / FMath::Max(1, GazeSweepSteps);

	for (int32 i = 1; i <= GazeSweepSteps; ++i)
	{
		const FVector Center = Start + Forward * (StepSize * i);

		TArray<FHitResult> Hits;
		const bool bHit = GetWorld()->SweepMultiByChannel(
			Hits,
			Center,
			Center,
			FQuat::Identity,
			TraceChannel,
			FCollisionShape::MakeSphere(ComponentDetectRadius),
			Params
		);

#if WITH_EDITOR
		if (bDrawDebugLine)
		{
			DrawDebugSphere(GetWorld(), Center, ComponentDetectRadius, 8, bHit ? FColor::Green : FColor::Red, false, 0.f);
		}
#endif

		if (!bHit)
		{
			continue;
		}

		for (const FHitResult& H : Hits)
		{
			AActor* HitActor = H.GetActor();
			if (!HitActor) continue;

			UActorComponent* MatchedComp = nullptr;
			int32 SetIndex = INDEX_NONE;
			if (!FindMatchedComponentInActor(HitActor, MatchedComp, SetIndex))
			{
				continue;
			}

			// 중복 후보 체크
			bool bAlreadyAdded = false;
			for (const FGazeCandidate& C : InOutCandidates)
			{
				if (C.Actor.Get() == HitActor)
				{
					bAlreadyAdded = true;
					break;
				}
			}
			if (bAlreadyAdded)
				continue;

			FGazeCandidate NewCand;
			NewCand.Actor = HitActor;
			NewCand.SetIndex = SetIndex;

			FVector HitPos;
			if (H.ImpactPoint.IsNearlyZero())
			{
				HitPos = HitActor->GetActorLocation();
			}
			else
			{
				HitPos = FVector(H.ImpactPoint);
			}
			NewCand.DistSqFromView = FVector::DistSquared(Start, HitPos);

			InOutCandidates.Add(NewCand);

			// 베스트 갱신 (시선에 가까운 놈 우선)
			if (NewCand.DistSqFromView < InOutBest.DistSqFromView)
			{
				InOutBest = NewCand;
			}
		}
	}
}

void UGazeInteractorComponent::CollectCandidatesAroundPlayer(
	const FVector& Origin,
	TArray<FGazeCandidate>& InOutCandidates,
	const FCollisionQueryParams& Params)
{
	TArray<FOverlapResult> Overlaps;

	bool bAny = GetWorld()->OverlapMultiByChannel(
		Overlaps,
		Origin,
		FQuat::Identity,
		TraceChannel,
		FCollisionShape::MakeSphere(PlayerOverlapRadius),
		Params
	);

#if WITH_EDITOR
	if (bDrawDebugLine)
	{
		DrawDebugSphere(GetWorld(), Origin, PlayerOverlapRadius, 12, bAny ? FColor::Cyan : FColor::Blue, false, 0.f);
	}
#endif

	if (!bAny)
		return;

	for (const FOverlapResult& O : Overlaps)
	{
		AActor* HitActor = O.GetActor();
		if (!HitActor) continue;

		UActorComponent* MatchedComp = nullptr;
		int32 SetIndex = INDEX_NONE;
		if (!FindMatchedComponentInActor(HitActor, MatchedComp, SetIndex))
		{
			continue;
		}

		// 이미 후보에 있으면 스킵
		bool bAlreadyAdded = false;
		for (const FGazeCandidate& C : InOutCandidates)
		{
			if (C.Actor.Get() == HitActor)
			{
				bAlreadyAdded = true;
				break;
			}
		}
		if (bAlreadyAdded)
			continue;

		FGazeCandidate NewCand;
		NewCand.Actor = HitActor;
		NewCand.SetIndex = SetIndex;
		NewCand.DistSqFromView = FVector::DistSquared(Origin, HitActor->GetActorLocation());

		InOutCandidates.Add(NewCand);

		// 여기서는 BestCandidate(인자 자체가 없음)를 절대 갱신하지 않는다
	}
}

bool UGazeInteractorComponent::FindMatchedComponentInActor(AActor* Actor, UActorComponent*& OutMatchedComp, int32& OutSetIndex) const
{
	OutMatchedComp = nullptr;
	OutSetIndex = INDEX_NONE;

	if (!Actor)
		return false;

	// 인터랙트 가능 여부 확인
	if (UGazeInteractableComponent* InteractableComp = Actor->FindComponentByClass<UGazeInteractableComponent>())
	{
		if (!InteractableComp->GetIsInteractable())
		{
			return false; // 시선에 안 잡히게 함
		}
	}

	for (int32 i = 0; i < DetectSets.Num(); ++i)
	{
		const FGazeDetectSet& Set = DetectSets[i];
		if (!*Set.TargetComponentClass)
			continue;

		UActorComponent* Found = Actor->FindComponentByClass(Set.TargetComponentClass);
		if (Found)
		{
			OutMatchedComp = Found;
			OutSetIndex = i;
			return true;
		}
	}

	return false;
}

void UGazeInteractorComponent::ShowWidgetForSet(AActor* TargetActor, int32 SetIndex)
{
	// 추가: 이 Actor가 이전 프레임까지 후보였다면 후보 위젯 정리
	if (UWidgetComponent** FoundCand = CandidateWidgetMap.Find(TargetActor))
	{
		if (UWidgetComponent* CandWidget = *FoundCand)
		{
			CandWidget->DestroyComponent();
		}
		CandidateWidgetMap.Remove(TargetActor);
	}

	// 기존 코드
	HideCurrentWidget();

	if (!TargetActor || !DetectSets.IsValidIndex(SetIndex))
		return;

	const FGazeDetectSet& Set = DetectSets[SetIndex];
	if (!*Set.WidgetClass)
		return;

	CurrentWidgetComp = SpawnWidgetOnActor(TargetActor, Set.WidgetClass, Set.MainWidgetOffset);
	ApplyGazeTextIfAny(TargetActor, CurrentWidgetComp);
}

void UGazeInteractorComponent::HideCurrentWidget()
{
	if (CurrentWidgetComp)
	{
		CurrentWidgetComp->DestroyComponent();
		CurrentWidgetComp = nullptr;
	}
}

UWidgetComponent* UGazeInteractorComponent::SpawnWidgetOnActor(
	AActor* TargetActor,
	TSubclassOf<UUserWidget> WidgetClass,
	const FVector& InOffset
) const
{
	if (!TargetActor || !*WidgetClass)
		return nullptr;

	UWidgetComponent* WidgetComp = NewObject<UWidgetComponent>(TargetActor);
	if (!WidgetComp)
		return nullptr;

	WidgetComp->RegisterComponent();
	WidgetComp->SetWidgetClass(WidgetClass);
	WidgetComp->SetDrawAtDesiredSize(true);
	WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComp->AttachToComponent(TargetActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	WidgetComp->SetRelativeLocation(InOffset);

	// 여기부터: 이 클라이언트의 로컬플레이어들 중에서
	//   "1P pawn 있으면 1P", 아니면 "2P pawn 있으면 2P" 고르기
	UWorld* World = GetWorld();
	if (World)
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			const TArray<ULocalPlayer*>& LocalPlayers = GI->GetLocalPlayers();
			ULocalPlayer* ChosenLocalPlayer = nullptr;

			// 1P 우선
			if (LocalPlayers.Num() >= 1)
			{
				if (APlayerController* PC1 = LocalPlayers[0]->GetPlayerController(World))
				{
					if (APawn* P1 = PC1->GetPawn())
					{
						ChosenLocalPlayer = LocalPlayers[0];
					}
				}
			}

			// 1P가 없고 2P가 있으면 2P
			if (!ChosenLocalPlayer && LocalPlayers.Num() >= 2)
			{
				if (APlayerController* PC2 = LocalPlayers[1]->GetPlayerController(World))
				{
					if (APawn* P2 = PC2->GetPawn())
					{
						ChosenLocalPlayer = LocalPlayers[1];
					}
				}
			}

			if (ChosenLocalPlayer)
			{
				WidgetComp->SetOwnerPlayer(ChosenLocalPlayer);
			}
		}
	}

	return WidgetComp;
}

void UGazeInteractorComponent::UpdateCandidateWidgets(const TArray<FGazeCandidate>& Candidates, const FGazeCandidate& FinalTarget)
{
	// 추가: 최종 타깃 액터 Raw 포인터로 뽑아 두기
	AActor* FinalTargetActor = FinalTarget.Actor.Get();

	TSet<TWeakObjectPtr<AActor>> ThisFrameActors;

	for (const FGazeCandidate& C : Candidates)
	{
		AActor* Actor = C.Actor.Get();
		if (!Actor)
			continue;

		// 수정: WeakPtr 비교 대신 Raw 포인터 비교
		if (FinalTargetActor && Actor == FinalTargetActor)
		{
			// 최종 타깃은 후보 위젯 만들지 않는다
			continue;
		}

		ThisFrameActors.Add(C.Actor);

		if (!DetectSets.IsValidIndex(C.SetIndex))
			continue;

		const FGazeDetectSet& Set = DetectSets[C.SetIndex];
		if (!*Set.CandidateWidgetClass)
			continue;

		if (UWidgetComponent** FoundPtr = CandidateWidgetMap.Find(Actor))
		{
			if (UWidgetComponent* FoundWidget = *FoundPtr)
			{
				FoundWidget->SetRelativeLocation(Set.CandidateWidgetOffset);
				ApplyGazeTextIfAny(Actor, FoundWidget);
			}
		}
		else
		{
			UWidgetComponent* NewWidget = SpawnWidgetOnActor(Actor, Set.CandidateWidgetClass, Set.CandidateWidgetOffset);
			if (NewWidget)
			{
				CandidateWidgetMap.Add(Actor, NewWidget);
				ApplyGazeTextIfAny(Actor, NewWidget);
			}
		}
	}

	// 이번 프레임에 없어진 후보들은 정리
	TArray<TWeakObjectPtr<AActor>> ToRemove;
	for (const TPair<TWeakObjectPtr<AActor>, UWidgetComponent*>& Pair : CandidateWidgetMap)
	{
		if (!ThisFrameActors.Contains(Pair.Key))
		{
			if (Pair.Value)
			{
				Pair.Value->DestroyComponent();
			}
			ToRemove.Add(Pair.Key);
		}
	}

	for (const TWeakObjectPtr<AActor>& Key : ToRemove)
	{
		CandidateWidgetMap.Remove(Key);
	}
}

void UGazeInteractorComponent::TryInteract(AActor* InstigatorActor)
{
	if (!CurrentTargetActor || CurrentSetIndex == INDEX_NONE)
		return;

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ProcessInteract(InstigatorActor, CurrentTargetActor, CurrentSetIndex);
	}
	else
	{
		ServerTryInteract(InstigatorActor, CurrentTargetActor, CurrentSetIndex);
	}
}

void UGazeInteractorComponent::ServerTryInteract_Implementation(AActor* InstigatorActor, AActor* TargetActor, int32 SetIndex)
{
	ProcessInteract(InstigatorActor, TargetActor, SetIndex);
}

void UGazeInteractorComponent::ProcessInteract(AActor* InstigatorActor, AActor* TargetActor, int32 SetIndex)
{
	if (!TargetActor || !DetectSets.IsValidIndex(SetIndex))
		return;

	const FGazeDetectSet& Set = DetectSets[SetIndex];

	// 1) 대상 컴포넌트 찾기
	UActorComponent* TargetComp = nullptr;
	if (*Set.TargetComponentClass)
	{
		TargetComp = TargetActor->FindComponentByClass(Set.TargetComponentClass);
	}

	// 2) 실제 인터랙트 호출 (기존 코드)
	if (TargetComp && TargetComp->GetClass()->ImplementsInterface(UGazeInteractableInterface::StaticClass()))
	{
		IGazeInteractableInterface::Execute_GazeInteract(TargetComp, InstigatorActor);
	}
	else if (TargetActor->GetClass()->ImplementsInterface(UGazeInteractableInterface::StaticClass()))
	{
		IGazeInteractableInterface::Execute_GazeInteract(TargetActor, InstigatorActor);
	}

	if (UGazeInteractableComponent* GI = TargetActor->FindComponentByClass<UGazeInteractableComponent>())
	{
		GI->NotifyGazeInteracted(InstigatorActor, SetIndex);
	}
}

void UGazeInteractorComponent::ApplyGazeTextIfAny(AActor* TargetActor, UWidgetComponent* WidgetComp)
{
	if (!TargetActor || !WidgetComp)
	{
		return;
	}

	// 액터에 붙은 텍스트 설정용 컴포넌트 찾기
	UGazeTextTargetComponent* TextComp = TargetActor->FindComponentByClass<UGazeTextTargetComponent>();
	if (!TextComp)
	{
		return; // 없으면 아무것도 안 함
	}

	// 위젯 실제 인스턴스 가져오기
	UUserWidget* UserWidget = WidgetComp->GetUserWidgetObject();
	if (!UserWidget)
	{
		return;
	}

	// 이름이 지정돼 있으면 그 위젯만 찾아서 텍스트 넣기
	if (TextComp->TextWidgetName != NAME_None)
	{
		if (UWidget* Found = UserWidget->GetWidgetFromName(TextComp->TextWidgetName))
		{
			if (UTextBlock* TextBlock = Cast<UTextBlock>(Found))
			{
				TextBlock->SetText(TextComp->DisplayText);
			}
		}
	}
	else
	{
		// 이름이 없으면 위젯 전체에서 TextBlock 하나만 찾아서 넣어주는 식으로 단순 처리
		// (필요하면 여기 로직을 더 정교하게 바꿔도 됨)
		if (UTextBlock* RootText = Cast<UTextBlock>(UserWidget->GetRootWidget()))
		{
			RootText->SetText(TextComp->DisplayText);
		}
	}
}

void UGazeInteractorComponent::RefreshTextForTargetActor(AActor* TargetActor)
{
	// 메인 타깃 위젯 갱신
	if (TargetActor && TargetActor == CurrentTargetActor && CurrentWidgetComp)
	{
		ApplyGazeTextIfAny(TargetActor, CurrentWidgetComp);
	}

	// 후보 위젯도 갱신하고 싶으면 여기서도 해줄 수 있음
	if (UWidgetComponent** Found = CandidateWidgetMap.Find(TargetActor))
	{
		if (UWidgetComponent* CandWidget = *Found)
		{
			ApplyGazeTextIfAny(TargetActor, CandWidget);
		}
	}
}

void UGazeInteractorComponent::OnTargetInteractableStateChanged(AActor* TargetActor, bool bNewInteractable)
{
	if (!TargetActor)
		return;

	// 지금 내가 보고 있는 애가 이거면
	if (TargetActor == CurrentTargetActor)
	{
		if (!bNewInteractable)
		{
			// 위젯 끄기
			if (CurrentWidgetComp)
			{
				CurrentWidgetComp->SetVisibility(false, true);
			}
			// 필요하면 CurrentTargetActor도 풀어준다
			// CurrentTargetActor = nullptr;
		}
		else
		{
			// 다시 켜질 수도 있으면 여기서 다시 텍스트 적용
			if (CurrentWidgetComp)
			{
				ApplyGazeTextIfAny(TargetActor, CurrentWidgetComp);
				CurrentWidgetComp->SetVisibility(true, true);
			}
		}
	}

	// 후보 맵에도 있을 수 있으니 한번 더 처리
	if (UWidgetComponent** FoundWidget = CandidateWidgetMap.Find(TargetActor))
	{
		if (*FoundWidget)
		{
			(*FoundWidget)->SetVisibility(bNewInteractable, true);
		}
	}
}
