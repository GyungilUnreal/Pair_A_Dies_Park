#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Slider.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ProgressBar.h"
#include "GameSettingsSubsystem.h"
#include "OptionsMenuWidget.generated.h"

UCLASS()
class UOptionsMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;

public:
    // ---------- BindWidget: UMG에서 자동 연결될 위젯들 ----------

    /** 마스터 볼륨 슬라이더 (0.0 ~ 1.0) */
    UPROPERTY(meta = (BindWidget))
    USlider* MasterVolumeSlider;

    /** BGM 볼륨 슬라이더 (0.0 ~ 1.0) */
    UPROPERTY(meta = (BindWidget))
    USlider* BGMVolumeSlider;

    /** SFX 볼륨 슬라이더 (0.0 ~ 1.0) */
    UPROPERTY(meta = (BindWidget))
    USlider* SFXVolumeSlider;

    /** 해상도 선택 콤보 (예: "1920x1080") */
    UPROPERTY(meta = (BindWidget))
    UComboBoxString* ResolutionComboBox;

    /** 창 모드 콤보 (예: "Fullscreen", "Windowed", "Borderless") */
    UPROPERTY(meta = (BindWidget))
    UComboBoxString* WindowModeComboBox;

    /** 그래픽 품질 콤보 (Low / Medium / High / Epic) */
    UPROPERTY(meta = (BindWidget))
    UComboBoxString* GraphicsQualityComboBox;

    UPROPERTY(meta = (BindWidget))
    USlider* MouseSensitivitySlider;

    UPROPERTY(meta = (BindWidget))
    UCheckBox* InvertYCheckBox;

    /** 적용 버튼 */
    UPROPERTY(meta = (BindWidget))
    UButton* ApplyButton;

    /** 기본값 버튼 */
    UPROPERTY(meta = (BindWidget, Optional))
    UButton* ResetButton;

    /** 취소/닫기 버튼 (선택) */
    UPROPERTY(meta = (BindWidget, Optional))
    UButton* CancelButton;

    UPROPERTY(meta = (BindWidget, Optional))
    UProgressBar* MouseSensitivityProgressBar;

    UPROPERTY(meta = (BindWidget, Optional))
    UProgressBar* MasterVolumeProgressBar;

    UPROPERTY(meta = (BindWidget, Optional))
    UProgressBar* BGMVolumeProgressBar;

    UPROPERTY(meta = (BindWidget, Optional))
    UProgressBar* SFXVolumeProgressBar;

protected:
    // ---------- 내부 참조 ----------

    /** 설정 서브시스템 */
    UPROPERTY()
    UGameSettingsSubsystem* SettingsSubsystem;

    /** CurrentSettings 캐시 */
    UPROPERTY()
    UGameSettingsSave* Settings;

    /** 해상도 목록 (콤보 인덱스 ↔ 실제 해상도 매핑) */
    TArray<FIntPoint> ResolutionOptions;

protected:
    // ---------- 초기화/갱신 로직 ----------

    /** Subsystem 및 Settings 포인터 가져오기 */
    void InitSubsystemAndSettings();

    /** 해상도/창 모드 콤보 옵션 채우기 */
    void SetupComboBoxOptions();

    /** SaveGame 값을 UI에 반영 */
    void RefreshUIFromSettings();

    /** UI 이벤트 바인딩 */
    void BindUIEvents();

    // ---------- UI 이벤트 핸들러 ----------

    UFUNCTION()
    void OnMasterVolumeChanged(float Value);

    UFUNCTION()
    void OnBGMVolumeChanged(float Value);

    UFUNCTION()
    void OnSFXVolumeChanged(float Value);

    UFUNCTION()
    void OnResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnGraphicsQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnMouseSensitivityChanged(float Value);

    UFUNCTION()
    void OnInvertYChanged(bool bIsChecked);

    UFUNCTION()
    void OnApplyClicked();

    UFUNCTION()
    void OnResetClicked();

    UFUNCTION()
    void OnCancelClicked();

    // ---------- 헬퍼 ----------

    int32 FindResolutionIndexFromSettings() const;
    int32 FindWindowModeIndexFromSettings() const;
    int32 FindGraphicsQualityIndexFromSettings() const;
};
