// Fill out your copyright notice in the Description page of Project Settings.

#include "JSW/FloorManager.h"
#include "JSW/FloorTile.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

AFloorManager::AFloorManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void AFloorManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFloorManager, GridData);
}

void AFloorManager::BeginPlay()
{
	Super::BeginPlay();

	// AFloorTile 클래스로 모든 큐브 찾기
	TArray<AActor*> FoundCubes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFloorTile::StaticClass(), FoundCubes);

	// 162개가 맞는지 확인 (1층 81 + 2층 81)
	const int32 ExpectedCount = TilesPerLayer * 2;

	if (FoundCubes.Num() < ExpectedCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("FloorManager: Found %d cubes, expected at least %d"), FoundCubes.Num(), ExpectedCount);
	}

	// 정렬: 층(Z) -> 행(X) -> 열(Y) 순서
	FoundCubes.Sort([this](const AActor& A, const AActor& B) {
		FVector LocA = A.GetActorLocation();
		FVector LocB = B.GetActorLocation();

		// 1순위: Z축 (층 구분) - 오차범위 넉넉히
		if (FMath::Abs(LocA.Z - LocB.Z) > 1000.0f)
		{
			return LocA.Z < LocB.Z; // 1층(낮음)이 먼저
		}
		// 2순위: X축
		if (FMath::Abs(LocA.X - LocB.X) > 10.0f)
		{
			return LocA.X < LocB.X;
		}
		// 3순위: Y축
		return LocA.Y < LocB.Y;
	});

	// 데이터 배열 초기화
	// 서버: 초기값 설정 / 클라이언트: 배열 크기 확보 및 액터 매핑

	GridData.SetNum(FoundCubes.Num());

	for (int32 i = 0; i < FoundCubes.Num(); ++i)
	{
		// 액터 연결
		GridData[i].VisualActor = FoundCubes[i];

		if (HasAuthority())
		{
			GridData[i].HP = 2; // 기본 체력 2

			// 보스 발판(파괴 불가) 처리
			if (AFloorTile* Tile = Cast<AFloorTile>(FoundCubes[i]))
			{
				if (Tile->bIsBossPlatform)
				{
					GridData[i].HP = 255; // 255를 파괴 불가(무적)으로 약속
				}
			}
		}
	}

	UpdateVisualsFromState();
}

void AFloorManager::OnRep_GridData()
{
	UpdateVisualsFromState();
}

void AFloorManager::UpdateVisualsFromState()
{
	for (const FTileData& Tile : GridData)
	{
		if (Tile.VisualActor)
		{
			// HP > 0 이면 보임, 0이면 숨김
			bool bVisible = (Tile.HP > 0);

			// 숨김 처리
			Tile.VisualActor->SetActorHiddenInGame(!bVisible);
			Tile.VisualActor->SetActorEnableCollision(bVisible);

			// TODO: 나중에 HP가 1일 때 '금 간 머티리얼'로 변경하는 로직 추가
		}
	}
}

bool AFloorManager::WorldToGridIndex(FVector WorldPos, int32& OutLayer, FIntPoint& OutCoord)
{
	// 층 판별 (중간값 기준)
	float MidHeight = (Floor1_Height + Floor2_Height) * 0.5f;

	if (WorldPos.Z >= MidHeight)
	{
		OutLayer = 1; // 2층
	}
	else
	{
		OutLayer = 0; // 1층
	}

	// 안전 장치 (너무 아래면 맵 밖)
	if (WorldPos.Z < (Floor1_Height - 3000.0f))
	{
		return false;
	}

	// 로컬 좌표 (매니저 기준)
	FVector RelativePos = WorldPos - GetActorLocation();

	// 인덱스 계산
	int32 X = FMath::RoundToInt(RelativePos.X / TileSize);
	int32 Y = FMath::RoundToInt(RelativePos.Y / TileSize);

	if (X >= 0 && X < GridWidth && Y >= 0 && Y < GridWidth)
	{
		OutCoord = FIntPoint(X, Y);
		return true;
	}

	return false;
}

bool AFloorManager::IsTileWalkable(int32 Layer, FIntPoint Coord)
{
	int32 Index = (Layer * TilesPerLayer) + (Coord.X * GridWidth) + Coord.Y;

	if (GridData.IsValidIndex(Index))
	{
		return GridData[Index].HP > 0;
	}
	return false;
}

FVector AFloorManager::GetTileWorldLocation(int32 Layer, FIntPoint Coord)
{
	int32 Index = (Layer * TilesPerLayer) + (Coord.X * GridWidth) + Coord.Y;
	if (GridData.IsValidIndex(Index) && GridData[Index].VisualActor)
	{
		return GridData[Index].VisualActor->GetActorLocation();
	}
	return FVector::ZeroVector;
}

void AFloorManager::Server_DamageTile(int32 Layer, FIntPoint Coord, int32 DamageAmount)
{
	if (!HasAuthority()) return;

	int32 Index = (Layer * TilesPerLayer) + (Coord.X * GridWidth) + Coord.Y;

	if (GridData.IsValidIndex(Index))
	{
		uint8 CurrentHP = GridData[Index].HP;

		// 무적(255)이거나 이미 파괴된(0) 경우 무시
		if (CurrentHP == 255 || CurrentHP == 0) return;

		int32 NewHP = (int32)CurrentHP - DamageAmount;
		GridData[Index].HP = (uint8)FMath::Max(0, NewHP);

		// 서버 비주얼 갱신 (리플리케이션은 자동)
		UpdateVisualsFromState();
	}
}

void AFloorManager::Server_RestoreRandomTiles(int32 Layer, int32 Count)
{
	if (!HasAuthority()) return;

	// 파괴된(HP=0) 타일들의 인덱스를 모음
	TArray<int32> BrokenIndices;
	int32 StartIndex = Layer * TilesPerLayer;
	int32 EndIndex = StartIndex + TilesPerLayer;

	for (int32 i = StartIndex; i < EndIndex; ++i)
	{
		if (GridData.IsValidIndex(i) && GridData[i].HP == 0)
		{
			BrokenIndices.Add(i);
		}
	}

	// 셔플 후 복구
	if (BrokenIndices.Num() > 0)
	{
		int32 RestoreCount = FMath::Min(Count, BrokenIndices.Num());

		for (int32 i = 0; i < RestoreCount; ++i)
		{
			int32 RandIdx = FMath::RandRange(0, BrokenIndices.Num() - 1);
			int32 TargetIndex = BrokenIndices[RandIdx];

			GridData[TargetIndex].HP = 2; // 체력 2로 복구

			BrokenIndices.RemoveAt(RandIdx); // 중복 방지
		}
		UpdateVisualsFromState();
	}
}