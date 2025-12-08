// Fill out your copyright notice in the Description page of Project Settings.

#include "JSW/BossTeleporter.h"
#include "JSW/BossCharacter.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

ABossTeleporter::ABossTeleporter()
{
	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	RootComponent = PortalMesh;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(RootComponent);
	TriggerBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
}

void ABossTeleporter::BeginPlay()
{
	Super::BeginPlay();
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ABossTeleporter::OnOverlapBegin);
	TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ABossTeleporter::OnOverlapEnd);
}

void ABossTeleporter::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACharacter* Player = Cast<ACharacter>(OtherActor);
	if (Player && !OverlappingPlayers.Contains(Player))
	{
		OverlappingPlayers.Add(Player);
		CurrentPlayerCount++;

		if (CurrentPlayerCount >= RequiredPlayers)
		{
			TeleportAllPlayers();
		}
	}
}

void ABossTeleporter::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ACharacter* Player = Cast<ACharacter>(OtherActor);
	if (Player)
	{
		OverlappingPlayers.Remove(Player);
		CurrentPlayerCount--;
	}
}

void ABossTeleporter::TeleportAllPlayers()
{
	FVector TargetLoc = GetActorLocation() + TeleportTargetOffset;

	TArray<AActor*> PlayersToTeleport = OverlappingPlayers;

	for (AActor* Actor : PlayersToTeleport)
	{
		ACharacter* Char = Cast<ACharacter>(Actor);
		if (Char)
		{
			// 화면 깜빡임
			APlayerController* PC = Cast<APlayerController>(Char->GetController());
			if (PC)
			{
				PC->ClientSetCameraFade(true, FColor::Black, FVector2D(0.0f, 1.0f), 0.5f, true, true);
			}

			// 위치 이동 
			Char->SetActorLocation(TargetLoc);

			// 화면 켜기
			if (PC)
			{
				PC->ClientSetCameraFade(true, FColor::Black, FVector2D(1.0f, 0.0f), 0.5f, false, true);
			}
			AActor* BossActor = UGameplayStatics::GetActorOfClass(GetWorld(), ABossCharacter::StaticClass());
			ABossCharacter* Boss = Cast<ABossCharacter>(BossActor);

			if (Boss)
			{
				Boss->WakeUpBoss();
			}
		}
	}

	OverlappingPlayers.Empty();
	CurrentPlayerCount = 0;
}