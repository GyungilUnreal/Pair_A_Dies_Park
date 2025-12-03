#include "JSW/PlayerFallComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "JSW/FloorManager.h"
#include "Net/UnrealNetwork.h"

UPlayerFallComponent::UPlayerFallComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UPlayerFallComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPlayerFallComponent, CurrentState);
}

void UPlayerFallComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	FloorManager = Cast<AFloorManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AFloorManager::StaticClass()));

	if (OwnerCharacter)
	{
		InitialSpawnLocation = OwnerCharacter->GetActorLocation();
	}
	if (FloorManager)
	{
		MinKillZThreshold = FloorManager->Floor1_Height - 2000.0f;
	}
	else
	{
		MinKillZThreshold = InitialSpawnLocation.Z - 2000.0f;
	}
}

void UPlayerFallComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentState == EFallState::Hanging)
	{
		if (CurrentClimbGauge > 0.0f)
		{
			CurrentClimbGauge = FMath::Max(CurrentClimbGauge - (GaugeDecayRate * DeltaTime), 0.0f);
			OnClimbGaugeChanged.Broadcast(CurrentClimbGauge / MaxClimbGauge);
		}
		return;
	}

	if (CurrentState == EFallState::Climbing) return;

	if (OwnerCharacter && OwnerCharacter->GetCharacterMovement()->IsMovingOnGround())
	{
		if (CurrentState != EFallState::Normal)
		{
			ChangeState(EFallState::Normal);
		}
	}
	else if (OwnerCharacter)
	{
		if (OwnerCharacter->GetActorLocation().Z < MinKillZThreshold && OwnerCharacter->GetActorLocation().Z > 30000)
		{
			RespawnAtFloor1();
		}
		else if (CurrentState == EFallState::Normal && OwnerCharacter->GetVelocity().Z < -100.f)
		{
			CheckFallingCondition();
		}
	}
}

void UPlayerFallComponent::SetFallSystemEnabled(bool bEnabled)
{
	if (OwnerCharacter && !OwnerCharacter->HasAuthority()) return;

	bFallEnabled = bEnabled;

	if (!bFallEnabled && CurrentState != EFallState::Normal)
	{
		ChangeState(EFallState::Normal);
	}
}