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
void AMainPlayerState::AddPlayerScore(float AddScore)
{
    if (!HasAuthority()) return;


    UE_LOG(LogTemp, Warning, TEXT("[MainPlayerState] AddPlayerScore (서버) - 대상 플레이어: %s, 기존 점수: %f, 추가될 점수: %f"),
        *GetPlayerName(), PlayerScore, AddScore);

    PlayerScore += AddScore;

    OnPlayerStatsChanged.Broadcast();

    //등수 계산
    if (AMainGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AMainGameMode>() : nullptr)
    {
        GM->UpdateAllPlayerRanks();
    }
}

float AMainPlayerState::GetPlayerScore() const
{
    return PlayerScore;
}

void AMainPlayerState::OnRep_PlayerScore()
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

// 칭호 부여 판단 로직
void AMainPlayerState::SetTitle()
{
    if (!HasAuthority()) return;

    ETitleType NewTitle = ETitleType::Default;

  if (Rank == 1)
  {
      NewTitle = ETitleType::MartKing;
  }
  else if (BumpCartCount >= 15)
  {
    NewTitle = ETitleType::BumpKing;
  }
  else if (DroppedItemCount >= 8)
  {
      NewTitle = ETitleType::DestroyKing;
  }
  else if (CheckoutCount >= 4)
  {
      NewTitle = ETitleType::ReceiptCollector;
  }
  else if (BumpCartCount <= 2 && DroppedItemCount <= 2 && CheckoutCount >= 1)
  {
      NewTitle = ETitleType::SafeCart;
  }

    if (Title == NewTitle) return;

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
}
