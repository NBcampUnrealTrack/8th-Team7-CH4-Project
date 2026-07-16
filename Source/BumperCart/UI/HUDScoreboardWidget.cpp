// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUDScoreboardWidget.h"
#include "HUDScoreboardRowWidget.h"
#include "PlayerState/MainPlayerState.h"
#include "GameState/MainGameState.h"
#include "Components/PanelWidget.h"


void UHUDScoreboardWidget::NativeConstruct()
{
    Super::NativeConstruct();

    Rows = { Row_1, Row_2, Row_3, Row_4 };

    for (int32 i = 0; i < Rows.Num(); ++i)
    {
        UHUDScoreboardRowWidget* Row = Rows[i];
        if (!IsValid(Row)) continue;

        Row->SetBaseIndex(i);
        Row->SetVisibility(ESlateVisibility::HitTestInvisible);
        Row->SetRenderOpacity(0.f);
        Row->SetRenderTranslation(FVector2D::ZeroVector);
    }

    ViewportResizedHandle = FViewport::ViewportResizedEvent.AddUObject(this, &ThisClass::HandleViewportResized);

    TryCacheGameState();
    QueueCacheRowLayout();
}

void UHUDScoreboardWidget::NativeDestruct()
{
    StopMoveAnimation();

    if (IsValid(CachedGameState))
    {
        CachedGameState->OnMainPlayerStateAdded.RemoveDynamic(this, &ThisClass::HandlePlayerStateAdded);
        CachedGameState->OnMainPlayerStateRemoved.RemoveDynamic(this, &ThisClass::HandlePlayerStateRemoved);
    }

    for (const auto& Pair : PlayerRows)
    {
        if (AMainPlayerState* PS = Pair.Key.Get())
        {
            PS->OnPlayerScoreChanged.RemoveDynamic(this, &ThisClass::HandlePlayerScoreChanged);
        }
    }

    if (ViewportResizedHandle.IsValid())
    {
        FViewport::ViewportResizedEvent.Remove(ViewportResizedHandle);
        ViewportResizedHandle.Reset();
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(GameStateCacheTimerHandle);
        World->GetTimerManager().ClearTimer(LayoutCacheTimerHandle);
        World->GetTimerManager().ClearTimer(MoveTimerHandle);
    }

    CachedGameState = nullptr;
    PlayerRows.Empty();
    Rows.Empty();
    SlotPositionsY.Empty();

    bLayoutReady = false;
    bLayoutCacheQueued = false;
    bHasInitialized = false;;
    bRefreshQueued = false;

    Super::NativeDestruct();
}

void UHUDScoreboardWidget::HandlePlayerScoreChanged()
{
    QueueRefresh();
}

void UHUDScoreboardWidget::HandlePlayerStateAdded(AMainPlayerState* PlayerState)
{
    AddPlayerRow(PlayerState);
}

void UHUDScoreboardWidget::HandlePlayerStateRemoved(AMainPlayerState* PlayerState)
{
    RemovePlayerRow(PlayerState);
}

void UHUDScoreboardWidget::TryCacheGameState()
{
    if (IsValid(CachedGameState)) return;

    UWorld* World = GetWorld();
    if (!IsValid(World)) return;

    AMainGameState* GameState = World->GetGameState<AMainGameState>();

    if (!IsValid(GameState))
    {
        if (!World->GetTimerManager().IsTimerActive(GameStateCacheTimerHandle))
        {
            World->GetTimerManager().SetTimer(
                GameStateCacheTimerHandle,
                this,
                &ThisClass::TryCacheGameState,
                0.1f,
                true);
        }
        return;
    }

    // GameState를 캐싱 했다면 아래 처리

    World->GetTimerManager().ClearTimer(GameStateCacheTimerHandle);

    CachedGameState = GameState;

    CachedGameState->OnMainPlayerStateAdded.AddUniqueDynamic(this, &ThisClass::HandlePlayerStateAdded);
    CachedGameState->OnMainPlayerStateRemoved.AddUniqueDynamic(this, &ThisClass::HandlePlayerStateRemoved);

    // 위젯 생성 전에 이미 추가된 PlayerState 처리
    SyncPlayerStates();
}

