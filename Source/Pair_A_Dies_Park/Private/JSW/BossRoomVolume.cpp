// Fill out your copyright notice in the Description page of Project Settings.


#include "JSW/BossRoomVolume.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "JSW/PlayerFallComponent.h"

ABossRoomVolume::ABossRoomVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	RootComponent = Box;

	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionObjectType(ECC_WorldStatic);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	Box->OnComponentBeginOverlap.AddDynamic(this, &ABossRoomVolume::OnBeginOverlap);
	Box->OnComponentEndOverlap.AddDynamic(this, &ABossRoomVolume::OnEndOverlap);

	bReplicates = true;
}

void ABossRoomVolume::OnBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (!HasAuthority()) return;

	ACharacter* Ch = Cast<ACharacter>(OtherActor);
	if (!Ch) return;

	if (UPlayerFallComponent* FallComp = Ch->FindComponentByClass<UPlayerFallComponent>())
	{
		FallComp->SetFallSystemEnabled(true);
	}
}

void ABossRoomVolume::OnEndOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32)
{
	if (!HasAuthority()) return;

	ACharacter* Ch = Cast<ACharacter>(OtherActor);
	if (!Ch) return;

	if (UPlayerFallComponent* FallComp = Ch->FindComponentByClass<UPlayerFallComponent>())
	{
		if (Ch->GetCharacterMovement()->IsMovingOnGround())
		{
			FallComp->SetFallSystemEnabled(false);
		}
	}
}