#include "OptionsMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"

void UOptionsMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    InitSubsystemAndSettings();
    SetupComboBoxOptions();
    RefreshUIFromSettings();
    BindUIEvents();
}

// ---------- 초기화 ----------

void UOptionsMenuWidget::InitSubsystemAndSettings()
{
    if (!GetWorld())
    {
        return;
    }

    UGameInstance* GI = GetWorld()->GetGameInstance();
    if (!GI)
    {
        return;
    }

    SettingsSubsystem = GI->GetSubsystem<UGameSettingsSubsystem>();
    if (!SettingsSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("OptionsMenuWidget: GameSettingsSubsystem not found!"));
        return;
    }

    Settings = SettingsSubsystem->GetSettings();
    if (!Settings)
    {
        UE_LOG(LogTemp, Warning, TEXT("OptionsMenuWidget: Settings is null!"));
    }
}

void UOptionsMenuWidget::SetupComboBoxOptions()
{
    if (!ResolutionComboBox || !WindowModeComboBox)
    {
        return;
    }

    // --- 해상도 목록 구성 ---
    ResolutionComboBox->ClearOptions();
    ResolutionOptions.Empty();

    // 간단하게 몇 개만 넣는 예시 (필요하면 GameUserSettings에서 지원 해상도 가져와도 됨)
    ResolutionOptions.Add(FIntPoint(1280, 720));
    ResolutionOptions.Add(FIntPoint(1920, 1080));
    ResolutionOptions.Add(FIntPoint(2560, 1440));
    ResolutionOptions.Add(FIntPoint(3840, 2160));

    for (const FIntPoint& Res : ResolutionOptions)
    {
        const FString Label = FString::Printf(TEXT("%dx%d"), Res.X, Res.Y);
        ResolutionComboBox->AddOption(Label);
    }

    // --- 창 모드 목록 구성 ---
    WindowModeComboBox->ClearOptions();
    WindowModeComboBox->AddOption(TEXT("Fullscreen"));
    WindowModeComboBox->AddOption(TEXT("Windowed"));
    WindowModeComboBox->AddOption(TEXT("Borderless"));
}

void UOptionsMenuWidget::RefreshUIFromSettings()
{
    if (!Settings)
    {
        return;
    }

    // 슬라이더 값
    if (MasterVolumeSlider)
    {
        MasterVolumeSlider->SetValue(Settings->MasterVolume);
    }
    if (BGMVolumeSlider)
    {
        BGMVolumeSlider->SetValue(Settings->BGMVolume);
    }
    if (SFXVolumeSlider)
    {
        SFXVolumeSlider->SetValue(Settings->SFXVolume);
    }

    // 해상도 콤보
    if (ResolutionComboBox)
    {
        const int32 Index = FindResolutionIndexFromSettings();
        if (Index != INDEX_NONE && ResolutionOptions.IsValidIndex(Index))
        {
            const FIntPoint Res = ResolutionOptions[Index];
            const FString Label = FString::Printf(TEXT("%dx%d"), Res.X, Res.Y);
            ResolutionComboBox->SetSelectedOption(Label);
        }
    }

    // 창 모드 콤보
    if (WindowModeComboBox)
    {
        const int32 Index = FindWindowModeIndexFromSettings();
        if (Index != INDEX_NONE && Index < WindowModeComboBox->GetOptionCount())
        {
            const FString Label = WindowModeComboBox->GetOptionAtIndex(Index);
            WindowModeComboBox->SetSelectedOption(Label);
        }
    }
}

void UOptionsMenuWidget::BindUIEvents()
{
    if (MasterVolumeSlider)
    {
        MasterVolumeSlider->OnValueChanged.AddDynamic(
            this, &UOptionsMenuWidget::OnMasterVolumeChanged);
    }

    if (BGMVolumeSlider)
    {
        BGMVolumeSlider->OnValueChanged.AddDynamic(
            this, &UOptionsMenuWidget::OnBGMVolumeChanged);
    }

    if (SFXVolumeSlider)
    {
        SFXVolumeSlider->OnValueChanged.AddDynamic(
            this, &UOptionsMenuWidget::OnSFXVolumeChanged);
    }

    if (ResolutionComboBox)
    {
        ResolutionComboBox->OnSelectionChanged.AddDynamic(
            this, &UOptionsMenuWidget::OnResolutionChanged);
    }

    if (WindowModeComboBox)
    {
        WindowModeComboBox->OnSelectionChanged.AddDynamic(
            this, &UOptionsMenuWidget::OnWindowModeChanged);
    }

    if (ApplyButton)
    {
        ApplyButton->OnClicked.AddDynamic(
            this, &UOptionsMenuWidget::OnApplyClicked);
    }

    if (ResetButton)
    {
        ResetButton->OnClicked.AddDynamic(
            this, &UOptionsMenuWidget::OnResetClicked);
    }

    if (CancelButton)
    {
        CancelButton->OnClicked.AddDynamic(
            this, &UOptionsMenuWidget::OnCancelClicked);
    }
}

