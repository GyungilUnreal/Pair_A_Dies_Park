#include "JSW/GA/GA_BossFloorAttack.h"
#include "JSW/FloorManager.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "JSW/AI/AIC_Boss.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"

UGA_BossFloorAttack::UGA_BossFloorAttack()
{
	FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Boss.FloorSlam"));
	AbilityTags.AddTag(AbilityTag);
}

void UGA_BossFloorAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AFloorManager* FloorManager = Cast<AFloorManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AFloorManager::StaticClass())
	);

	AActor* BossActor = GetAvatarActorFromActorInfo();
	APawn* BossPawn = Cast<APawn>(BossActor);
	bool bFoundTarget = false;

	if (FloorManager && BossPawn)
	{
		AAIController* AIC = Cast<AAIController>(BossPawn->GetController());
		AActor* TargetActor = nullptr;

		if (AIC && AIC->GetBlackboardComponent())
		{
			TargetActor = Cast<AActor>(AIC->GetBlackboardComponent()->GetValueAsObject(FBossBlackboardKeys::FocusTargetKey));
		}

		FVector TargetLoc = TargetActor ? TargetActor->GetActorLocation() : BossActor->GetActorForwardVector() * 500.0f + BossActor->GetActorLocation();

		if (FloorManager->WorldToGridIndex(TargetLoc, CachedTargetLayer, CachedTargetCoord))
		{
			bFoundTarget = true;

			if (ReplicatedDecalBP)
			{
				TArray<FIntPoint> Offsets = {
					FIntPoint(0, 0),
					FIntPoint(1, 0),
					FIntPoint(-1, 0),
					FIntPoint(0, 1),
					FIntPoint(0, -1)
				};

				for (const FIntPoint& Off : Offsets)
				{
					FIntPoint CurrentCoord = CachedTargetCoord + Off;

					FVector TilePos = FloorManager->GetTileWorldLocation(CachedTargetLayer, CurrentCoord);

					if (TilePos.IsNearlyZero()) continue;

					FVector DecalSpawnLoc = FVector(TilePos.X, TilePos.Y, TargetLoc.Z + 50.0f);
					FRotator DecalRot = FRotator(-90.0f, 0.0f, 0.0f);

					ADecalActor* NewDecal = GetWorld()->SpawnActor<ADecalActor>(ReplicatedDecalBP, DecalSpawnLoc, DecalRot);
					if (NewDecal)
					{
						NewDecal->SetLifeSpan(2.0f);
					}
				}
			}
		}
	}

	if (!bFoundTarget)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AttackMontage);
	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &UGA_BossFloorAttack::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_BossFloorAttack::OnMontageCompleted);
		MontageTask->ReadyForActivation();
	}

	FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Boss.Smash"));
	UAbilityTask_WaitGameplayEvent* WaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, EventTag);
	if (WaitTask)
	{
		WaitTask->EventReceived.AddDynamic(this, &UGA_BossFloorAttack::OnSmashEventReceived);
		WaitTask->ReadyForActivation();
	}
}

void UGA_BossFloorAttack::OnSmashEventReceived(FGameplayEventData Payload)
{
	if (!GetActorInfo().IsNetAuthority()) return;

	AFloorManager* FloorManager = Cast<AFloorManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AFloorManager::StaticClass())
	);

	if (FloorManager)
	{

		FloorManager->Server_DamageTile(CachedTargetLayer, CachedTargetCoord, 2);

		FloorManager->Server_DamageTile(CachedTargetLayer, CachedTargetCoord + FIntPoint(1, 0), 2);
		FloorManager->Server_DamageTile(CachedTargetLayer, CachedTargetCoord + FIntPoint(-1, 0), 2);
		FloorManager->Server_DamageTile(CachedTargetLayer, CachedTargetCoord + FIntPoint(0, 1), 2);
		FloorManager->Server_DamageTile(CachedTargetLayer, CachedTargetCoord + FIntPoint(0, -1), 2);

		UE_LOG(LogTemp, Warning, TEXT("!!! SMASH !!! Destroyed Cached Grid: Layer %d, [%d, %d]"), CachedTargetLayer, CachedTargetCoord.X, CachedTargetCoord.Y);
	}
}

void UGA_BossFloorAttack::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}