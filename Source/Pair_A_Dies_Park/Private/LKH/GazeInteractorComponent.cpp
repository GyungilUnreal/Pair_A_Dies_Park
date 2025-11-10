#include "GazeInteractorComponent.h"
#include "GazeInteractableInterface.h"

#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"

UGazeInteractorComponent::UGazeInteractorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);

    TraceDistance = 800.f;
    TraceChannel = ECC_Visibility;
    CurrentTargetActor = nullptr;
    CurrentSetIndex = INDEX_NONE;
    bDrawDebugLine = true;
    bOnlyLocal = true;
    CurrentWidgetComp = nullptr;
}

void UGazeInteractorComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UGazeInteractorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    HideCurrentWidget();
    Super::EndPlay(EndPlayReason);
}

void UGazeInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 로컬 클라만 감지하도록 한 경우면 여기서 걸러줌
    if (bOnlyLocal && !IsOwnerLocal())
    {
        return;
    }

    PerformGazeTrace();

    // 위젯이 살아있으면 카메라 쪽으로 보게 만든다
    if (CurrentWidgetComp && IsOwnerLocal())
    {
        AActor* Owner = GetOwner();
        if (Owner)
        {
            if (UCameraComponent* Cam = Owner->FindComponentByClass<UCameraComponent>())
            {
                const FVector CamLoc = Cam->GetComponentLocation();
                const FVector WidgetLoc = CurrentWidgetComp->GetComponentLocation();

                FVector ToCam = CamLoc - WidgetLoc;
                ToCam.Normalize();

                // 위아래도 따라가게 하려면 이 줄 빼기
                // ToCam.Z = 0.f;

                const FRotator FaceRot = ToCam.Rotation();
                CurrentWidgetComp->SetWorldRotation(FaceRot);
            }
        }
    }
}

bool UGazeInteractorComponent::IsOwnerLocal() const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return false;
    }

    AController* Ctrl = Owner->GetInstigatorController();
    APlayerController* PC = Cast<APlayerController>(Ctrl);
    return (PC && PC->IsLocalController());
}

void UGazeInteractorComponent::PerformGazeTrace()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    UCameraComponent* CameraComp = Owner->FindComponentByClass<UCameraComponent>();
    if (!CameraComp)
    {
        return;
    }

    const FVector Start = CameraComp->GetComponentLocation();
    const FVector End = Start + CameraComp->GetComponentRotation().Vector() * TraceDistance;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Owner);

    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params);

#if WITH_EDITOR
    if (bDrawDebugLine)
    {
        DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 0.f, 0, 1.5f);
    }
#endif

    if (bHit && Hit.GetActor())
    {
        AActor* HitActor = Hit.GetActor();
        int32 MatchedSet = FindMatchedSetIndex(HitActor);
        if (MatchedSet != INDEX_NONE)
        {
            // 다른 애를 봤거나, 같은 애인데 다른 세트면 UI 갱신
            if (HitActor != CurrentTargetActor || MatchedSet != CurrentSetIndex)
            {
                ShowWidgetForSet(HitActor, MatchedSet);
            }

            // 인터랙트용 캐시
            CurrentTargetActor = HitActor;
            CurrentSetIndex = MatchedSet;
            CurrentMatchedComponent = nullptr;

            // 실제로 맞은 컴포넌트도 기억해두고 싶으면 여기서 찾음
            const FGazeDetectSet& Set = DetectSets[MatchedSet];
            TArray<UActorComponent*> ActorComps = HitActor->GetComponents().Array();
            for (UActorComponent* Comp : ActorComps)
            {
                if (Comp && Comp->IsA(Set.TargetComponentClass))
                {
                    CurrentMatchedComponent = Comp;
                    break;
                }
            }

            return; // 감지 성공했으므로 종료
        }
    }

    // 못 맞췄으면 초기화
    HideCurrentWidget();
    CurrentTargetActor = nullptr;
    CurrentSetIndex = INDEX_NONE;
    CurrentMatchedComponent = nullptr;
}

int32 UGazeInteractorComponent::FindMatchedSetIndex(AActor* Actor) const
{
    if (!Actor)
    {
        return INDEX_NONE;
    }

    TArray<UActorComponent*> ActorComps = Actor->GetComponents().Array();

    for (int32 i = 0; i < DetectSets.Num(); ++i)
    {
        const FGazeDetectSet& Set = DetectSets[i];
        if (!*Set.TargetComponentClass)
        {
            continue;
        }

        for (UActorComponent* Comp : ActorComps)
        {
            if (Comp && Comp->IsA(Set.TargetComponentClass))
            {
                return i;
            }
        }
    }

    return INDEX_NONE;
}

