// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JSW/Weapon/BaseWeapon.h"
#include "DartWeapon.generated.h"

UCLASS()
class ADartWeapon : public ABaseWeapon
{
	GENERATED_BODY()

public:
	ADartWeapon();

	virtual void StartFire() override;

protected:
	UFUNCTION(Server,Reliable)
	void Server_Fire();

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float MaxRange = 5000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float Damage = 10.0f;
	
};
