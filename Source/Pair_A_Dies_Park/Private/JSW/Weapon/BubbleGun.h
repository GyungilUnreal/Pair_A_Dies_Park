// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JSW/Weapon/BaseWeapon.h"
#include "BubbleGun.generated.h"

UCLASS()
class ABubbleGun : public ABaseWeapon
{
	GENERATED_BODY()
	
public:
	virtual void StartFire() override;
	virtual void StopFire() override;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TSubclassOf<class ABubbleProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float FireRate = 0.15f;

protected:
	UFUNCTION(Server,Reliable)
	void Server_Fire();

	FTimerHandle FireTimerHandle;
};
