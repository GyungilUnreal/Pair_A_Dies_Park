// Fill out your copyright notice in the Description page of Project Settings.


#include "JSW/PlayerFallComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "FloorManager.h"
#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "JSW/FloorTile.h" 
#include "JSW/RescueInteractableComponent.h"

// Sets default values for this component's properties
UPlayerFallComponent::UPlayerFallComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true); // 컴포넌트 복제 설정.
}


// Called when the game starts
void UPlayerFallComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	FloorManager = Cast<AFloorManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AFloorManager::StaticClass()));

	if (FloorManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("FallComponent: FloorManager Found!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("FallComponent: FloorManager NOT Found! Check Level Actor."));
	}
}


// Called every frame
void UPlayerFallComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentState == EFallState::Hanging)
	{
		return;
	}

	if (OwnerCharacter->GetCharacterMovement()->IsMovingOnGround())
	{
		if (CurrentState != EFallState::Normal)
		{
			ChangeState(EFallState::Normal);
		}
	}

	if (CurrentState == EFallState::Normal || CurrentState == EFallState::Falling)
	{
		CheckFallingCondition();
	}
}

void UPlayerFallComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPlayerFallComponent, CurrentState);
}

void UPlayerFallComponent::OnRep_CurrentState()
{
	ApplyStateLogic();
}

void UPlayerFallComponent::ApplyStateLogic()
{
	GetWorld()->GetTimerManager().ClearTimer(HangTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(ClimbUpTimerHandle);


	if (!OwnerCharacter) return;
	UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement();
	URescueInteractableComponent* RescueComp = OwnerCharacter->FindComponentByClass<URescueInteractableComponent>();

	switch (CurrentState)
	{
	case EFallState::Normal:
		OwnerCharacter->SetActorEnableCollision(true);
		if (Movement)
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
		if (OwnerCharacter->GetMesh()->GetAnimInstance()->Montage_IsPlaying(HangingMontage))
		{
			OwnerCharacter->StopAnimMontage(HangingMontage);
		}
		if (RescueComp) RescueComp->SetIsInteractable(false);
		break;

	case EFallState::Hanging:
		if (Movement)
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_Flying); // 중력 무시
			Movement->DisableMovement();
		}
		if (HangingMontage)
		{
			OwnerCharacter->PlayAnimMontage(HangingMontage);
		}
		if (RescueComp) RescueComp->SetIsInteractable(true);

		if (OwnerCharacter->HasAuthority())
		{
			GetWorld()->GetTimerManager().SetTimer(
				HangTimerHandle,
				this,
				&UPlayerFallComponent::OnHangTimerExpired,
				MaxHangTime,
				false
			);
		}
		// TODO: 여기서 "살려줘!" UI(위젯)를 띄우거나 외치기 사운드 재생 가능
		UE_LOG(LogTemp, Warning, TEXT("State Changed: Hanging - Animation Started"));
		break;

	case EFallState::Climbing:
	{
		if (Movement)
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_Flying);
		}

		OwnerCharacter->SetActorEnableCollision(false);

		float Duration = 1.0f;
		if (ClimbUpMontage)
		{
			Duration = OwnerCharacter->PlayAnimMontage(ClimbUpMontage);
		}

		if (RescueComp) RescueComp->SetIsInteractable(false);

		if (OwnerCharacter->HasAuthority())
		{
			float WaitTime = FMath::Max(0.1f, Duration - 0.2f);

			GetWorld()->GetTimerManager().SetTimer(
				ClimbUpTimerHandle,
				this,
				&UPlayerFallComponent::FinishClimbing,
				WaitTime,
				false
			);
		}
		break;
	}


	case EFallState::Falling:
		// TODO 2층 추락. 
		break;
	}
}

