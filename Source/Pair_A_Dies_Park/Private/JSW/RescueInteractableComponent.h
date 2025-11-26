// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GazeInteractableComponent.h"
#include "GazeInteractableInterface.h"
#include "RescueInteractableComponent.generated.h"


/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class URescueInteractableComponent : public UGazeInteractableComponent, public IGazeInteractableInterface
{
	GENERATED_BODY()
	
public:
	URescueInteractableComponent();

	virtual void GazeInteract_Implementation(AActor* InstigatorActor) override;
};
