#include "JSW/PlayerFallComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "JSW/FloorManager.h"
#include "JSW/FloorTile.h"

void UPlayerFallComponent::CheckFallingCondition()
{
	if (!OwnerCharacter) return;
	if (!bFallEnabled) return;

	if (!bCanGrabLedge)
	{
		float Now = GetWorld()->GetTimeSeconds();
		if (Now - LastDetachTime < LedgeRegrabCooldown) return;
		else bCanGrabLedge = true;
	}

	float VelZ = OwnerCharacter->GetVelocity().Z;
	if (VelZ >= -100.f) return;

	FVector LedgeLoc;
	FRotator LedgeRot;
	FHitResult WallHit;

	if (CheckLedgeTrace(LedgeLoc, LedgeRot, WallHit))
	{
		if (AActor* HitActor = WallHit.GetActor())
		{
			float HitZ = HitActor->GetActorLocation().Z;
			float F1_Z = FloorManager ? FloorManager->Floor1_Height : 43111.0f;
			float F2_Z = FloorManager ? FloorManager->Floor2_Height : 49550.0f;

			if (FMath::IsNearlyEqual(HitZ, F1_Z, 500.0f) || FMath::IsNearlyEqual(HitZ, F2_Z, 500.0f))
			{
				StartHanging(WallHit, LedgeLoc);
			}
		}
	}
	else
	{
		float MyZ = OwnerCharacter->GetActorLocation().Z;
		float F2_Z = FloorManager ? FloorManager->Floor2_Height : 49550.0f;
		if (MyZ < F2_Z - 200.0f && MyZ > F2_Z - 2000.0f)
		{
			ChangeState(EFallState::Falling);
		}
	}
}

bool UPlayerFallComponent::CheckLedgeTrace(FVector& OutLedgeLoc, FRotator& OutLedgeRot, FHitResult& OutWallHit)
{
	if (!OwnerCharacter) return false;

	const FVector Start = OwnerCharacter->GetActorLocation();
	const FVector Forward = OwnerCharacter->GetActorForwardVector();

	const FVector ForwardStart = Start;
	const FVector ForwardEnd = ForwardStart + (Forward * 150.f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	bool bHitWall = GetWorld()->SweepSingleByChannel(
		OutWallHit,
		ForwardStart,
		ForwardEnd,
		FQuat::Identity,
		ECC_WorldStatic,
		FCollisionShape::MakeSphere(150.f),
		QueryParams
	);

	if (bHitWall)
	{
		AActor* HitActor = OutWallHit.GetActor();

		if (!HitActor || !Cast<AFloorTile>(HitActor))
		{
			return false;
		}

		FVector IntoWallDir = -OutWallHit.ImpactNormal;
		const FVector DownStart = OutWallHit.ImpactPoint + (IntoWallDir * 15.f) + FVector(0, 0, 150.f);
		const FVector DownEnd = DownStart - FVector(0, 0, 250.f);

		FHitResult LedgeHit;
		bool bHitLedge = GetWorld()->LineTraceSingleByChannel(LedgeHit, DownStart, DownEnd, ECC_WorldStatic, QueryParams);

		if (bHitLedge)
		{
			float LedgeZ = LedgeHit.ImpactPoint.Z;
			float MyHeadZ = Start.Z + OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			float HeightDiff = LedgeZ - MyHeadZ;

			if (HeightDiff > 50.0f || HeightDiff < -100.0f) return false;

			float HangOffsetZ = 100.f;
			float HangOffsetFwd = 30.f;

			OutLedgeLoc = LedgeHit.ImpactPoint - (IntoWallDir * HangOffsetFwd) - FVector(0, 0, HangOffsetZ);
			OutLedgeRot = IntoWallDir.Rotation();
			OutLedgeRot.Pitch = 0.f;
			OutLedgeRot.Roll = 0.f;
			return true;
		}
	}
	return false;
}

void UPlayerFallComponent::StartHanging(const FHitResult& WallHit, const FVector& LedgeLoc)
{
	ChangeState(EFallState::Hanging);

	FRotator TargetRot = (-WallHit.ImpactNormal).Rotation();
	TargetRot.Pitch = 0.f;
	TargetRot.Roll = 0.f;
	OwnerCharacter->SetActorRotation(TargetRot);

	FTimerDelegate TimerDel;
	TimerDel.BindUObject(this, &UPlayerFallComponent::SnapActorToLedge, LedgeLoc, TargetRot);
	GetWorld()->GetTimerManager().SetTimer(CorrectionTimerHandle, TimerDel, 0.05f, false);
}

void UPlayerFallComponent::SnapActorToLedge(FVector TargetLedgeLoc, FRotator TargetLedgeRot)
{
	if (CurrentState != EFallState::Hanging || !OwnerCharacter) return;
	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!Mesh) return;

	FVector HandWorldLoc = Mesh->GetSocketLocation(HangingHandSocket);
	FVector IntoWallDir = TargetLedgeRot.Vector();
	FVector FinalTargetPoint = TargetLedgeLoc - (IntoWallDir * 5.0f);
	FVector Diff = FinalTargetPoint - HandWorldLoc;
	OwnerCharacter->AddActorWorldOffset(Diff);
}

void UPlayerFallComponent::OnHangTimerExpired()
{
	if (CurrentState != EFallState::Hanging) return;
	PerformWallDrop();
}

void UPlayerFallComponent::PerformWallDrop()
{
	if (CurrentState != EFallState::Hanging) return;
	bCanGrabLedge = false;
	LastDetachTime = GetWorld()->GetTimeSeconds();
	ChangeState(EFallState::Normal);

	if (OwnerCharacter)
	{
		FVector KnockbackDir = -OwnerCharacter->GetActorForwardVector();
		OwnerCharacter->LaunchCharacter(KnockbackDir * 400.f, true, true);
	}
}