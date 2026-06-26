// MainPlayerState.cpp


#include "PlayerState/MainPlayerState.h"
#include "Net/UnrealNetwork.h"

AMainPlayerState::AMainPlayerState()
{
    bReplicates = true;
}

void AMainPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMainPlayerState, Score);
    DOREPLIFETIME(AMainPlayerState, Rank);
    DOREPLIFETIME(AMainPlayerState, CheckoutCount);
    DOREPLIFETIME(AMainPlayerState, BumpCartCount);
    DOREPLIFETIME(AMainPlayerState, DroppedItemCount);
}

// 점수 관련
void AMainPlayerState::AddScore(float AddScore)
{
    if (!HasAuthority()) return;

    Score += AddScore;

    OnPlayerStatsChanged.Broadcast();
}

float AMainPlayerState::GetScore() const
{
    return Score;
}

void AMainPlayerState::OnRep_Score()
{
    OnPlayerStatsChanged.Broadcast();
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

//모든 통계 초기화
void AMainPlayerState::ResetStats()
{
    if (!HasAuthority()) return;

    Score = 0;
    Rank = 0;
    CheckoutCount = 0;
    BumpCartCount = 0;
    DroppedItemCount = 0;

    OnPlayerStatsChanged.Broadcast();
}
