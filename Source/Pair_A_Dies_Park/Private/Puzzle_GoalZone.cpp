// Fill out your copyright notice in the Description page of Project Settings.

#include "Puzzle_GoalZone.h"
#include "Components/BoxComponent.h"
#include "Split_Character.h"
#include "RoomController.h"

APuzzle_GoalZone::APuzzle_GoalZone()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	RootComponent = TriggerVolume;

	// 오버랩 이벤트에 함수 바인딩
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &APuzzle_GoalZone::OnOverlapBegin);

	TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &APuzzle_GoalZone::OnOverlapEnd);

	bReplicates = true;
}

void APuzzle_GoalZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    ASplit_Character* Player = Cast<ASplit_Character>(OtherActor); 

	if (Player == nullptr)
		return;

	if (HasAuthority())
	{
        // 이미 완료된 퍼즐이면 아무것도 안 함
        if (IsCompletedPuzzle()) return;

        // 중복 추가 방지하며 배열에 추가
        if (!PlayerInZone.Contains(Player))
        {
            PlayerInZone.Add(Player);
            UE_LOG(LogTemp, Warning, TEXT("%s entered. Current players in zone: %d"), *Player->GetName(), PlayerInZone.Num());
        }

        // 인원 수 체크
        if (PlayerInZone.Num() >= RequiredPlayerCount)
        {
            UE_LOG(LogTemp, Warning, TEXT("Goal Zone Puzzle COMPLETED! (Required: %d, Current: %d)"), RequiredPlayerCount, PlayerInZone.Num());

            // 퍼즐 완료
            CompletePuzzle(); 
        }
	}
}

void APuzzle_GoalZone::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    ASplit_Character* Player = Cast<ASplit_Character>(OtherActor);

    if (Player == nullptr)
        return;

    if (HasAuthority())
    {
        PlayerInZone.Remove(Player);
        UE_LOG(LogTemp, Warning, TEXT("%s left. Current players in zone: %d"), *Player->GetName(), PlayerInZone.Num());
    }
}

void APuzzle_GoalZone::BeginPlay()
{
}