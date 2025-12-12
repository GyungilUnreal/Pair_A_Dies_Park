// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class AMyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Cinematic")
	void PlayBossIntro(class ALevelSequenceActor* SequenceActor);

	UFUNCTION(BlueprintCallable, Category = "Cinematic")
	void SetFullScreenCinematic(bool bIsCinematic);

protected:
	virtual void OnPossess(APawn* InPawn) override;
};
