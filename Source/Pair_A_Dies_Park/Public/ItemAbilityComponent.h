#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayAbilitySpec.h"
#include "ItemAbilityComponent.generated.h"

class UAbilitySystemComponent;

UENUM(BlueprintType)
enum class EGrantTrigger : uint8
{
    OnPickup,     // 줍는 순간
    OnEquip,      // 장착 시
    Manual        // 수동 호출
};

USTRUCT(BlueprintType)
struct FGrantAbilityData
{
    GENERATED_BODY()

    // 어떤 능력을 줄지
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UGameplayAbility> AbilityClass;

    // 몇 레벨로 줄지
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 AbilityLevel = 1;

    // 입력 슬롯(원하면)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 InputID = -1;

    // 장착 해제 시 이 능력을 빼줄 것인지
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRemoveOnUnequip = true;
};

USTRUCT()
struct FGrantedAbilityHandles
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FGameplayAbilitySpecHandle> Handles;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UItemAbilityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UItemAbilityComponent();

    // 언제 줄지
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
    EGrantTrigger GrantTrigger = EGrantTrigger::OnEquip;

    // 부여할 능력들
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
    TArray<FGrantAbilityData> AbilitiesToGrant;

    // 사용 가능 횟수 (0 이면 무제한)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Use")
    int32 MaxCharges = 0;

    // 사용 시 1씩 깎을지
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Use")
    bool bConsumeOnAbilityUse = false;

    // 장착 해제 시 효과/능력 제거
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
    bool bRemoveAllOnUnequip = true;

    // 외부에서 부를 함수
    UFUNCTION(BlueprintCallable, Category = "GAS")
    void GrantToActor(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category = "GAS")
    void RemoveFromActor(AActor* TargetActor);

    // 능력 1회 사용했다고 알릴 때
    UFUNCTION(BlueprintCallable, Category = "GAS")
    void NotifyAbilityConsumed(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category = "GAS")
    void AttachItemToSocket(AActor* TargetActor, const FName SocketName);

protected:
    // Target마다 Handle을 기억해야 나중에 뺄 수 있음
    UPROPERTY()
    TMap<TWeakObjectPtr<AActor>, FGrantedAbilityHandles> GrantedHandles;

    UPROPERTY()
    TMap<TWeakObjectPtr<AActor>, int32> CurrentCharges;

    UAbilitySystemComponent* GetASC(AActor* TargetActor) const;
};
