#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyPlayersWidget.generated.h"

class UVerticalBox;
class UPlayerRowWidget;

UCLASS()
class ULobbyPlayersWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void RebuildPlayerList();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UPlayerRowWidget> PlayerRowClass;

    UPROPERTY(meta = (BindWidget)) UVerticalBox* VerticalBox_List;

private:
    FTimerHandle RefreshTimerHandle;
    int32 LastPlayerCount = -1;

    void TickRefresh(); // PlayerArray°¡ ¹Ù²î¸é Rebuild
};