void UPlayerFallComponent::CheckFallingCondition()
{
	if (!OwnerCharacter) return;
	// 쿨타임.
	if (!bCanGrabLedge)
	{
		float Now = GetWorld()->GetTimeSeconds();

		if (Now - LastDetachTime < LedgeRegrabCooldown)
		{
			return;
		}
		else
		{
			bCanGrabLedge = true;
		}
	}

	// 떨어지는 중인지 확인
	float VelZ = OwnerCharacter->GetVelocity().Z;
	// 너무 느리게 떨어질 때(걷기 등)는 무시 (-100.f 정도가 적당)
	if (VelZ >= -100.f) return;

	FVector LedgeLoc;
	FRotator LedgeRot;
	FHitResult WallHit; // 어떤 벽을 잡았는지 확인하기 위해 필요

	// CheckLedgeTrace 함수를 조금 수정해서 HitResult를 뱉게 하거나,
	// 여기서 직접 트레이스를 쏘는 게 낫습니다. (아래 CheckLedgeTrace 수정 참고)
	if (CheckLedgeTrace(LedgeLoc, LedgeRot, WallHit))
	{
		// 잡은 벽이 '몇 층'인지 확인
		if (AActor* HitActor = WallHit.GetActor())
		{
			float HitActorZ = HitActor->GetActorLocation().Z;

			// 1층 높이 근처 (예: 43111)
			// FloorManager가 없어도 대략적인 높이로 판단하거나, Manager의 변수 참조
			// (여기서는 편의상 FloorManager를 통해 높이를 가져온다고 가정)
			float Floor1_Z = FloorManager ? FloorManager->Floor1_Height : 43111.0f;
			float Floor2_Z = FloorManager ? FloorManager->Floor2_Height : 49550.0f;

			// 1층 벽을 잡음 -> 매달리기
			if (FMath::IsNearlyEqual(HitActorZ, Floor1_Z, 500.0f))
			{
				UE_LOG(LogTemp, Warning, TEXT("1F Wall Detected! Hanging Start."));

				// 상태 변경 (애니메이션 재생 시작)
				ChangeState(EFallState::Hanging);

				// 회전만 벽을 보게 확실히 맞춤
				FRotator TargetRot = (-WallHit.ImpactNormal).Rotation();
				TargetRot.Pitch = 0.f;
				TargetRot.Roll = 0.f;
				OwnerCharacter->SetActorRotation(TargetRot);

				// 위치 보정 함수 호출 (0.05초 뒤, 애니메이션 블렌딩이 약간 진행된 후 실행)
				FTimerDelegate TimerDel;
				TimerDel.BindUObject(this, &UPlayerFallComponent::SnapActorToLedge, LedgeLoc, TargetRot);

				GetWorld()->GetTimerManager().SetTimer(CorrectionTimerHandle, TimerDel, 0.05f, false);
			}
			// 2층 벽을 잡음 (혹은 스침) -> QTE 추락
			else if (FMath::IsNearlyEqual(HitActorZ, Floor2_Z, 500.0f))
			{
				UE_LOG(LogTemp, Warning, TEXT("2F Wall Detected! QTE Start."));
				ChangeState(EFallState::Falling);
			}
		}
	}
	else
	{
		// 벽을 못 잡고 허공으로 떨어지는 중 (2층 구멍 등)
		// 2층 높이에서 떨어지고 있다면 QTE 진입
		float MyZ = OwnerCharacter->GetActorLocation().Z;
		float Floor2_Z = FloorManager ? FloorManager->Floor2_Height : 49550.0f;

		// 2층 높이보다 약간 아래로 떨어졌을 때
		if (MyZ < Floor2_Z - 200.0f && MyZ > Floor2_Z - 2000.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Falling from 2F (No Wall)! QTE Start."));
			ChangeState(EFallState::Falling);
		}
	}
}

void UPlayerFallComponent::ChangeState(EFallState NewState)
{
	if (CurrentState == NewState) return;

	// 서버는 변수를 바꾸고 로직을 직접 실행
	CurrentState = NewState;
	ApplyStateLogic();
}

bool UPlayerFallComponent::CheckLedgeTrace(FVector& OutLedgeLoc, FRotator& OutLedgeRot, FHitResult& OutWallHit)
{
	if (!OwnerCharacter) return false;

	const FVector Start = OwnerCharacter->GetActorLocation();
	const FVector Forward = OwnerCharacter->GetActorForwardVector();
	float CapsuleHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	const FVector ForwardStart = Start; // 캡슐 중심(가슴/배)
	const FVector ForwardEnd = ForwardStart + (Forward * 150.f); // 길이 1.5m

	DrawDebugCapsule(GetWorld(), (ForwardStart + ForwardEnd) / 2, 75.f, 20.f, Forward.Rotation().Quaternion(), FColor::Red, false, 1.f);

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

	// 벽을 찾았는지 확인
	if (bHitWall)
	{
		AActor* HitActor = OutWallHit.GetActor();
		// 발판(AFloorTile) 클래스인지 확인
		if (!HitActor || !Cast<AFloorTile>(HitActor))
		{
			return false;
		}

		//벽 안쪽 위에서 아래로 "정확한 손잡이 위치" 찾기
		FVector IntoWallDir = -OutWallHit.ImpactNormal; // 벽 안쪽 방향
		const FVector DownStart = OutWallHit.ImpactPoint + (IntoWallDir * 15.f) + FVector(0, 0, 150.f);
		const FVector DownEnd = DownStart - FVector(0, 0, 250.f); // 충분히 길게 내림

		FHitResult LedgeHit;
		bool bHitLedge = GetWorld()->LineTraceSingleByChannel(
			LedgeHit,
			DownStart,
			DownEnd,
			ECC_WorldStatic,
			QueryParams
		);

		if (bHitLedge)
		{
			// 너무 높거나 낮은 건 잡지 않음 (자연스러움)
			float LedgeZ = LedgeHit.ImpactPoint.Z;
			float MyHeadZ = Start.Z + CapsuleHalfHeight;
			float HeightDiff = LedgeZ - MyHeadZ;

			// 머리 위 50cm ~ 발 아래 50cm 사이만 인정
			if (HeightDiff > 50.0f || HeightDiff < -100.0f)
			{
				return false;
			}

			// 위치 및 회전 계산
			float HangOffsetZ = 170.f;  // 손 높이 보정
			float HangOffsetFwd = 35.f; // 벽에서 떨어질 거리 (스피어라서 조금 더 띄움)

			OutLedgeLoc = LedgeHit.ImpactPoint - (IntoWallDir * HangOffsetFwd) - FVector(0, 0, HangOffsetZ);
			OutLedgeRot = IntoWallDir.Rotation();
			OutLedgeRot.Pitch = 0.f;
			OutLedgeRot.Roll = 0.f;

			return true;
		}
	}

	return false;
}

