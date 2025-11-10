#include "CharacterFunctionLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Split_Character.h"
#include "GameFramework/SpringArmComponent.h"

bool UCharacterFunctionLibrary::IsMovingOnGround(ACharacter* Character)
{
    return Character && Character->GetCharacterMovement()->IsMovingOnGround();
}

void UCharacterFunctionLibrary::SetWalkSpeed(ACharacter* Character, float NewSpeed)
{
    if (Character && Character->GetCharacterMovement())
    {
        Character->GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
    }
}

void UCharacterFunctionLibrary::SetPlayerScale(ACharacter* Character, float PlayerScale)
{
    Character->GetRootComponent()->SetWorldScale3D(FVector(PlayerScale, PlayerScale, PlayerScale));
}

void UCharacterFunctionLibrary::SetCameraDistance(ACharacter* Character, float CameraDistance)
{
    TObjectPtr<ASplit_Character> _player = Cast<ASplit_Character>(Character);
    if (_player)
    {
        _player->GetCameraBoom()->TargetArmLength = CameraDistance;
    }
}