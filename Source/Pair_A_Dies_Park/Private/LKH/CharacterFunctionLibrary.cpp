#include "CharacterFunctionLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"

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
    UE_LOG(LogTemp, Warning, TEXT("Player scale : [&f]"), PlayerScale);
    Character->GetRootComponent()->SetWorldScale3D(FVector(PlayerScale, PlayerScale, PlayerScale));
}