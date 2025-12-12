// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossTeleporter.generated.h"

class UBoxComponent;

UCLASS()
class ABossTeleporter : public AActor
{
	GENERATED_BODY()

public:
	ABossTeleporter();

	UFUNCTION(BlueprintCallable, Category = "Teleport")
	void TeleportAllPlayers();

	UFUNCTION(BlueprintImplementableEvent, Category = "Teleport")
	void OnTeleportFinished(APlayerController* PC);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* PortalMesh; // 포탈 모양

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UBoxComponent* TriggerBox; // 감지 영역

	// 이동할 목표 지점
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MakeEditWidget = true))
	FVector TeleportTargetOffset;

	// 필요한 인원 (2명)
	UPROPERTY(EditAnywhere)
	int32 RequiredPlayers = 2;

	int32 CurrentPlayerCount = 0;
	TArray<AActor*> OverlappingPlayers;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

};