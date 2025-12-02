// Fill out your copyright notice in the Description page of Project Settings.


#include "JSW/PlayerFallComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "JSW/FloorManager.h"
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

void UPlayerFallComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPlayerFallComponent, CurrentState);
}

// Called when the game starts
void UPlayerFallComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	FloorManager = Cast<AFloorManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AFloorManager::StaticClass()));
}

// Called every frame
void UPlayerFallComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Hanging일 때만 감소
	if (CurrentState == EFallState::Hanging)
	{
		if (CurrentClimbGauge > 0.0f)
		{
			CurrentClimbGauge = FMath::Max(CurrentClimbGauge - (GaugeDecayRate * DeltaTime), 0.0f);
			OnClimbGaugeChanged.Broadcast(CurrentClimbGauge / MaxClimbGauge);
		}
		return;
	}

	// Climbing 중에는 아무것도 안함
	if (CurrentState == EFallState::Climbing) return;

	// 착지 감지
	if (OwnerCharacter->GetCharacterMovement()->IsMovingOnGround())
	{
		if (CurrentState != EFallState::Normal)
		{
			ChangeState(EFallState::Normal);
			if (CurrentState == EFallState::Falling)
			{
				UE_LOG(LogTemp, Warning, TEXT("Landed Safely! Back to Normal."));
			}
		}
	}
	// 공중
	else
	{
		// 나락 감지 (KillZ)
		if (OwnerCharacter->GetActorLocation().Z < 41000.0f)
		{
			RespawnAtFloor1();
		}
		// 일반 낙하 중 (벽 잡기 시도)
		else if (CurrentState == EFallState::Normal && OwnerCharacter->GetVelocity().Z < -100.f)
		{
			CheckFallingCondition();
		}
	}
}

void UPlayerFallComponent::Multicast_StartRescuerAction_Implementation(ACharacter* RescuerChar, float Duration)
{
	if (!RescuerChar) return;

	RescuerChar->GetCharacterMovement()->DisableMovement();

	UAnimInstance* AnimInst = RescuerChar->GetMesh()->GetAnimInstance();
	if (AnimInst && RescueMontage)
	{
		RescuerChar->PlayAnimMontage(RescueMontage, 1.0f);
	}

	// 약한 참조 사용 (대기 중 구조자가 나가거나 죽을 수 있음)
	TWeakObjectPtr<ACharacter> WeakRescuer(RescuerChar);

	FTimerHandle RescuerFinishTimer;
	GetWorld()->GetTimerManager().SetTimer(RescuerFinishTimer, [this, WeakRescuer]()
	{
		if (WeakRescuer.IsValid())
		{
			FinishRescuerAction(WeakRescuer.Get());
		}
	}, Duration, false);
}

void UPlayerFallComponent::FinishRescuerAction(ACharacter* RescuerChar)
{
	if (!RescuerChar) return;

	UAnimInstance* AnimInst = RescuerChar->GetMesh()->GetAnimInstance();

	// 몽타주 역재생
	if (AnimInst && RescueMontage && AnimInst->Montage_IsPlaying(RescueMontage))
	{
		// 현재 재생 위치 가져오기
		float CurrentPos = AnimInst->Montage_GetPosition(RescueMontage);

		// 재생 속도 -1.0으로 변경
		AnimInst->Montage_SetPlayRate(RescueMontage, -1.5f); // 1.5배 빠르게 일어서기

		// 만약 몽타주가 이미 끝나서 멈춰있다면, 끝지점부터 다시 틀어야 함
		if (AnimInst->Montage_GetIsStopped(RescueMontage))
		{
			RescuerChar->PlayAnimMontage(RescueMontage, -1.5f, NAME_None);
			// 시작 위치를 끝(Length)으로 강제 이동
			AnimInst->Montage_SetPosition(RescueMontage, RescueMontage->GetPlayLength());
		}
	}

	// 움직임 복구
	RescuerChar->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
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

	CurrentClimbGauge = 0.0f;
	OnClimbGaugeChanged.Broadcast(0.0f);

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
		if (Movement)
		{
			Movement->SetMovementMode(MOVE_Falling);
		}
		OwnerCharacter->SetActorEnableCollision(true);
		break;
	}
}

