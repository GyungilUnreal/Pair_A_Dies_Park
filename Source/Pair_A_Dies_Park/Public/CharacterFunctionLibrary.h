#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameFramework/Character.h"
#include "CharacterFunctionLibrary.generated.h"

UCLASS()
class UCharacterFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "CharacterUtils")
    static bool IsMovingOnGround(ACharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "CharacterUtils")
    static void SetWalkSpeed(ACharacter* Character, float NewSpeed);

    UFUNCTION(BlueprintCallable, Category = "CharacterUtils")
    static void SetPlayerScale(ACharacter* Character, float PlayerScale);

    UFUNCTION(BlueprintCallable, Category = "CharacterUtils")
    static void SetCameraDistance(ACharacter* Character, float CameraDistance);
};
