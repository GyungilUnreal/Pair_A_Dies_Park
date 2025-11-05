// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialPlatform.generated.h"

UCLASS()
class ATutorialPlatform : public AActor
{
	GENERATED_BODY()
	
public:
	ATutorialPlatform();

	// 블루프린트에서도 구현 가능.
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void ActivatePlatform();
	virtual void ActivatePlatform_Implementation(); // C++ 구현.

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void DeactivatePlatform();
	virtual void DeactivatePlatform_Implementation(); // C++ 구현.

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlatformMesh;

	FVector OriginalLocation;

	virtual void BeginPlay() override;

};
