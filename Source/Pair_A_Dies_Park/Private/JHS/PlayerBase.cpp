// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/PlayerBase.h"
#include "Kismet/GameplayStatics.h"
#include "JHS/GameControll/MyGameinstance.h"

// Sets default values
APlayerBase::APlayerBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APlayerBase::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void APlayerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APlayerBase::SetPlayerScale(float PlayerScale)
{
	UE_LOG(LogTemp, Warning, TEXT("Player scale : [%f]"), PlayerScale);
	RootComponent->SetWorldScale3D(FVector(PlayerScale, PlayerScale, PlayerScale));
}