void UHUDScoreboardWidget::SyncPlayerStates()
{
    if (!IsValid(CachedGameState)) return;

    TSet<AMainPlayerState*> ActivePlayers;

    // 유효한 플레이어만 저장
    for (APlayerState* PlayerState : CachedGameState->PlayerArray)
    {
        if (AMainPlayerState* PS = Cast<AMainPlayerState>(PlayerState))
        {
            ActivePlayers.Add(PS);
            AddPlayerRow(PS);
        }
    }

    TArray<TObjectPtr<AMainPlayerState>> PlayersToRemove;

    // 유효하지 않은 플레이어는 제거할 명단에 등록
    for (const auto& Pair : PlayerRows)
    {
        AMainPlayerState* PS = Pair.Key.Get();

        if (!IsValid(PS) || !ActivePlayers.Contains(PS))
        {
            PlayersToRemove.Add(Pair.Key);
        }
    }

    // 명단에 있는 플레이어 제거
    for (AMainPlayerState* PS : PlayersToRemove)
    {
        RemovePlayerRow(PS);
    }
}

void UHUDScoreboardWidget::AddPlayerRow(AMainPlayerState* PlayerState)
{
    if (!IsValid(PlayerState) || PlayerRows.Contains(PlayerState)) return;

    UHUDScoreboardRowWidget* Row = FindAvailableRow();
    if (!IsValid(Row)) return;

    PlayerState->OnPlayerScoreChanged.AddUniqueDynamic(this, &ThisClass::HandlePlayerScoreChanged);

    PlayerRows.Add(PlayerState, Row);
    Row->SetVisibility(ESlateVisibility::HitTestInvisible);
    Row->SetRenderOpacity(1.f);

    QueueRefresh();
}

void UHUDScoreboardWidget::RemovePlayerRow(AMainPlayerState* PlayerState)
{
    if (!IsValid(PlayerState)) return;

    PlayerState->OnPlayerScoreChanged.RemoveDynamic(this, &ThisClass::HandlePlayerScoreChanged);

    UHUDScoreboardRowWidget* Row = PlayerRows.FindRef(PlayerState);
    PlayerRows.Remove(PlayerState);

    if (IsValid(Row))
    {
        Row->PrepareMove(0.f);
        Row->SnapToTarget();
        Row->SetRenderOpacity(0.f);
    }

    QueueRefresh();
} 

UHUDScoreboardRowWidget* UHUDScoreboardWidget::FindAvailableRow() const
{
    for (UHUDScoreboardRowWidget* Row : Rows)
    {
        if (!IsValid(Row)) continue;

        bool bUsed = false;

        for (const auto& Pair : PlayerRows)
        {
            if (Pair.Value.Get() == Row)
            {
                bUsed = true;
                break;
            }
        }

        if (!bUsed)
        {
            return Row;
        }
    }

    return nullptr;
}

void UHUDScoreboardWidget::QueueRefresh()
{
    if (bRefreshQueued || !GetWorld()) return;

    bRefreshQueued = true;
    GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::RefreshScoreboard);
}

void UHUDScoreboardWidget::RefreshScoreboard()
{
    bRefreshQueued = false;

    TArray<AMainPlayerState*> SortedPlayers;
    for (const auto& Pair : PlayerRows)
    {
        if (IsValid(Pair.Key))
        {
            SortedPlayers.Add(Pair.Key);
        }
    }

    SortedPlayers.Sort([](const AMainPlayerState& A, const AMainPlayerState& B)
        {
            if (A.GetPlayerScore() == B.GetPlayerScore())
            {
                return A.GetPlayerId() < B.GetPlayerId();
            }
            return A.GetPlayerScore() > B.GetPlayerScore();
        }
    );

    int32 DisplayRank = 1;

    for (int32 i = 0; i < SortedPlayers.Num(); ++i)
    {
        AMainPlayerState* PS = SortedPlayers[i];
        UHUDScoreboardRowWidget* Row = PlayerRows.FindRef(PS);

        if (!IsValid(PS) || !IsValid(Row)) continue;

        // 동점자 아니면 i+1 등수 부여
        if (i > 0 && PS->GetPlayerScore() != SortedPlayers[i - 1]->GetPlayerScore())
        {
            DisplayRank = i + 1;
        }

        Row->UpdateRow(PS, DisplayRank);
        Row->SetRenderOpacity(1.f);
    }

    // 위치 계산은 레이아웃 준비 이후 처리
    if (!bLayoutReady)
    {
        QueueCacheRowLayout();
        return;
    }

    StopMoveAnimation();

    bool bRowMoved = false;

    for (int32 i = 0; i < SortedPlayers.Num(); ++i)
    {
        AMainPlayerState* PS = SortedPlayers[i];
        UHUDScoreboardRowWidget* Row = PlayerRows.FindRef(PS);

        if (!IsValid(Row)) continue;

        int32 BaseIndex = Row->GetBaseIndex();

        if (!SlotPositionsY.IsValidIndex(i) || !SlotPositionsY.IsValidIndex(BaseIndex)) continue;

        const float TargetY = SlotPositionsY[i] - SlotPositionsY[BaseIndex];

        bRowMoved |= Row->PrepareMove(TargetY);
    }

    if (!bHasInitialized || MoveDuration <= KINDA_SMALL_NUMBER)
    {
        for (const auto& Pair : PlayerRows)
        {
            if (UHUDScoreboardRowWidget* Row = Pair.Value.Get())
            {
                Row->SnapToTarget();
            }
        }

        bHasInitialized = true;
        return;
    }

    if (bRowMoved)
    {
        StartMoveAnimation();
    }
}

