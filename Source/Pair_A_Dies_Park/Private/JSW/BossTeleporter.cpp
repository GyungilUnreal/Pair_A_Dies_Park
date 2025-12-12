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

	AActor* BossActor = UGameplayStatics::GetActorOfClass(GetWorld(), ABossCharacter::StaticClass());
	if (ABossCharacter* Boss = Cast<ABossCharacter>(BossActor))
	{
		Boss->WakeUpBoss();
	}

	for (AActor* Actor : PlayersToTeleport)
	{
		if (ACharacter* Char = Cast<ACharacter>(Actor))
		{
			APlayerController* PC = Cast<APlayerController>(Char->GetController());

			Char->SetActorLocation(TargetLoc);

			if (PC)
			{
				PC->ClientSetCameraFade(true, FColor::Black, FVector2D(1.0f, 0.0f), 1.0f, false, true);

				OnTeleportFinished(PC);
			}
		}
	}

	OverlappingPlayers.Empty();
	CurrentPlayerCount = 0;
}