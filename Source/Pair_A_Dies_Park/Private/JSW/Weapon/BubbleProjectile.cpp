// Fill out your copyright notice in the Description page of Project Settings.


#include "JSW/Weapon/BubbleProjectile.h"
#include "JSW/BossCharacter.h"
#include "JSW/FloorManager.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"

// Sets default values
ABubbleProjectile::ABubbleProjectile()
{
	bReplicates = true;
	SetReplicateMovement(true);

	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
	NetPriority = 3.0f;
	bAlwaysRelevant = true;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	CollisionComp->InitSphereRadius(60.0f);
	CollisionComp->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionComp->SetNotifyRigidBodyCollision(true);
	CollisionComp->SetSimulatePhysics(false);

	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CollisionComp->SetNotifyRigidBodyCollision(true);

	RootComponent = CollisionComp;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(CollisionComp);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionComp);
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->InitialSpeed = 200.0f;
}

void ABubbleProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABubbleProjectile, CurrentBubbleType);
	DOREPLIFETIME(ABubbleProjectile, bIsIce);
}

// Called when the game starts or when spawned
void ABubbleProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (MeshComp) DynamicMat = MeshComp->CreateAndSetMaterialInstanceDynamic(0);

	CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (HasAuthority())
	{
		int32 Rand = FMath::RandRange(0, 100);
		if (Rand < 60) CurrentBubbleType = EBubbleType::Normal;
		else if (Rand < 80) CurrentBubbleType = EBubbleType::Fire;
		else if (Rand < 90) CurrentBubbleType = EBubbleType::Ice;
		else CurrentBubbleType = EBubbleType::Lightning;

		OnRep_BubbleType();
	}

	FTimerHandle CollisionTimer;
	GetWorld()->GetTimerManager().SetTimer(CollisionTimer, [this]()
	{
		if (IsValid(this) && CollisionComp && !bIsIce)
		{
			CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);

			CollisionComp->SetCollisionResponseToAllChannels(ECR_Block);
			CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
			CollisionComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

			CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

			CollisionComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		}

	}, 2.0f, false);

	CollisionComp->OnComponentHit.AddDynamic(this, &ABubbleProjectile::OnCompHit);
}
void ABubbleProjectile::SetVelocity(FVector NewVelocity)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = NewVelocity;
	}
}

float ABubbleProjectile::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	UE_LOG(LogTemp, Error, TEXT(">>> Bubble Took Damage! Authority: %d <<<"), HasAuthority());

	if (!bIsIce && HasAuthority())
	{
		bIsIce = true;
		OnRep_IsIce();

		ProjectileMovement->Deactivate();

		CollisionComp->SetSimulatePhysics(true);
		CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		CollisionComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
		CollisionComp->AddImpulse(FVector(0, 0, -1000.0f), NAME_None, true);
	}
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void ABubbleProjectile::OnCompHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this || OtherActor == GetInstigator() || OtherActor == GetOwner())
	{
		return;
	}

	if (!OtherActor || !bIsIce) return;

	if (!HasAuthority())
	{
		return;
	}

	if (ABossCharacter* Boss = Cast<ABossCharacter>(OtherActor))
	{
		UE_LOG(LogTemp, Warning, TEXT(">>> HIT BOSS! BubbleType: %d <<<"), (int32)CurrentBubbleType);

		float Dmg = 20.0f;
		switch (CurrentBubbleType)
		{
		case EBubbleType::Fire:
			Boss->ApplyPuzzleDamage(Dmg * 3.0f);
			UE_LOG(LogTemp, Warning, TEXT("   [Fire] Critical Damage Applied!"));
			break;

		case EBubbleType::Ice:
			Boss->ApplyPuzzleDamage(Dmg);
			// TODO: Boss->ApplySlowDebuff(5.0f); 
			UE_LOG(LogTemp, Warning, TEXT("   [Ice] Slow Effect Applied!"));
			break;

		case EBubbleType::Lightning:
			Boss->ApplyPuzzleDamage(Dmg);
			if (HasAuthority())
			{
				if (AFloorManager* FM = Cast<AFloorManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AFloorManager::StaticClass())))
				{
					FM->ModifyTeamLife(-1);
					UE_LOG(LogTemp, Warning, TEXT("   [Lightning] Team Life Recovered!"));
				}
			}
			break;

		default:
			Boss->ApplyPuzzleDamage(Dmg);
			UE_LOG(LogTemp, Warning, TEXT("   [Normal] Damage Applied."));
			break;
		}

		Destroy();
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Hit Ground/Wall: %s -> Destroying Bubble"), *OtherActor->GetName());
		Destroy();
	}
}

void ABubbleProjectile::OnRep_BubbleType() { UpdateVisuals(); }

void ABubbleProjectile::OnRep_IsIce() { UpdateVisuals(); }

void ABubbleProjectile::UpdateVisuals()
{
	// 색깔 바꾸기 (머티리얼에 'Color' 파라미터 있어야 함)
	if (DynamicMat && TypeColors.Contains(CurrentBubbleType))
	{
		DynamicMat->SetVectorParameterValue(FName("Color"), TypeColors[CurrentBubbleType]);
	}
	// 메쉬 바꾸기
	if (bIsIce && IceMeshAsset) MeshComp->SetStaticMesh(IceMeshAsset);
	else if (!bIsIce && BubbleMeshAsset) MeshComp->SetStaticMesh(BubbleMeshAsset);
}