void UHUDScoreboardWidget::QueueCacheRowLayout()
{
    if (bLayoutCacheQueued || !GetWorld()) return;

    bLayoutCacheQueued = true;
    LayoutCacheTimerHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::CacheRowLayout);
}

void UHUDScoreboardWidget::CacheRowLayout()
{
    bLayoutCacheQueued = false;
    bLayoutReady = false;

    StopMoveAnimation();

    for (UHUDScoreboardRowWidget* Row : Rows)
    {
        if (IsValid(Row))
        {
            Row->SetRenderTranslation(FVector2D::ZeroVector);
        }
    }

    ForceLayoutPrepass();

    if (!IsValid(Vb_Root)) return;

    FGeometry ContainerGeometry = Vb_Root->GetCachedGeometry();
    if (ContainerGeometry.GetLocalSize().IsNearlyZero())
    {
        QueueCacheRowLayout();
        return;
    }

    SlotPositionsY.Init(0.f, Rows.Num());

    for (int32 i = 0; i < Rows.Num(); ++i)
    {
        UHUDScoreboardRowWidget* Row = Rows[i];
        if (!IsValid(Row) || Row->GetCachedGeometry().GetLocalSize().IsNearlyZero())
        {
            QueueCacheRowLayout();
            return;
        }

        FVector2D AbsolutePosition = Row->GetCachedGeometry().GetAbsolutePosition();
        FVector2D LocalPosition = ContainerGeometry.AbsoluteToLocal(AbsolutePosition);

        SlotPositionsY[i] = LocalPosition.Y;
    }

    bLayoutReady = true;

    // 해상도 변경되면 새 위치로 맞추기 위함
    bHasInitialized = false;

    RefreshScoreboard();
}

void UHUDScoreboardWidget::HandleViewportResized(FViewport* Viewport, uint32 Unused)
{
    bLayoutReady = false;
    QueueCacheRowLayout();
}

void UHUDScoreboardWidget::StartMoveAnimation()
{
    UWorld* World = GetWorld();
    if (!IsValid(World) || MoveDuration <= KINDA_SMALL_NUMBER) return;

    MoveStartTime = World->GetTimeSeconds();

    World->GetTimerManager().SetTimer(
        MoveTimerHandle,
        this,
        &ThisClass::UpdateMoveAnimation,
        MoveInterval,
        true);
}

void UHUDScoreboardWidget::UpdateMoveAnimation()
{
    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        StopMoveAnimation();
        return;
    }

    double ElapsedTime = World->GetTimeSeconds() - MoveStartTime;
    float Alpha = FMath::Clamp(static_cast<float>(ElapsedTime / MoveDuration), 0.f, 1.f);
    float InterpAlpha = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);

    for (const auto& Pair : PlayerRows)
    {
        if (UHUDScoreboardRowWidget* Row = Pair.Value.Get())
        {
            Row->ApplyMove(InterpAlpha);
        }
    }

    if (Alpha >= 1.f)
    {
        for (const auto& Pair : PlayerRows)
        {
            if (UHUDScoreboardRowWidget* Row = Pair.Value.Get())
            {
                Row->SnapToTarget();
            }
        }

        StopMoveAnimation();
    }
}

void UHUDScoreboardWidget::StopMoveAnimation()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MoveTimerHandle);
    }

    MoveTimerHandle.Invalidate();
}
