// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GazeInteractableInterface.h"
#include "BaseWeapon.generated.h"

UCLASS()
class ABaseWeapon : public AActor, public IGazeInteractableInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseWeapon();

	virtual void GazeInteract_Implementation(AActor* InstigatorActor) override; 

	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void Fire(){}

protected:
	UPROPERTY(VisibleAnywhere,BlueprintReadWrite)
	UStaticMeshComponent* BaseMesh;

	UPROPERTY(VisibleAnywhere)
	class UArrowComponent* MuzzleLocation;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnEquip();

};
