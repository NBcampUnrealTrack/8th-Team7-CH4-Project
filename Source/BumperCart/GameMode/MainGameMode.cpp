// MainGameMode.cpp


#include "GameMode/MainGameMode.h"

AMainGameMode::AMainGameMode()
{
    GameStateClass = AMainGameState::StaticClass();
    bUseSeamlessTravel = true;
}

void AMainGameMode::HandleMatchHasStarted()
{
    Super::HandleMatchHasStarted();

    if (CartPawnClass)
    {
        DefaultPawnClass = CartPawnClass;
    }

    StartRound();
}

// 라운드 시작 지점
void AMainGameMode::StartRound()
{
    AMainGameState* GS = GetGameState<AMainGameState>();
    if (!GS)
    {
        return;
    }

    // 라운드 전체 시간표
    PhaseSchedule = {
        { 0.f,   ERoundPhase::RoundStart },
        { 30.f,  ERoundPhase::RandomOpenTwo },
        { 60.f,  ERoundPhase::SaleEvent },
        { 90.f,  ERoundPhase::PremiumRespawn },
        { 150.f, ERoundPhase::FinalWarningOneOpen },
        { 180.f, ERoundPhase::RoundEnd },
    };

    NextPhaseIndex = 0;

    GS->SetRoundStartTime();

    // 첫 Phase 즉시 진입
    EnterPhase(PhaseSchedule[0].Phase);
    NextPhaseIndex = 1;

    // 0.25초 마다 Phase 확인
    GetWorldTimerManager().SetTimer(Timer_RoundTick, this, &AMainGameMode::TickRoundSchedule, TickInterval, true);
}

//지금 다음 Phase로 넘어가야하는지 체크
void AMainGameMode::TickRoundSchedule()
{
    // 모든 Phase가 끝났는지 확인
    if (!PhaseSchedule.IsValidIndex(NextPhaseIndex))
    {
        // 모든 Phase를 다 진입했으면 타이머 종료
        GetWorldTimerManager().ClearTimer(Timer_RoundTick);
        return;
    }

    AMainGameState* GS = GetGameState<AMainGameState>();
    if (!GS)
    {
        return;
    }

    const float Elapsed = GS->RoundDurationSeconds - GS->GetRemainingTime();
    const FRoundPhaseSchedule& NextSchedule = PhaseSchedule[NextPhaseIndex];

    if (Elapsed >= NextSchedule.TriggerTimeSeconds)
    {
        EnterPhase(NextSchedule.Phase);
        ++NextPhaseIndex;
    }
}


//Phase 진입
void AMainGameMode::EnterPhase(ERoundPhase NewPhase)
{
    AMainGameState* GS = GetGameState<AMainGameState>();
    if (!GS)
    {
        return;
    }

    GS->SetRoundPhase(NewPhase);

    switch (NewPhase)
    {
    case ERoundPhase::RoundStart:
        // 라운드 시작 / 계산대 3개 모두 오픈
        break;

    case ERoundPhase::RandomOpenTwo:
        // 랜덤 오픈 시작 / 계산대 3개중 2개 오픈
        break;

    case ERoundPhase::SaleEvent:
        // 세일 상품 이벤트 시작
        break;

    case ERoundPhase::PremiumRespawn:
        // 중앙 고급 상품 스폰
        break;

    case ERoundPhase::FinalWarningOneOpen:
        // 마지막 30초 경고 및 계산대 1게만 오픈
        break;

    case ERoundPhase::RoundEnd:
        // 라운드 종료 계산대 전부 닫음
        // 결과화면 표시 및 점수 집계
        GetWorldTimerManager().ClearTimer(Timer_RoundTick);
        break;

    default:
        break;
    }
}
