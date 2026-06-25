// MainGameMode.cpp


#include "GameMode/MainGameMode.h"

#include "Checkout/CheckoutManager.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogMainGameMode, Log, All);

AMainGameMode::AMainGameMode()
{
    GameStateClass = AMainGameState::StaticClass();

    if (CartPawnClass)
    {
        DefaultPawnClass = CartPawnClass;
    }

    bUseSeamlessTravel = true;

    // 라운드 전체 시간 초기화(기본값 설정 추후 수정 가능)
    PhaseScheduleMap.Add(0.f,   ERoundPhase::RoundStart);
    PhaseScheduleMap.Add(30.f,  ERoundPhase::RandomOpenTwo);
    PhaseScheduleMap.Add(60.f,  ERoundPhase::SaleEvent);
    PhaseScheduleMap.Add(90.f,  ERoundPhase::PremiumRespawn);
    PhaseScheduleMap.Add(150.f, ERoundPhase::FinalWarningOneOpen);
    PhaseScheduleMap.Add(180.f, ERoundPhase::RoundEnd);

    CheckoutManagerRef = nullptr;
}

void AMainGameMode::HandleMatchHasStarted()
{
    Super::HandleMatchHasStarted();

    UE_LOG(LogMainGameMode, Warning, TEXT("=== HandleMatchHasStarted: 매치가 시작되었습니다! ==="));
    StartRound();
}

// 라운드 시작 지점
void AMainGameMode::StartRound()
{
    UE_LOG(LogMainGameMode, Warning, TEXT("=== StartRound ==="));
    AMainGameState* GS = GetGameState<AMainGameState>();
    if (!GS)
    {
        return;
    }

    // 계산대 매니저 찾기
    AActor* FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), ACheckoutManager::StaticClass());
    if (FoundActor)
    {
        CheckoutManagerRef = Cast<ACheckoutManager>(FoundActor);
        UE_LOG(LogMainGameMode, Log, TEXT("CheckoutManager를 성공적으로 찾았습니다."));
    }
    else
    {
        UE_LOG(LogMainGameMode, Error, TEXT("월드에서 CheckoutManager를 찾을 수 없습니다!"));
    }


    // 맵 내부의 키(시간)을 오름차순으로 정리
    PhaseScheduleMap.GetKeys(SortedTriggerTimes);
    SortedTriggerTimes.Sort();

    NextPhaseIndex = 0;

    GS->SetRoundStartTime();

    // 첫 Phase 즉시 진입
    EnterPhase(PhaseScheduleMap[SortedTriggerTimes[0]]);
    NextPhaseIndex = 1;

    // 0.25초 마다 Phase 확인
    GetWorldTimerManager().SetTimer(Timer_RoundTick, this, &AMainGameMode::TickRoundSchedule, TickInterval, true);
}

//지금 다음 Phase로 넘어가야하는지 체크
void AMainGameMode::TickRoundSchedule()
{

    // 모든 Phase가 끝났는지 확인
    if (!SortedTriggerTimes.IsValidIndex(NextPhaseIndex))
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
    const float NextTriggerTime = SortedTriggerTimes[NextPhaseIndex];

    if (Elapsed >= NextTriggerTime)
    {
        ERoundPhase* TargetPhase = PhaseScheduleMap.Find(NextTriggerTime);
        if (TargetPhase)
        {
            UE_LOG(LogMainGameMode, Warning, TEXT("[Phase 트리거됨] 경과 시간: %.2f초 >= 목표 시간: %.2f초"), Elapsed, NextTriggerTime);
            EnterPhase(*TargetPhase);
        }
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

    FString PhaseName = TEXT("Unknown");

    switch (NewPhase)
    {
    case ERoundPhase::RoundStart:
        // 라운드 시작 / 계산대 3개 모두 오픈
        PhaseName = TEXT("RoundStart (계산대 3개 모두 오픈)");

        if (CheckoutManagerRef)
        {
            CheckoutManagerRef->PrepareNextCheckoutZoneState(3);
        }

        break;

    case ERoundPhase::RandomOpenTwo:
        // 랜덤 오픈 시작 / 계산대 3개중 2개 오픈
        PhaseName = TEXT("RandomOpenTwo (계산대 3개 중 2개 오픈)");

        if (CheckoutManagerRef)
        {
            CheckoutManagerRef->PrepareNextCheckoutZoneState(2);
        }
        break;

    case ERoundPhase::SaleEvent:
        // 세일 상품 이벤트 시작
        PhaseName = TEXT("SaleEvent (세일 상품 이벤트 시작)");
        break;

    case ERoundPhase::PremiumRespawn:
        // 중앙 고급 상품 스폰
        PhaseName = TEXT("PremiumRespawn (중앙 고급 상품 스폰)");
        break;

    case ERoundPhase::FinalWarningOneOpen:
        // 마지막 30초 경고 및 계산대 1게만 오픈
        PhaseName = TEXT("FinalWarningOneOpen (마지막 30초 경고 및 계산대 1개만 오픈)");
        break;

    case ERoundPhase::RoundEnd:
        // 라운드 종료 계산대 전부 닫음
        // 결과화면 표시 및 점수 집계
        PhaseName = TEXT("RoundEnd (라운드 종료 계산대 전부 닫음)");
        GetWorldTimerManager().ClearTimer(Timer_RoundTick);
        break;

    default:
        break;
    }

    UE_LOG(LogMainGameMode, Warning, TEXT("[PHASE CHANGE] 새로운 페이즈 진입: %s"), *PhaseName);
}
