#include "SteamPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/Texture2D.h"
#include "OnlineSubsystem.h"

// Steam API (OnlineSubsystemSteam 플러그인 활성화 필요)
#include "steam/steam_api.h"

ASteamPlayerState::ASteamPlayerState()
{
    bReplicates = true;
}

void ASteamPlayerState::BeginPlay()
{
    Super::BeginPlay();

    // 로컬 플레이어면 자기 SteamID를 SteamId64에 세팅(서버로 복제)
    CacheMySteamIdIfLocal();

    // 이미 SteamId64가 있거나(리슨서버/싱글), 나중에 Rep 되면 OnRep에서 처리
    if (!SteamId64.IsEmpty())
    {
        RequestSteamAvatar();
    }
}

void ASteamPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ASteamPlayerState, SteamId64);
}

void ASteamPlayerState::OnRep_SteamId64()
{
    // 원격 플레이어의 SteamId64가 도착하면 그때 아바타 로드
    RequestSteamAvatar();
}

void ASteamPlayerState::CacheMySteamIdIfLocal()
{
    // 소유한(로컬) PlayerState에서만 내 SteamID를 박아주면 됨
    if (!IsOwnedBy(GetWorld()->GetFirstPlayerController()))
        return;

    if (SteamAPI_IsSteamRunning() && SteamUser())
    {
        const CSteamID MyId = SteamUser()->GetSteamID();
        SteamId64 = FString::Printf(TEXT("%llu"), (uint64)MyId.ConvertToUint64());
    }
}

void ASteamPlayerState::RequestSteamAvatar()
{
    // 이미 로드했으면 바로 브로드캐스트
    if (SteamAvatarTexture)
    {
        OnSteamAvatarReady.Broadcast(SteamAvatarTexture);
        return;
    }

    TryLoadSteamAvatarInternal();
}

void ASteamPlayerState::TryLoadSteamAvatarInternal()
{
    if (SteamId64.IsEmpty())
        return;

    if (!SteamAPI_IsSteamRunning() || !SteamFriends() || !SteamUtils())
        return;

    const uint64 Id64 = FCString::Strtoui64(*SteamId64, nullptr, 10);
    const CSteamID TargetId((uint64)Id64);

    // 큰 아바타 요청 (반환값: image handle, -1이면 아직 준비 안됨)
    const int ImageHandle = SteamFriends()->GetLargeFriendAvatar(TargetId);
    if (ImageHandle == -1)
    {
        // 아직 다운로드/생성 중 → 잠깐 뒤 재시도
        if (!RetryTimerHandle.IsValid())
        {
            GetWorldTimerManager().SetTimer(
                RetryTimerHandle,
                this,
                &ASteamPlayerState::TryLoadSteamAvatarInternal,
                0.2f,
                true
            );
        }
        return;
    }

    // 실패(0) or 성공(>0)
    if (ImageHandle <= 0)
        return;

    // 더 이상 재시도 필요 없음
    if (RetryTimerHandle.IsValid())
    {
        GetWorldTimerManager().ClearTimer(RetryTimerHandle);
        RetryTimerHandle.Invalidate();
    }

    uint32 Width = 0, Height = 0;
    if (!SteamUtils()->GetImageSize(ImageHandle, &Width, &Height) || Width == 0 || Height == 0)
        return;

    TArray<uint8> RGBA;
    RGBA.SetNumUninitialized((int32)(Width * Height * 4));

    if (!SteamUtils()->GetImageRGBA(ImageHandle, RGBA.GetData(), RGBA.Num()))
        return;

    SteamAvatarTexture = CreateTextureFromSteamRGBA(RGBA, (int32)Width, (int32)Height);
    OnSteamAvatarReady.Broadcast(SteamAvatarTexture);
}

UTexture2D* ASteamPlayerState::CreateTextureFromSteamRGBA(const TArray<uint8>& RGBA, int32 Width, int32 Height)
{
    // UE Texture는 보통 BGRA가 기본이라(플랫폼에 따라) RGBA->BGRA 스왑해주는 게 안전
    TArray<uint8> BGRA;
    BGRA.SetNumUninitialized(RGBA.Num());

    for (int32 i = 0; i < RGBA.Num(); i += 4)
    {
        BGRA[i + 0] = RGBA[i + 2]; // B
        BGRA[i + 1] = RGBA[i + 1]; // G
        BGRA[i + 2] = RGBA[i + 0]; // R
        BGRA[i + 3] = RGBA[i + 3]; // A
    }

    UTexture2D* Tex = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
    if (!Tex) return nullptr;

#if WITH_EDITORONLY_DATA
    Tex->MipGenSettings = TMGS_NoMipmaps;
#endif
    Tex->NeverStream = true;

    void* TextureData = Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
    FMemory::Memcpy(TextureData, BGRA.GetData(), BGRA.Num());
    Tex->GetPlatformData()->Mips[0].BulkData.Unlock();

    Tex->UpdateResource();
    return Tex;
}