#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Engine/EngineTypes.h"
#include "GameSettingsSave.generated.h"

UCLASS()
class UGameSettingsSave : public USaveGame
{
    GENERATED_BODY()

public:
    UGameSettingsSave();

    /** Graphics */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Graphics")
    int32 ResolutionX;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Graphics")
    int32 ResolutionY;

    /** Window mode (Fullscreen / Windowed / Borderless) */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Graphics")
    TEnumAsByte<EWindowMode::Type> WindowMode;

    /** 0 = Low, 1 = Medium, 2 = High, 3 = Epic */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Graphics")
    int32 GraphicsQuality;

    /** Audio */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio")
    float MasterVolume;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio")
    float BGMVolume;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Audio")
    float SFXVolume;

    /** Gameplay */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Gameplay")
    float MouseSensitivity;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Gameplay")
    bool bInvertY;
};