void UPlayerFallComponent::CheckFallingCondition()
{
	if (!OwnerCharacter) return;

	// 쿨타임 체크
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

	// 벽을 발견했는가
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
		// 벽이 없는 2층 공중
		float MyZ = OwnerCharacter->GetActorLocation().Z;
		float F2_Z = FloorManager ? FloorManager->Floor2_Height : 49550.0f;

		if (MyZ < F2_Z - 200.0f && MyZ > F2_Z - 2000.0f)
		{
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

	// 벽을 찾았는지
	if (bHitWall)
	{
		AActor* HitActor = OutWallHit.GetActor();
		// 발판 클래스인지
		if (!HitActor || !Cast<AFloorTile>(HitActor))
		{
			return false;
		}

		//벽 안쪽 위에서 아래로 위치 찾기
		FVector IntoWallDir = -OutWallHit.ImpactNormal; // 벽 안쪽 방향
		const FVector DownStart = OutWallHit.ImpactPoint + (IntoWallDir * 15.f) + FVector(0, 0, 150.f);
		const FVector DownEnd = DownStart - FVector(0, 0, 250.f);

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
			float LedgeZ = LedgeHit.ImpactPoint.Z;
			float MyHeadZ = Start.Z + CapsuleHalfHeight;
			float HeightDiff = LedgeZ - MyHeadZ;

			// 머리 위 50cm ~ 발 아래 50cm 사이만 인정
			if (HeightDiff > 50.0f || HeightDiff < -100.0f)
			{
				return false;
			}

			// 위치 및 회전 계산
			float HangOffsetZ = 100.f;  // 손 높이 보정
			float HangOffsetFwd = 30.f; // 벽에서 떨어질 거리

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

	FVector IntoWallDir = TargetLedgeRot.Vector();

	// 목표 지점 = 트레이스로 찾은 벽 위치 - (벽 안쪽으로 조금 띄우기, 손 두께 고려)
	FVector FinalTargetPoint = TargetLedgeLoc - (IntoWallDir * 5.0f);

	FVector Diff = FinalTargetPoint - HandWorldLoc;

	// 캐릭터 전체를 오차만큼 이동
	OwnerCharacter->AddActorWorldOffset(Diff);

	UE_LOG(LogTemp, Warning, TEXT("Snapped Actor to Ledge based on %s Socket"), *HangingHandSocket.ToString());
}

void UPlayerFallComponent::PerformWallDrop()
{
	// 매달린 상태가 아니면 무시
	if (CurrentState != EFallState::Hanging) return;

	// 쿨타임 적용
	bCanGrabLedge = false;
	LastDetachTime = GetWorld()->GetTimeSeconds();

	// 상태 변경
	ChangeState(EFallState::Normal);

	// 벽 반대 방향으로 튕겨내기
	if (OwnerCharacter)
	{
		FVector KnockbackDir = -OwnerCharacter->GetActorForwardVector();
		OwnerCharacter->LaunchCharacter(KnockbackDir * 400.f, true, true);
	}
}

void UPlayerFallComponent::RespawnAtFloor1()
{
	// 서버에서만 실행
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority()) return;

	// 1층 안전지대로 강제 이동
	FVector RespawnLoc = FVector(0, 0, 43200.0f);
	if (FloorManager)
	{
		RespawnLoc = FloorManager->GetRandomSafeFloorLocation();

		FloorManager->ModifyTeamLife(1);
	}

	// 위치 이동 및 물리력 초기화
	OwnerCharacter->GetCharacterMovement()->Velocity = FVector::ZeroVector;
	OwnerCharacter->SetActorLocation(RespawnLoc);

	// 상태 복구
	ChangeState(EFallState::Normal);
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

void UPlayerFallComponent::OnHangTimerExpired()
{
	if (CurrentState != EFallState::Hanging) return;

	UE_LOG(LogTemp, Warning, TEXT("Stamina Depleted! Dropping..."));

	PerformWallDrop();
}

void UPlayerFallComponent::Server_LetGo_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("Server received LetGo request! Dropping player immediately."));

	PerformWallDrop();
}

void UPlayerFallComponent::Server_ClimbUpSelf_Implementation()
{
	TryRescue(nullptr);
}

void UPlayerFallComponent::TryRescue(AActor* RescuerActor)
{
	if (CurrentState != EFallState::Hanging) return;
	if (!OwnerCharacter) return;

	// 안전 위치 계산
	FVector MyLoc = OwnerCharacter->GetActorLocation();
	FVector MyForward = OwnerCharacter->GetActorForwardVector();
	FVector MyUp = FVector::UpVector;
	TargetSafeLocation = MyLoc + (MyForward * 130.0f) + (MyUp * 190.0f);
	TargetSafeRotation = OwnerCharacter->GetActorRotation();

	// 상태 변경
	ChangeState(EFallState::Climbing);

	// 구조자가 있다면 애니메이션 지시
	ACharacter* RescuerChar = Cast<ACharacter>(RescuerActor);
	if (RescuerChar)
	{
		float VictimDuration = (ClimbUpMontage) ? ClimbUpMontage->GetPlayLength() : 1.5f;
		Multicast_StartRescuerAction(RescuerChar, VictimDuration);
	}
}

void UPlayerFallComponent::ClimbUpSelf()
{
	Server_ClimbUpSelf();
}

void UPlayerFallComponent::LetGo()
{
	if (CurrentState != EFallState::Hanging) return;

	// 내가 서버면 바로 실행하고, 클라라면 서버에게 요청
	if (OwnerCharacter->HasAuthority())
	{
		PerformWallDrop();
	}
	else
	{
		Server_LetGo();
	}
}

void UPlayerFallComponent::Input_MashF()
{
	if (CurrentState != EFallState::Hanging) return;

	float MyZ = OwnerCharacter->GetActorLocation().Z;
	float Floor2_Threshold = 46000.0f;

	if (FloorManager)
	{
		Floor2_Threshold = (FloorManager->Floor1_Height + FloorManager->Floor2_Height) * 0.5f;
	}
	// 1층이라면 F키 입력 무시
	if (MyZ < Floor2_Threshold)	return;

	// 게이지 증가
	CurrentClimbGauge = FMath::Clamp(CurrentClimbGauge + GaugeIncreasePerPress, 0.0f, MaxClimbGauge);

	// UI 갱신 알림
	OnClimbGaugeChanged.Broadcast(CurrentClimbGauge / MaxClimbGauge);

	UE_LOG(LogTemp, Log, TEXT("Climb Gauge: %f"), CurrentClimbGauge);

	// 게이지 꽉 찼는지 확인
	if (CurrentClimbGauge >= MaxClimbGauge)
	{
		// 자력 등반 실행
		ClimbUpSelf();

		// 게이지 초기화
		CurrentClimbGauge = 0.0f;
		OnClimbGaugeChanged.Broadcast(0.0f);
	}
}
