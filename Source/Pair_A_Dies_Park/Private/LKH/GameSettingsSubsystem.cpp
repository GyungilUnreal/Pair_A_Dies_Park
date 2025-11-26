#include "GameSettingsSubsystem.h"
#include "Split_character.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"

UGameSettingsSubsystem::UGameSettingsSubsystem()
{
    static ConstructorHelpers::FObjectFinder<USoundClass> MasterSC(
        TEXT("/Game/Collaborators/LKH/Split/SFX/MasterSoundClass.MasterSoundClass"));
    if (MasterSC.Succeeded())
    {
        MasterSoundClass = MasterSC.Object;
    }

    static ConstructorHelpers::FObjectFinder<USoundClass> BGMSC(
        TEXT("/Game/Collaborators/LKH/Split/SFX/BGMSoundClass.BGMSoundClass"));
    if (BGMSC.Succeeded())
    {
        BGMSoundClass = BGMSC.Object;
    }

    static ConstructorHelpers::FObjectFinder<USoundClass> SFXSC(
        TEXT("/Game/Collaborators/LKH/Split/SFX/SFXSoundClass.SFXSoundClass"));
    if (SFXSC.Succeeded())
    {
        SFXSoundClass = SFXSC.Object;
    }

    static ConstructorHelpers::FObjectFinder<USoundMix> MainMix(
        TEXT("/Game/Audio/Mixes/MainSoundMix.MainSoundMix"));
    if (MainMix.Succeeded())
    {
        MainSoundMix = MainMix.Object;
    }
}

void UGameSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    LoadOrCreateSettings();

    // 게임 시작 시 한 번 적용
    ApplySettings();
}

void UGameSettingsSubsystem::Deinitialize()
{
    // 게임 끝날 때 한 번 저장해두고 싶다면
    if (CurrentSettings)
    {
        SaveSettings();
    }

    Super::Deinitialize();
}

void UGameSettingsSubsystem::LoadOrCreateSettings()
{
    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
    {
        USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0);
        CurrentSettings = Cast<UGameSettingsSave>(LoadedGame);
    }

    if (!CurrentSettings)
    {
        CurrentSettings = Cast<UGameSettingsSave>(
            UGameplayStatics::CreateSaveGameObject(UGameSettingsSave::StaticClass()));

        // 첫 생성 시 바로 저장해두기
        SaveSettings();
    }
}

void UGameSettingsSubsystem::SaveSettings()
{
    if (!CurrentSettings)
    {
        UE_LOG(LogTemp, Warning, TEXT("GameSettingsSubsystem::SaveSettings - CurrentSettings is null"));
        return;
    }

    const bool bSuccess = UGameplayStatics::SaveGameToSlot(CurrentSettings, SaveSlotName, 0);
    if (!bSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to save game settings to slot %s"), *SaveSlotName);
    }
}

void UGameSettingsSubsystem::ApplySettings()
{
    if (!CurrentSettings)
    {
        UE_LOG(LogTemp, Warning, TEXT("GameSettingsSubsystem::ApplySettings - CurrentSettings is null"));
        return;
    }

    ApplyGraphicsSettings();
    ApplyAudioSettings();
    ApplyGameplaySettings();
}

void UGameSettingsSubsystem::ResetToDefaults()
{
    if (!CurrentSettings)
    {
        LoadOrCreateSettings();
    }

    // SaveGame 기본 생성자와 동일하게 맞춰줌 (필요하면 따로 Static 함수로 분리해도 됨)
    CurrentSettings->ResolutionX = 1920;
    CurrentSettings->ResolutionY = 1080;
    CurrentSettings->WindowMode = EWindowMode::Fullscreen;
    CurrentSettings->GraphicsQuality = 2;

    CurrentSettings->MasterVolume = 1.0f;
    CurrentSettings->BGMVolume = 1.0f;
    CurrentSettings->SFXVolume = 1.0f;

    CurrentSettings->MouseSensitivity = 1.0f;
    CurrentSettings->bInvertY = false;

    ApplySettings();
    SaveSettings();
}

// ------------------ Graphics ------------------

