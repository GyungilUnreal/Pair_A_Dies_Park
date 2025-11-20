// Fill out your copyright notice in the Description page of Project Settings.


#include "JSW/FloorManager.h"
#include "JSW/FloorTile.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AFloorManager::AFloorManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void AFloorManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// GridState 변수 복제.
	DOREPLIFETIME(AFloorManager, GridState);
}

// Called when the game starts or when spawned
void AFloorManager::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> FoundActors;
	// AFloorTile 클래스만 전부 찾음.
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFloorTile::StaticClass(), FoundActors);
	// 개수 확인용.
	const int32 ExpectedCount = GridWidth * GridWidth * 2;
	// 타일 개수가 맞게 있는지 확인.
	if (FoundActors.Num() < ExpectedCount)
		UE_LOG(LogTemp, Warning, TEXT("AFloorManager: Found %d tiles, but expected at least %d."), FoundActors.Num(), ExpectedCount);

	// Z -> X -> Y 순으로 정렬
	FoundActors.Sort([](const AActor& A, const AActor& B)
	{
		FVector LocA = A.GetActorLocation();
		FVector LocB = B.GetActorLocation();
		// 층 구분
		if (FMath::Abs(LocA.Z - LocB.Z) > 100.0f)
		{
			return LocA.Z < LocB.Z;
		}
		// 행 구분
		if (FMath::Abs(LocA.X - LocB.X) > 10.0f)
		{
			return LocA.X < LocB.X;
		}
		// 열 구분
		return LocA.Y < LocB.Y;
	});
	// 로컬 배열에 저장.
	LocalCubes = FoundActors;
	// 배열 초기화
	bIsBreakable.SetNum(LocalCubes.Num());

	if (HasAuthority())
	{
		GridState.Init(false, LocalCubes.Num());
	}

	for (int32 Index = 0; Index < LocalCubes.Num(); ++Index)
	{
		if (LocalCubes[Index])
		{
			// 클래스 멤버 변수로 확인.
			if (AFloorTile* Tile = Cast<AFloorTile>(LocalCubes[Index])) 
			{
				if (Tile->bIsBossPlatform)
				{
					bIsBreakable[Index] = false;
				}
				else
				{
					bIsBreakable[Index] = true;
				}
			}
		}
	}
	// 초기 비주얼 상태 적용.
	UpdateVisualsFromState();
}

void AFloorManager::OnRep_GridState()
{
	UpdateVisualsFromState();
}

void AFloorManager::UpdateVisualsFromState()
{
	for (int32 Index = 0; Index < LocalCubes.Num(); ++Index)
	{
		// 인덱스 유효성 검사
		if (LocalCubes.IsValidIndex(Index) && GridState.IsValidIndex(Index) && bIsBreakable.IsValidIndex(Index))
		{
			bool bShouldBeHidden = false; // 기본값 = 보임

			if (bIsBreakable[Index])
			{
				bShouldBeHidden = GridState[Index]; // 파괴 상태면 숨김
			}
			else
			{
				bShouldBeHidden = false; // 보스 발판은 항상 보임
			}
			
			// 실제 액터 적용
			LocalCubes[Index]->SetActorHiddenInGame(bShouldBeHidden);
			LocalCubes[Index]->SetActorEnableCollision(!bShouldBeHidden);
		}
	}
}

void AFloorManager::Server_BreakCubes(const TArray<FIntPoint>& CoordsToBreak)
{
	if (!HasAuthority()) return;

	bool bStateChanged = false;

	for (const FIntPoint& Coord : CoordsToBreak)
	{
		// 2차원 좌표를 1차원 인덱스로 변환.
		int32 Index = (Coord.X * GridWidth) + Coord.Y;

		// !GridState[Index]는 false(존재)
		if (bIsBreakable.IsValidIndex(Index) && bIsBreakable[Index] 
			&& GridState.IsValidIndex(Index) && !GridState[Index])
		{
			GridState[Index] = true;
			bStateChanged = true;
		}
	}
	if (bStateChanged)
	{
		UpdateVisualsFromState();
	}
}

void AFloorManager::Server_RegenerateCubes(const TArray<FIntPoint>& CoordsToRegen)
{
	if (!HasAuthority()) return;

	bool bStateChanged = false;

	for (const FIntPoint& Coord : CoordsToRegen)
	{
		// 2차원 좌표를 1차원 인덱스로 변환.
		int32 Index = (Coord.X * GridWidth) + Coord.Y;

		// GridState[Index]는 true(파괴)
		if (GridState.IsValidIndex(Index) && GridState[Index])
		{
			GridState[Index] = false;
			bStateChanged = true;
		}
	}
	if (bStateChanged)
	{
		UpdateVisualsFromState();
	}
}

