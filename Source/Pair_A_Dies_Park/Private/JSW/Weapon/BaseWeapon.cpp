// Fill out your copyright notice in the Description page of Project Settings.

#include "JSW/Weapon/BaseWeapon.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Components/ArrowComponent.h"
#include "Split_Character.h"
#include "GazeTextTargetComponent.h"

// Sets default values
ABaseWeapon::ABaseWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;
	BaseMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	MuzzleLocation = CreateDefaultSubobject<UArrowComponent>(TEXT("MuzzleLocation"));
	MuzzleLocation->SetupAttachment(BaseMesh);
}

void ABaseWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseWeapon, CurrentAmmo);
}

void ABaseWeapon::GazeInteract_Implementation(AActor* InstigatorActor)
{
	ASplit_Character* Player = Cast<ASplit_Character>(InstigatorActor);
	if (Player)
	{
		Player->EquipWeapon(this);
		Multicast_OnEquip();
		if (HasAuthority())
		{
			if (!bInfiniteAmmo) CurrentAmmo = MaxAmmo;
			OnRep_CurrentAmmo();
		}
	}
}

void ABaseWeapon::StartFire() {}
void ABaseWeapon::StopFire() {}

void ABaseWeapon::Multicast_OnEquip_Implementation()
{
	// 메인 몸통 물리/충돌 끄기
	if (BaseMesh)
	{
		BaseMesh->SetSimulatePhysics(false);
		BaseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 붙어있는 모든 자식 부품들 처리
	TArray<UPrimitiveComponent*> ChildComps;
	GetComponents<UPrimitiveComponent>(ChildComps);

	for (UPrimitiveComponent* Comp : ChildComps)
	{
		// 몸통은 이미 껐으니 패스
		if (Comp == BaseMesh) continue;

		Comp->SetSimulatePhysics(false);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// UI 텍스트 끄기
	if (UActorComponent* TextComp = GetComponentByClass(UGazeTextTargetComponent::StaticClass()))
	{
		TextComp->DestroyComponent();
	}
}

void ABaseWeapon::OnRep_CurrentAmmo()
{
	OnAmmoChanged.Broadcast(CurrentAmmo, MaxAmmo);
}

bool ABaseWeapon::ConsumeAmmo()
{
	if (bInfiniteAmmo) return true;

	if (CurrentAmmo > 0)
	{
		CurrentAmmo--;
		OnRep_CurrentAmmo();
		return true;
	}
	return false;
}