// Fill out your copyright notice in the Description page of Project Settings.


#include "JSW/Weapon/BubbleGun.h"
#include "BubbleProjectile.h"
#include "Components/ArrowComponent.h"

void ABubbleGun::Fire()
{
	Server_Fire();
}

void ABubbleGun::Server_Fire_Implementation()
{
	if (!ProjectileClass) return;

	FVector SpawnLoc = MuzzleLocation->GetComponentLocation();
	FRotator SpawnRot = MuzzleLocation->GetComponentRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(ProjectileClass, SpawnLoc, SpawnRot, SpawnParams);

	if (ABubbleProjectile* Bubble = Cast<ABubbleProjectile>(SpawnedActor))
	{
		FVector LaunchVelocity = MuzzleLocation->GetForwardVector() * 200.0f;
		Bubble->SetVelocity(LaunchVelocity);
	}
}
