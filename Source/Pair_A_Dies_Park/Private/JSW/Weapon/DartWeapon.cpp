// Fill out your copyright notice in the Description page of Project Settings.

#include "JSW/Weapon/DartWeapon.h"
#include "Components/ArrowComponent.h"
#include "Kismet/GameplayStatics.h"

ADartWeapon::ADartWeapon()
{
	bInfiniteAmmo = true;
}

void ADartWeapon::StartFire()
{
	Server_Fire();
}

void ADartWeapon::Server_Fire_Implementation()
{
	FVector Start = MuzzleLocation->GetComponentLocation();
	FVector Forward = MuzzleLocation->GetForwardVector();

	float SphereRadius = 100.0f;

	FVector TraceStart = Start + (Forward * 40.0f);
	FVector End = TraceStart + (Forward * MaxRange);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(GetOwner());

	bool bHit = GetWorld()->SweepSingleByChannel(
		Hit,
		TraceStart,
		End,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(SphereRadius),
		Params
	);

	if (bHit && Hit.GetActor())
	{
		UGameplayStatics::ApplyDamage(Hit.GetActor(), Damage, GetInstigatorController(), this, UDamageType::StaticClass());
	}
}
