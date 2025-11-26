#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameSettingsSave.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "GameSettingsSubsystem.generated.h"

UCLASS()
class UGameSettingsSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UGameSettingsSubsystem();

    // Subsystem 라이프사이클
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** 현재 사용중인 설정 SaveGame */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    UGameSettingsSave* GetSettings() const { return CurrentSettings; }

    /** 설정 저장 (SaveGameToSlot) */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void SaveSettings();

    /** 설정을 실제 엔진 시스템(해상도/그래픽/사운드 등)에 적용 */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void ApplySettings();

    /** 기본값으로 리셋하고, 바로 적용 + 저장 */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    void ResetToDefaults();

protected:
    /** SaveGame을 로드하거나 없으면 생성 */
    void LoadOrCreateSettings();

    /** 그래픽 설정 적용 */
    void ApplyGraphicsSettings();

    /** 오디오 설정 적용 */
    void ApplyAudioSettings();

    /** 게임플레이(마우스, 인버트 등) 적용 - 프로젝트마다 구현 방식 다름 */
    void ApplyGameplaySettings();

protected:
    /** 현재 세션에서 들고 있을 SaveGame 객체 */
    UPROPERTY()
    UGameSettingsSave* CurrentSettings;

    /** SaveGame 슬롯 이름 */
    UPROPERTY()
    FString SaveSlotName = TEXT("GameSettings");

    // ---- Audio 관련 참조 (에디터에서 세팅) ----
    /** 전체 볼륨용 사운드 클래스 */
    UPROPERTY(EditAnywhere, Category = "Audio")
    USoundClass* MasterSoundClass;

    /** BGM 볼륨용 사운드 클래스 */
    UPROPERTY(EditAnywhere, Category = "Audio")
    USoundClass* BGMSoundClass;

    /** SFX 볼륨용 사운드 클래스 */
    UPROPERTY(EditAnywhere, Category = "Audio")
    USoundClass* SFXSoundClass;

    /** 볼륨 적용에 사용할 사운드 믹스 (선택사항) */
    UPROPERTY(EditAnywhere, Category = "Audio")
    USoundMix* MainSoundMix;
};
