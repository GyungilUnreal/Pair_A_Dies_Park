// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "BubbleProjectile.generated.h"

UENUM(BlueprintType)
enum class EBubbleType : uint8
{
	Normal,
	Fire,
	Ice,
	Lightning
};

UCLASS()
class ABubbleProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABubbleProjectile();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	void SetVelocity(FVector NewVelocity);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(ReplicatedUsing = OnRep_BubbleType)
	EBubbleType CurrentBubbleType = EBubbleType::Normal;

	UPROPERTY(ReplicatedUsing = OnRep_IsIce)
	bool bIsIce = false;

	UFUNCTION()
	void OnRep_BubbleType();

	UFUNCTION()
	void OnRep_IsIce();
	
	void UpdateVisuals();

	UFUNCTION()
	void OnCompHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	UStaticMesh* IceMeshAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	UStaticMesh* BubbleMeshAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	TMap<EBubbleType, FLinearColor> TypeColors;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USphereComponent* CollisionComp;
};
