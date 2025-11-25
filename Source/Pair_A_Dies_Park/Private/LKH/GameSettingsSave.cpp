#include "GameSettingsSave.h"

UGameSettingsSave::UGameSettingsSave()
{
    // 기본값 세팅
    ResolutionX = 1920;
    ResolutionY = 1080;
    WindowMode = EWindowMode::Fullscreen;
    GraphicsQuality = 2;      // High

    MasterVolume = 1.0f;
    BGMVolume = 1.0f;
    SFXVolume = 1.0f;

    MouseSensitivity = 1.0f;
    bInvertY = false;
}