void UGameSettingsSubsystem::ApplyGraphicsSettings()
{
    if (!GEngine)
    {
        return;
    }

    UGameUserSettings* UserSettings = GEngine->GetGameUserSettings();
    if (!UserSettings || !CurrentSettings)
    {
        return;
    }

    // 해상도 및 창 모드
    FIntPoint Resolution(CurrentSettings->ResolutionX, CurrentSettings->ResolutionY);
    UserSettings->SetScreenResolution(Resolution);
    UserSettings->SetFullscreenMode(CurrentSettings->WindowMode);

    // 품질 (엔진에 따라 세분화 가능)
    // 0~3 사이 값으로 가정
    int32 Quality = FMath::Clamp(CurrentSettings->GraphicsQuality, 0, 3);
    UserSettings->SetOverallScalabilityLevel(Quality);

    // 즉시 적용 + 저장
    UserSettings->ApplySettings(false); // bCheckForCommandLineOverrides = false
    UserSettings->SaveSettings();
}

// ------------------ Audio ------------------

void UGameSettingsSubsystem::ApplyAudioSettings()
{
    if (!CurrentSettings)
    {
        return;
    }

    // 사운드 믹스/클래스 기반으로 적용하는 가장 일반적인 패턴
    if (!MainSoundMix)
    {
        // 믹스 없이 SoundClass Volume만 직접 바꿀 수도 있음 (단, 프로젝트에 따라 다름)
        if (MasterSoundClass)
        {
            MasterSoundClass->Properties.Volume = CurrentSettings->MasterVolume;
        }
        if (BGMSoundClass)
        {
            BGMSoundClass->Properties.Volume = CurrentSettings->BGMVolume;
        }
        if (SFXSoundClass)
        {
            SFXSoundClass->Properties.Volume = CurrentSettings->SFXVolume;
        }

        return;
    }

    // 믹스를 통해 볼륨 오버라이드
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // 마스터
    if (MasterSoundClass)
    {
        UGameplayStatics::SetSoundMixClassOverride(
            World,
            MainSoundMix,
            MasterSoundClass,
            CurrentSettings->MasterVolume,
            1.0f,   // Pitch
            0.0f,   // FadeInTime
            true    // bApplyToChildren
        );
    }

    // BGM
    if (BGMSoundClass)
    {
        UGameplayStatics::SetSoundMixClassOverride(
            World,
            MainSoundMix,
            BGMSoundClass,
            CurrentSettings->BGMVolume,
            1.0f,
            0.0f,
            true
        );
    }

    // SFX
    if (SFXSoundClass)
    {
        UGameplayStatics::SetSoundMixClassOverride(
            World,
            MainSoundMix,
            SFXSoundClass,
            CurrentSettings->SFXVolume,
            1.0f,
            0.0f,
            true
        );
    }

    // 변경 사항 즉시 반영
    UGameplayStatics::PushSoundMixModifier(World, MainSoundMix);
}

// ------------------ Gameplay ------------------

void UGameSettingsSubsystem::ApplyGameplaySettings()
{
    if (!CurrentSettings)
    {
        return;
    }

    // 이 부분은 프로젝트마다 입력 처리 방식이 달라서
    // 예시로 "PlayerController에 값 전달" 정도만 적어둘게요.

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        APawn* Pawn = PC->GetPawn();
        if (ASplit_Character* MyChar = Cast<ASplit_Character>(Pawn))
        {
            MyChar->MouseSensitivity = CurrentSettings->MouseSensitivity;
            MyChar->bInvertY = CurrentSettings->bInvertY;
        }
    }

    // 예) PlayerController에 IGameSettingsListener 같은 인터페이스를 만들어서 넘겨주는 방식 등
    // 여기서는 단순한 Pseudo 코드로 처리:

    /*
    if (auto SettingsInterface = Cast<IGameSettingsListener>(PC))
    {
        SettingsInterface->OnMouseSettingsChanged(CurrentSettings->MouseSensitivity, CurrentSettings->bInvertY);
    }
    */

    // 또는 PlayerController에 직접 멤버 변수/함수 만들어놓고 캐스팅해서 세팅해도 됨
}