void UPlayerFallComponent::FinishClimbing()
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority()) return;

	UE_LOG(LogTemp, Warning, TEXT("Climb Animation Finished. Teleporting to Safe Zone."));

	// 계산해둔 안전지대로 이동
	OwnerCharacter->SetActorLocationAndRotation(TargetSafeLocation, TargetSafeRotation);

	// 상태를 Normal로 변경 
	ChangeState(EFallState::Normal);
}

void UPlayerFallComponent::SnapActorToLedge(FVector TargetLedgeLoc, FRotator TargetLedgeRot)
{
	if (CurrentState != EFallState::Hanging || !OwnerCharacter) return;

	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!Mesh) return;

	// 현재 내 손의 월드 위치 가져오기
	FVector HandWorldLoc = Mesh->GetSocketLocation(HangingHandSocket);

	// 오차 계산 (벽 위치 - 현재 손 위치)
	// 벽 위치를 조금 보정합니다. (손이 벽 속에 파묻히지 않고 표면에 닿게)
	// 벽 안쪽 법선 벡터
	FVector IntoWallDir = TargetLedgeRot.Vector();

	// 목표 지점 = 트레이스로 찾은 벽 위치 - (벽 안쪽으로 조금 띄우기, 손 두께 고려)
	FVector FinalTargetPoint = TargetLedgeLoc - (IntoWallDir * 5.0f); // 5cm 정도 띄움

	FVector Diff = FinalTargetPoint - HandWorldLoc;

	// 캐릭터 전체를 오차만큼 이동 (손이 목표 지점으로 이동됨)
	OwnerCharacter->AddActorWorldOffset(Diff);

	UE_LOG(LogTemp, Warning, TEXT("Snapped Actor to Ledge based on %s Socket"), *HangingHandSocket.ToString());
}

void UPlayerFallComponent::OnHangTimerExpired()
{
	// 이미 구조되었거나 상태가 변했다면 무시
	if (CurrentState != EFallState::Hanging) return;

	UE_LOG(LogTemp, Warning, TEXT("Stamina Depleted! Dropping..."));

	bCanGrabLedge = false;
	LastDetachTime = GetWorld()->GetTimeSeconds();

	// 강제로 Normal 상태로 변경 -> 중력 적용되어 떨어짐
	ChangeState(EFallState::Normal);

	// 떨어질 때 약간 앞으로 튕겨나가게
	if (OwnerCharacter)
	{
		// 벽 반대 방향으로 살짝 밀어줌
		FVector KnockbackDir = -OwnerCharacter->GetActorForwardVector();
		OwnerCharacter->LaunchCharacter(KnockbackDir * 400.f, true, true);
	}
}

void UPlayerFallComponent::TryRescue(AActor* RescuerActor)
{
	if (CurrentState != EFallState::Hanging) return;
	if (!OwnerCharacter) return;

	// 구조자가 있으면 구조자 옆으로 이동
	if (RescuerActor)
	{
		TargetSafeLocation = RescuerActor->GetActorLocation() + (RescuerActor->GetActorRightVector() * 100.f)
			+ FVector(0.f, 0.f, 10.f);
		TargetSafeRotation = RescuerActor->GetActorRotation();
	}
	else
	{
		// 구조자가 없으면 벽 위로 텔레포트
		TargetSafeLocation = OwnerCharacter->GetActorLocation() + (OwnerCharacter->GetActorForwardVector() * 100.f) +
			FVector(0.f, 0.f, 250.f);
		TargetSafeRotation = OwnerCharacter->GetActorRotation();
	}
	ChangeState(EFallState::Climbing);
}

