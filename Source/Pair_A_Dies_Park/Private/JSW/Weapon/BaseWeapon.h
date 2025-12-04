// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GazeInteractableInterface.h"
#include "BaseWeapon.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponAmmoChanged, int32, CurrentAmmo, int32, MaxAmmo);

UCLASS()
class ABaseWeapon : public AActor, public IGazeInteractableInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseWeapon();

	virtual void GazeInteract_Implementation(AActor* InstigatorActor) override; 

	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void StopFire();

	UPROPERTY(BlueprintAssignable, Category = "Combat|UI")
	FOnWeaponAmmoChanged OnAmmoChanged;

protected:
	UPROPERTY(VisibleAnywhere,BlueprintReadWrite)
	UStaticMeshComponent* BaseMesh;

	UPROPERTY(VisibleAnywhere)
	class UArrowComponent* MuzzleLocation;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnEquip();

	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bInfiniteAmmo = false;

	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 MaxAmmo = 30;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentAmmo)
	int32 CurrentAmmo;

	UFUNCTION()
	void OnRep_CurrentAmmo();

	// 탄약 소모 함수
	bool ConsumeAmmo();

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};