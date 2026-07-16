// MainPlayerState.cpp


#include "PlayerState/MainPlayerState.h"

#include "GameMode/MainGameMode.h"
#include "Net/UnrealNetwork.h"

class AMainGameMode;

AMainPlayerState::AMainPlayerState()
{
    bReplicates = true;

    PlayerScore = 0;
    Rank = 0;
    CheckoutCount = 0;
    BumpCartCount = 0;
    DroppedItemCount = 0;
    Title = ETitleType::Default;
}

void AMainPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMainPlayerState, PlayerScore);
    DOREPLIFETIME(AMainPlayerState, Rank);
    DOREPLIFETIME(AMainPlayerState, CheckoutCount);
    DOREPLIFETIME(AMainPlayerState, BumpCartCount);
    DOREPLIFETIME(AMainPlayerState, DroppedItemCount);
    DOREPLIFETIME(AMainPlayerState, Title);
}

// 점수 관련
void AMainPlayerState::AddPlayerScore(int32 AddScore)
{
    if (!HasAuthority()) return;

#if !UE_BUILD_DEBUG
    UE_LOG(LogTemp, Warning, TEXT("[MainPlayerState] AddPlayerScore (서버) - 대상 플레이어: %s, 기존 점수: %d, 추가될 점수: %d"),
        *GetPlayerName(), PlayerScore, AddScore);
#endif

    PlayerScore += AddScore;

    OnPlayerStatsChanged.Broadcast();
    OnPlayerScoreChanged.Broadcast();
    ForceNetUpdate();

    //등수 계산
    if (AMainGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AMainGameMode>() : nullptr)
    {
        GM->UpdateAllPlayerRanks();
    }
}

int32 AMainPlayerState::GetPlayerScore() const
{
    return PlayerScore;
}

void AMainPlayerState::OnRep_PlayerScore()
{
    OnPlayerStatsChanged.Broadcast();
    OnPlayerScoreChanged.Broadcast();
}

// 순위 관련
void AMainPlayerState::SetRank(int32 NewRank)
{
 if (!HasAuthority()) return;

    if (Rank == NewRank)
    {
        return;
    }

    Rank = NewRank;

    OnPlayerStatsChanged.Broadcast();
}

int32 AMainPlayerState::GetRank() const
{
    return Rank;
}

void AMainPlayerState::OnRep_Rank()
{
    OnPlayerStatsChanged.Broadcast();
}


//계산대 사용 횟수 관련
void AMainPlayerState::AddCheckoutCount(int32 Count)
{
    if (!HasAuthority()) return;

    CheckoutCount += Count;

    OnPlayerStatsChanged.Broadcast();
}

int32 AMainPlayerState::GetCheckoutCount() const
{
    return CheckoutCount;
}

void AMainPlayerState::OnRep_CheckoutCount()
{
    OnPlayerStatsChanged.Broadcast();
}

//카트 충돌 횟수 관련
void AMainPlayerState::AddCartBumpCount(int32 Count)
{
    if (!HasAuthority()) return;

    BumpCartCount += Count;

    OnPlayerStatsChanged.Broadcast();
}

int32 AMainPlayerState::GetCartBumpCount() const
{
    return BumpCartCount;
}

void AMainPlayerState::OnRep_CartBumpCount()
{
    OnPlayerStatsChanged.Broadcast();
}

//상품 낙하 횟수 관련
void AMainPlayerState::AddDroppedItemCount(int32 Count)
{
    if (!HasAuthority()) return;

    DroppedItemCount += Count;

    OnPlayerStatsChanged.Broadcast();
}

int32 AMainPlayerState::GetDroppedItemCount() const
{
    return DroppedItemCount;
}

void AMainPlayerState::OnRep_DroppedItemCount()
{
    OnPlayerStatsChanged.Broadcast();
}

// 칭호 부여 판단 로직
void AMainPlayerState::SetTitle(ETitleType NewTitle)
{
    if (!HasAuthority()) return;

    if (Title == NewTitle) return;

    Title = NewTitle;

    OnPlayerStatsChanged.Broadcast();
}

ETitleType AMainPlayerState::GetTitle() const
{
    return Title;
}

void AMainPlayerState::OnRep_Title()
{
    OnPlayerStatsChanged.Broadcast();
}

//모든 통계 초기화
void AMainPlayerState::ResetStats()
{
    if (!HasAuthority()) return;

    PlayerScore = 0;
    Rank = 0;
    CheckoutCount = 0;
    BumpCartCount = 0;
    DroppedItemCount = 0;
    Title = ETitleType::Default;

    OnPlayerStatsChanged.Broadcast();
    OnPlayerScoreChanged.Broadcast();
    ForceNetUpdate();
}
