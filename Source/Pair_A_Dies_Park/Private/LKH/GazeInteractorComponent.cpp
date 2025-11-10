#include "GazeInteractorComponent.h"
#include "GazeInteractableInterface.h"

#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UGazeInteractorComponent::UGazeInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	TraceDistance = 800.f;
	GazeSweepSteps = 8;
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

	const FVector Start = CameraComp->GetComponentLocation();
	const FVector Forward = CameraComp->GetComponentRotation().Vector();
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

	// 2) 시선에서 못 찾았다면 플레이어 주변 오버랩
	if (!BestCandidate.Actor.IsValid())
	{
		CollectCandidatesAroundPlayer(Owner->GetActorLocation(), AllCandidates, BestCandidate, Params);
	}

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

void UGazeInteractorComponent::CollectCandidatesAroundPlayer(const FVector& Origin, TArray<FGazeCandidate>& InOutCandidates, FGazeCandidate& InOutBest, const FCollisionQueryParams& Params)
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

		// 아직 베스트가 없으면 이것도 베스트가 될 수 있음
		if (!InOutBest.Actor.IsValid() || NewCand.DistSqFromView < InOutBest.DistSqFromView)
		{
			InOutBest = NewCand;
		}
	}
}

bool UGazeInteractorComponent::FindMatchedComponentInActor(AActor* Actor, UActorComponent*& OutMatchedComp, int32& OutSetIndex) const
{
	OutMatchedComp = nullptr;
	OutSetIndex = INDEX_NONE;

	if (!Actor)
		return false;

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
	HideCurrentWidget();

	if (!TargetActor || !DetectSets.IsValidIndex(SetIndex))
		return;

	const FGazeDetectSet& Set = DetectSets[SetIndex];
	if (!*Set.WidgetClass)
		return;

	CurrentWidgetComp = SpawnWidgetOnActor(TargetActor, Set.WidgetClass);
}

void UGazeInteractorComponent::HideCurrentWidget()
{
	if (CurrentWidgetComp)
	{
		CurrentWidgetComp->DestroyComponent();
		CurrentWidgetComp = nullptr;
	}
}

UWidgetComponent* UGazeInteractorComponent::SpawnWidgetOnActor(AActor* TargetActor, TSubclassOf<UUserWidget> WidgetClass) const
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
	WidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 120.f)); // 머리 위
	return WidgetComp;
}

void UGazeInteractorComponent::UpdateCandidateWidgets(const TArray<FGazeCandidate>& Candidates, const FGazeCandidate& FinalTarget)
{
	// 이번 틱에 살아있는 액터 목록
	TSet<TWeakObjectPtr<AActor>> ThisFrameActors;

	for (const FGazeCandidate& C : Candidates)
	{
		// 타깃은 후보 위젯 안 만듦
		if (FinalTarget.Actor.IsValid() && C.Actor == FinalTarget.Actor)
			continue;

		ThisFrameActors.Add(C.Actor);

		AActor* Actor = C.Actor.Get();
		if (!Actor) continue;

		// 해당 세트가 후보 위젯을 지정했는지
		if (!DetectSets.IsValidIndex(C.SetIndex))
			continue;

		const FGazeDetectSet& Set = DetectSets[C.SetIndex];
		if (!*Set.CandidateWidgetClass)
			continue;

		// 이미 있으면 패스
		if (UWidgetComponent** FoundPtr = CandidateWidgetMap.Find(Actor))
		{
			// 있으면 위치만 갱신해도 됨
			if (UWidgetComponent* Found = *FoundPtr)
			{
				Found->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
			}
		}
		else
		{
			// 새로 생성
			UWidgetComponent* NewWidget = SpawnWidgetOnActor(Actor, Set.CandidateWidgetClass);
			if (NewWidget)
			{
				CandidateWidgetMap.Add(Actor, NewWidget);
			}
		}
	}

	// 이번 프레임에 없어진 애들 제거
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
	// 현재 타깃이 없으면 끝
	if (!CurrentTargetActor || CurrentSetIndex == INDEX_NONE)
	{
		return;
	}

	// 로컬에서 바로도 처리하고 싶으면 여기서 ProcessInteract 호출해도 되지만
	// 보통은 서버 권한으로 하니까 서버 RPC 부름
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

	// 대상 컴포넌트 찾기
	UActorComponent* TargetComp = nullptr;
	if (*Set.TargetComponentClass)
	{
		TargetComp = TargetActor->FindComponentByClass(Set.TargetComponentClass);
	}

	// 인터페이스 호출
	if (TargetComp && TargetComp->GetClass()->ImplementsInterface(UGazeInteractableInterface::StaticClass()))
	{
		IGazeInteractableInterface::Execute_Interact(TargetComp, InstigatorActor);
	}
	else if (TargetActor->GetClass()->ImplementsInterface(UGazeInteractableInterface::StaticClass()))
	{
		IGazeInteractableInterface::Execute_Interact(TargetActor, InstigatorActor);
	}

	// 여기서 UI 정리
	HideCurrentWidget();
	CurrentTargetActor = nullptr;
	CurrentSetIndex = INDEX_NONE;

	// 후보 위젯들도 정리
	for (auto& Pair : CandidateWidgetMap)
	{
		if (Pair.Value)
		{
			Pair.Value->DestroyComponent();
		}
	}
	CandidateWidgetMap.Empty();
}