void UGazeInteractorComponent::ShowWidgetForSet(AActor* TargetActor, int32 SetIndex)
{
    if (!TargetActor || !DetectSets.IsValidIndex(SetIndex))
    {
        HideCurrentWidget();
        return;
    }

    // 로컬 UI만 띄울 거면 여기서도 체크
    if (bOnlyLocal && !IsOwnerLocal())
    {
        return;
    }

    // 기존 위젯 제거
    HideCurrentWidget();

    const FGazeDetectSet& Set = DetectSets[SetIndex];
    if (!*Set.WidgetClass)
    {
        return; // 이 세트는 UI 안 띄움
    }

    // 타겟 액터에 위젯 컴포넌트를 동적으로 붙인다
    UWidgetComponent* NewWidgetComp = NewObject<UWidgetComponent>(TargetActor);
    if (!NewWidgetComp)
    {
        return;
    }

    NewWidgetComp->RegisterComponent();
    NewWidgetComp->SetWidgetSpace(EWidgetSpace::World);
    NewWidgetComp->SetWidgetClass(Set.WidgetClass);
    NewWidgetComp->SetDrawSize(FVector2D(300.f, 100.f)); // 필요하면 세트에다 빼기
    NewWidgetComp->SetTwoSided(true);

    // 그림자 안 생기게
    NewWidgetComp->SetCastShadow(false);
    NewWidgetComp->SetReceivesDecals(false);

    // 붙일 위치 (액터 머리 위 정도)
    USceneComponent* AttachComp = TargetActor->GetRootComponent();
    if (AttachComp)
    {
        NewWidgetComp->AttachToComponent(AttachComp, FAttachmentTransformRules::KeepRelativeTransform);
        NewWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
    }

    CurrentWidgetComp = NewWidgetComp;
}

void UGazeInteractorComponent::HideCurrentWidget()
{
    if (CurrentWidgetComp)
    {
        CurrentWidgetComp->DestroyComponent();
        CurrentWidgetComp = nullptr;
    }
}

void UGazeInteractorComponent::TryInteract(AActor* InstigatorActor)
{
    if (!InstigatorActor)
    {
        return;
    }

    if (!CurrentTargetActor || CurrentSetIndex == INDEX_NONE)
    {
        return; // 볼 대상 없으면 종료
    }

    // 서버면 바로 처리
    if (InstigatorActor->HasAuthority())
    {
        ProcessInteract(InstigatorActor, CurrentTargetActor, CurrentSetIndex);
    }
    else
    {
        // 클라이언트면 서버로 요청
        ServerTryInteract(InstigatorActor, CurrentTargetActor, CurrentSetIndex);
    }
}

void UGazeInteractorComponent::ServerTryInteract_Implementation(AActor* InstigatorActor, AActor* TargetActor, int32 SetIndex)
{
    ProcessInteract(InstigatorActor, TargetActor, SetIndex);
}

void UGazeInteractorComponent::ProcessInteract(AActor* InstigatorActor, AActor* TargetActor, int32 SetIndex)
{
    if (!InstigatorActor || !TargetActor)
    {
        return;
    }

    // 1) 액터가 인터페이스 구현했으면 그걸 먼저
    if (TargetActor->GetClass()->ImplementsInterface(UGazeInteractableInterface::StaticClass()))
    {
        IGazeInteractableInterface::Execute_Interact(TargetActor, InstigatorActor);
        return;
    }

    // 2) 세트에 맞는 컴포넌트를 찾아서 인터페이스 호출
    if (DetectSets.IsValidIndex(SetIndex))
    {
        const FGazeDetectSet& Set = DetectSets[SetIndex];
        TArray<UActorComponent*> ActorComps = TargetActor->GetComponents().Array();
        for (UActorComponent* Comp : ActorComps)
        {
            if (!Comp)
            {
                continue;
            }

            // 세트에서 지정한 컴포넌트 타입과 맞거나, 세트가 비어있으면 그냥 인터페이스 검사
            if (!*Set.TargetComponentClass || Comp->IsA(Set.TargetComponentClass))
            {
                if (Comp->GetClass()->ImplementsInterface(UGazeInteractableInterface::StaticClass()))
                {
                    IGazeInteractableInterface::Execute_Interact(Comp, InstigatorActor);
                    break;
                }
            }
        }
    }
}