// ---------- UI 이벤트 ----------

void UOptionsMenuWidget::OnMasterVolumeChanged(float Value)
{
    if (Settings)
    {
        Settings->MasterVolume = Value;
    }
}

void UOptionsMenuWidget::OnBGMVolumeChanged(float Value)
{
    if (Settings)
    {
        Settings->BGMVolume = Value;
    }
}

void UOptionsMenuWidget::OnSFXVolumeChanged(float Value)
{
    if (Settings)
    {
        Settings->SFXVolume = Value;
    }
}

void UOptionsMenuWidget::OnResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (!Settings)
    {
        return;
    }

    int32 X = 0, Y = 0;

    {
        FString Left, Right;
        if (SelectedItem.Split(TEXT("x"), &Left, &Right))
        {
            X = FCString::Atoi(*Left);
            Y = FCString::Atoi(*Right);
        }
    }

    if (X > 0 && Y > 0)
    {
        Settings->ResolutionX = X;
        Settings->ResolutionY = Y;
    }
}

void UOptionsMenuWidget::OnWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (!Settings)
    {
        return;
    }

    if (SelectedItem == TEXT("Fullscreen"))
    {
        Settings->WindowMode = EWindowMode::Fullscreen;
    }
    else if (SelectedItem == TEXT("Windowed"))
    {
        Settings->WindowMode = EWindowMode::Windowed;
    }
    else if (SelectedItem == TEXT("Borderless"))
    {
        Settings->WindowMode = EWindowMode::WindowedFullscreen;
    }
}

void UOptionsMenuWidget::OnApplyClicked()
{
    if (!SettingsSubsystem || !Settings)
    {
        return;
    }

    // 여기까지는 Settings 객체 안에 값만 반영된 상태.
    // 실제 엔진(해상도/볼륨 등)에 적용 + SaveGame 저장
    SettingsSubsystem->ApplySettings();
    SettingsSubsystem->SaveSettings();

    // 필요하면 여기서 위젯 닫기(BP에서 Override 가능)
    // 예: RemoveFromParent();
}

void UOptionsMenuWidget::OnResetClicked()
{
    if (!SettingsSubsystem)
    {
        return;
    }

    SettingsSubsystem->ResetToDefaults();

    // Reset 후 Settings 값이 변경되었으므로 UI 다시 갱신
    Settings = SettingsSubsystem->GetSettings();
    RefreshUIFromSettings();
}

void UOptionsMenuWidget::OnCancelClicked()
{
    // 변경사항 무시하고 UI 닫고 싶으면 여기서 처리
    // 단, 슬라이더/콤보 변경이 이미 Settings에 반영된 상태라면
    // 따로 "복사본"을 두는 구조로 바꿀 필요도 있음.

    // 간단하게: SaveGame 다시 로드 -> UI 갱신 정도로도 가능
    if (SettingsSubsystem)
    {
        // 이런 함수가 있다면:
        // SettingsSubsystem->ReloadSettingsFromDisk();

        // 일단 CurrentSettings 값으로 다시 UI 덮어쓰기
        Settings = SettingsSubsystem->GetSettings();
        RefreshUIFromSettings();
    }

    // 위젯 닫기
    RemoveFromParent();
}

// ---------- 헬퍼 ----------

int32 UOptionsMenuWidget::FindResolutionIndexFromSettings() const
{
    if (!Settings)
    {
        return INDEX_NONE;
    }

    for (int32 i = 0; i < ResolutionOptions.Num(); ++i)
    {
        if (ResolutionOptions[i].X == Settings->ResolutionX &&
            ResolutionOptions[i].Y == Settings->ResolutionY)
        {
            return i;
        }
    }

    return INDEX_NONE;
}

int32 UOptionsMenuWidget::FindWindowModeIndexFromSettings() const
{
    if (!Settings)
    {
        return INDEX_NONE;
    }

    switch (Settings->WindowMode)
    {
    case EWindowMode::Fullscreen:
        return 0; // "Fullscreen"
    case EWindowMode::Windowed:
        return 1; // "Windowed"
    case EWindowMode::WindowedFullscreen:
        return 2; // "Borderless"
    default:
        return 0;
    }
}