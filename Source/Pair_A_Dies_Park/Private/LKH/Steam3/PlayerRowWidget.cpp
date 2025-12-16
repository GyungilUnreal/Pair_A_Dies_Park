#include "PlayerRowWidget.h"
#include "SteamPlayerState.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UPlayerRowWidget::BindPlayerState(ASteamPlayerState* InPS)
{
    if (!InPS) return;

    // 이전 바인딩 해제(중요: 리빌드/재바인딩 시 중복 방지)
    if (BoundPS)
    {
        BoundPS->OnSteamAvatarReady.RemoveAll(this);
    }

    BoundPS = InPS;

    if (Text_Name)
    {
        Text_Name->SetText(FText::FromString(BoundPS->GetPlayerName()));
    }

    // 아바타 이벤트 바인딩
    BoundPS->OnSteamAvatarReady.AddDynamic(this, &UPlayerRowWidget::HandleAvatarReady);

    // 즉시 요청(이미 로드됐으면 바로 브로드캐스트되도록 PlayerState에서 처리해둔 상태)
    BoundPS->RequestSteamAvatar();
}

void UPlayerRowWidget::HandleAvatarReady(UTexture2D* Avatar)
{
    if (Image_Avatar && Avatar)
    {
        Image_Avatar->SetBrushFromTexture(Avatar, true);
    }
}