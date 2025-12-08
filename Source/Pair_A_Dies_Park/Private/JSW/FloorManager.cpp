// Fill out your copyright notice in the Description page of Project Settings.

#include "JSW/FloorManager.h"
#include "JSW/FloorTile.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

AFloorManager::AFloorManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void SetActorActive(AActor* TargetActor, bool bActive)
{
	if (!TargetActor) return;

	TargetActor->SetActorHiddenInGame(!bActive);
	TargetActor->SetActorEnableCollision(bActive);
	TargetActor->SetActorTickEnabled(bActive);
}

void AFloorManager::ActivateClearItem()
{
	if (!HasAuthority()) return;

	int32 CenterLayer = 0;
	for (int32 x = 3; x <= 5; ++x)
	{
		for (int32 y = 3; y <= 5; ++y)
		{
			int32 Index = (CenterLayer * TilesPerLayer) + (x * GridWidth) + y;
			if (GridData.IsValidIndex(Index))
			{
				GridData[Index].HP = 2;
			}
		}
	}

	Multicast_ActivateClearVisuals();
}

void AFloorManager::Multicast_ActivateClearVisuals_Implementation()
{
	UpdateVisualsFromState();

	if (ClearRewardItem)
	{
		SetActorActive(ClearRewardItem, true);
	}
}

void AFloorManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFloorManager, GridData);
	DOREPLIFETIME(AFloorManager, TeamLife);
}

void AFloorManager::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (ClearRewardItem)
		{
			SetActorActive(ClearRewardItem, false);
			UE_LOG(LogTemp, Log, TEXT("ClearRewardItem Hidden by FloorManager BeginPlay"));
		}
	}

	TArray<AActor*> FoundCubes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFloorTile::StaticClass(), FoundCubes);

	TArray<AActor*> FoundStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundStarts);

	if (FoundStarts.Num() > 0)
	{
		for (AActor* StartActor : FoundStarts)
		{
			InitialSpawnLocations.Add(StartActor->GetActorLocation());
		}
	}
	else
	{
		InitialSpawnLocations.Add(GetActorLocation() + FVector(0, 0, 200));
	}
	const int32 ExpectedCount = TilesPerLayer * 2;

	if (FoundCubes.Num() < ExpectedCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("FloorManager: Found %d cubes, expected at least %d"), FoundCubes.Num(), ExpectedCount);
	}

	// 정렬: 층(Z) -> 행(X) -> 열(Y) 순서
	FoundCubes.Sort([this](const AActor& A, const AActor& B) {
		FVector LocA = A.GetActorLocation();
		FVector LocB = B.GetActorLocation();

		if (FMath::Abs(LocA.Z - LocB.Z) > 1000.0f)
		{
			return LocA.Z < LocB.Z;
		}

		int32 GridXA = FMath::RoundToInt(LocA.X / TileSize);
		int32 GridXB = FMath::RoundToInt(LocB.X / TileSize);

		if (GridXA != GridXB)
		{
			return GridXA < GridXB;
		}

		int32 GridYA = FMath::RoundToInt(LocA.Y / TileSize);
		int32 GridYB = FMath::RoundToInt(LocB.Y / TileSize);

		return GridYA < GridYB;
	});

	GridData.SetNum(FoundCubes.Num());

	for (int32 i = 0; i < FoundCubes.Num(); ++i)
	{
		GridData[i].VisualActor = FoundCubes[i];

		if (HasAuthority())
		{
			GridData[i].HP = 2;

			if (AFloorTile* Tile = Cast<AFloorTile>(FoundCubes[i]))
			{
				if (Tile->bIsBossPlatform)
				{
					GridData[i].HP = 255;
					
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

void AFloorManager::OnRep_TeamLife()
{
	OnTeamLifeChanged.Broadcast(TeamLife);
}

void AFloorManager::UpdateVisualsFromState()
{
	for (const FTileData& Tile : GridData)
	{
		if (Tile.VisualActor)
		{
			if (Tile.HP == 0 || Tile.HP == 255)
			{
				Tile.VisualActor->SetActorHiddenInGame(true);
				Tile.VisualActor->SetActorEnableCollision(false);
			}
			else
			{
				Tile.VisualActor->SetActorHiddenInGame(false);
				Tile.VisualActor->SetActorEnableCollision(true);
			}

			// TODO: 나중에 HP가 1일 때 '금 간 머티리얼'로 변경하는 로직은 여기(else)에 추가
		}
	}
}

bool AFloorManager::WorldToGridIndex(FVector WorldPos, int32& OutLayer, FIntPoint& OutCoord)
{
	float MidHeight = (Floor1_Height + Floor2_Height) * 0.5f;

	if (WorldPos.Z >= MidHeight)
	{
		OutLayer = 1;
	}
	else
	{
		OutLayer = 0;
	}

	if (WorldPos.Z < (Floor1_Height - 3000.0f))
	{
		return false;
	}

	FVector RelativePos = WorldPos - GetActorLocation();

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

		if (CurrentHP == 255 || CurrentHP == 0) return;

		int32 NewHP = (int32)CurrentHP - DamageAmount;
		GridData[Index].HP = (uint8)FMath::Max(0, NewHP);

		UpdateVisualsFromState();
	}
}

void AFloorManager::Server_RestoreRandomTiles(int32 Layer, int32 Count)
{
	if (!HasAuthority()) return;

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

	if (BrokenIndices.Num() > 0)
	{
		int32 RestoreCount = FMath::Min(Count, BrokenIndices.Num());

		for (int32 i = 0; i < RestoreCount; ++i)
		{
			int32 RandIdx = FMath::RandRange(0, BrokenIndices.Num() - 1);
			int32 TargetIndex = BrokenIndices[RandIdx];

			GridData[TargetIndex].HP = 2;

			BrokenIndices.RemoveAt(RandIdx);
		}
		UpdateVisualsFromState();
	}
}

FVector AFloorManager::GetRandomSafeFloorLocation()
{
	TArray<AActor*> SafeCandidates;

	for (const FTileData& Tile : GridData)
	{
		if (Tile.VisualActor && Tile.HP > 0 && Tile.HP < 10)
		{
			if (FMath::IsNearlyEqual(Tile.VisualActor->GetActorLocation().Z, Floor1_Height, 500.0f))
			{
				SafeCandidates.Add(Tile.VisualActor);
			}
		}
	}

	if (SafeCandidates.Num() > 0)
	{
		int32 RandIdx = FMath::RandRange(0, SafeCandidates.Num() - 1);
		return SafeCandidates[RandIdx]->GetActorLocation() + FVector(0.0f, 0.0f, 2000.0f);
	}

	if (InitialSpawnLocations.Num() > 0)
	{
		int32 RandIdx = FMath::RandRange(0, InitialSpawnLocations.Num() - 1);
		return InitialSpawnLocations[RandIdx];
	}

	return GetActorLocation() + FVector(0.0f, 0.0f, 300.0f);
}

void AFloorManager::ModifyTeamLife(int32 Amount)
{
	if (!HasAuthority()) return;

	TeamLife = FMath::Clamp(TeamLife - Amount, 0, 5);

	OnRep_TeamLife();

	if (TeamLife <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("GAME OVERRRRR"));
	}
}

void AFloorManager::IncrementFallCount()
{
	FallCount++;
	OnFallCountChanged.Broadcast(FallCount);
	UE_LOG(LogTemp, Log, TEXT("Fall Count Increased: %d"), FallCount);
}
