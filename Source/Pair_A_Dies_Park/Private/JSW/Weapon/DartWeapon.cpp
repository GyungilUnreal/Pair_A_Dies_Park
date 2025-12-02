// Fill out your copyright notice in the Description page of Project Settings.

#include "JSW/Weapon/DartWeapon.h"
#include "Components/ArrowComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

void ADartWeapon::Fire()
{
	bReplicates = true;

	Server_Fire();
}

void ADartWeapon::Server_Fire_Implementation()
{
	FVector Start = MuzzleLocation->GetComponentLocation();
	FVector Forward = MuzzleLocation->GetForwardVector();

	float SphereRadius = 30.0f;

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
		ECC_WorldDynamic,
		FCollisionShape::MakeSphere(SphereRadius),
		Params
	);

	// [디버그] 빨간색 궤적을 5초 동안 남김
	DrawDebugCapsule(GetWorld(), (TraceStart + End) / 2, MaxRange / 2, SphereRadius, FRotationMatrix::MakeFromZ(Forward).ToQuat(), FColor::Red, false, 5.0f);

	if (bHit && Hit.GetActor())
	{
		// [디버그] 무엇을 맞췄는지 무조건 출력!
		UE_LOG(LogTemp, Error, TEXT(">>> HIT: %s (Comp: %s) <<<"), *Hit.GetActor()->GetName(), *Hit.GetComponent()->GetName());
		DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 20.0f, FColor::Green, false, 5.0f); // 맞은 곳에 초록 점

		UGameplayStatics::ApplyDamage(Hit.GetActor(), Damage, GetInstigatorController(), this, UDamageType::StaticClass());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT(">>> HIT NOTHING <<<"));
	}
}
