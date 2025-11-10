#include "ItemAbilityComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"

UItemAbilityComponent::UItemAbilityComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

UAbilitySystemComponent* UItemAbilityComponent::GetASC(AActor* TargetActor) const
{
    if (!TargetActor) return nullptr;

    if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor))
    {
        return ASI->GetAbilitySystemComponent();
    }
    return nullptr;
}

void UItemAbilityComponent::GrantToActor(AActor* TargetActor)
{
    if (!TargetActor)
    {
        return;
    }

    // 서버 권한 보장
    const AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority())
    {
        // Server_GrantToActor(TargetActor); // 필요하면 서버 RPC 호출
        return;
    }

    UAbilitySystemComponent* ASC = GetASC(TargetActor);
    if (!ASC)
    {
        return;
    }

    FGrantedAbilityHandles& Entry = GrantedHandles.FindOrAdd(TargetActor);

    // 이미 부여된 경우 중복 방지
    if (Entry.Handles.Num() > 0)
    {
        return;
    }

    // 능력 부여
    for (const FGrantAbilityData& Data : AbilitiesToGrant)
    {
        if (!Data.AbilityClass)
        {
            continue;
        }

        FGameplayAbilitySpec Spec(Data.AbilityClass, Data.AbilityLevel);

        // 부여 주체를 넘겨줌 (권장: 아이템 액터 또는 이 컴포넌트)
        Spec.SourceObject = this; // 또는: Spec.SourceObject = GetOwner(); // 아이템 액터

        if (Data.InputID >= 0)
        {
            Spec.InputID = Data.InputID;
        }

        const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
        Entry.Handles.Add(Handle);
    }

    // 충전 횟수 초기화
    if (MaxCharges > 0)
    {
        CurrentCharges.FindOrAdd(TargetActor, MaxCharges);
    }
}

void UItemAbilityComponent::RemoveFromActor(AActor* TargetActor)
{
    if (auto* ASC = GetASC(TargetActor))
    {
        if (FGrantedAbilityHandles* Container = GrantedHandles.Find(TargetActor))
        {
            for (auto& Handle : Container->Handles)
            {
                ASC->ClearAbility(Handle);
            }
        }
    }
    GrantedHandles.Remove(TargetActor);
}

void UItemAbilityComponent::NotifyAbilityConsumed(AActor* TargetActor)
{
    if (MaxCharges <= 0) return; // 무제한

    int32* ChargesPtr = CurrentCharges.Find(TargetActor);
    if (!ChargesPtr) return;

    (*ChargesPtr)--;

    if (*ChargesPtr <= 0)
    {
        if (AActor* OwnerActor = GetOwner())
        {
            // 블루프린트에서 바인딩한 애들 호출
            OnAbilityConsumed.Broadcast(TargetActor);

            // 일단 안 보이게 하고, 충돌도 끔
            OwnerActor->SetActorHiddenInGame(true);
            OwnerActor->SetActorEnableCollision(false);
            OwnerActor->SetActorTickEnabled(false);

            // 3초 후에 어빌리티 회수 및 삭제
            FTimerHandle DestroyTimerHandle;
            OwnerActor->GetWorldTimerManager().SetTimer(
                DestroyTimerHandle,
                [this, TargetActor, OwnerActor]()
                {
                    // 어빌리티 회수
                    RemoveFromActor(TargetActor);

                    // 액터 유효하면 삭제
                    if (IsValid(OwnerActor))
                    {
                        OwnerActor->Destroy();
                    }
                },
                3.0f,  // 지연 시간 (3초)
                false  // 반복 안 함
            );
        }
    }
}

void UItemAbilityComponent::AttachItemToSocket(AActor* TargetActor, const FName SocketName)
{
    if (!TargetActor) return;

    // 붙일 주체(아이템)
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor) return;

    // TargetActor의 Mesh 컴포넌트 가져오기
    USkeletalMeshComponent* TargetMesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>();
    if (!TargetMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("TargetActor %s has no SkeletalMeshComponent!"), *TargetActor->GetName());
        return;
    }

    // 소켓 존재 확인
    if (!TargetMesh->DoesSocketExist(SocketName))
    {
        UE_LOG(LogTemp, Warning, TEXT("Socket %s does not exist on %s"), *SocketName.ToString(), *TargetActor->GetName());
        return;
    }

    // 부착
    OwnerActor->AttachToComponent(TargetMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
}