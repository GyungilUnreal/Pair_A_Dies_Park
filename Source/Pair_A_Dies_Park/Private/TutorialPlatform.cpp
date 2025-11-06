// Fill out your copyright notice in the Description page of Project Settings.


#include "TutorialPlatform.h"

void ATutorialPlatform::BeginPlay()
{
	Super::BeginPlay();

	// 게임 시작 시 현재 위치 저장.
	OriginalLocation = GetActorLocation();
}

// Sets default values
ATutorialPlatform::ATutorialPlatform()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
	RootComponent = PlatformMesh;

	// 멀티 복제
	bReplicates = true;
	PlatformMesh->SetIsReplicated(true);

}

void ATutorialPlatform::OnActivatePuzzleAction()
{
	UE_LOG(LogTemp, Warning, TEXT("Platform %s activated"), *GetName());

	FVector NewLocation = OriginalLocation + FVector(0, 0, 500.f);
	SetActorLocation(NewLocation);
}

void ATutorialPlatform::OnDeactivatePuzzleAction()
{
	UE_LOG(LogTemp, Warning, TEXT("Platform %s deactivated"), *GetName());

	SetActorLocation(OriginalLocation);
}