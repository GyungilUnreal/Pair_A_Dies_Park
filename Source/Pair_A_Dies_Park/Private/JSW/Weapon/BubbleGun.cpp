// Fill out your copyright notice in the Description page of Project Settings.


#include "JSW/Weapon/BubbleGun.h"
#include "BubbleProjectile.h"
#include "Components/ArrowComponent.h"
#include "TimerManager.h"

void ABubbleGun::StartFire()
{
	if (GetWorldTimerManager().IsTimerActive(FireTimerHandle)) return;

	Server_Fire();
	GetWorld()->GetTimerManager().SetTimer(FireTimerHandle, this, &ABubbleGun::Server_Fire, FireRate, true);
}

void ABubbleGun::StopFire()
{
	GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
}

void ABubbleGun::Server_Fire_Implementation()
{
	if (!ProjectileClass) return;

	if (!ConsumeAmmo())
	{
		StopFire();
		return;
	}

	FVector SpawnLoc = MuzzleLocation->GetComponentLocation();
	FRotator SpawnRot = MuzzleLocation->GetComponentRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(ProjectileClass, SpawnLoc, SpawnRot, SpawnParams);

	if (ABubbleProjectile* Bubble = Cast<ABubbleProjectile>(SpawnedActor))
	{
		FVector LaunchVelocity = MuzzleLocation->GetForwardVector() * 400.0f;
		Bubble->SetVelocity(LaunchVelocity);
	}
}
