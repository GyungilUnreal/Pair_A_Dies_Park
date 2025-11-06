// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JHS/Puzzle/PuzzleActionBase.h"
#include "TutorialPlatform.generated.h"

UCLASS()
class ATutorialPlatform : public APuzzleActionBase
{
	GENERATED_BODY()
	
public:
	ATutorialPlatform();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlatformMesh;

	FVector OriginalLocation;

	virtual void BeginPlay() override;

	virtual void OnActivatePuzzleAction() override;

	virtual void OnDeactivatePuzzleAction() override;

};
