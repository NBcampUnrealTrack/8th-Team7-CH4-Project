// MainGameMode.cpp


#include "GameMode/MainGameMode.h"

#include "Checkout/CheckoutManager.h"
#include "Kismet/GameplayStatics.h"
#include "ItemSpawn/ProductShelfManager/ProductShelfManager.h"
#include "EventManager/BC_EventManager.h"
#include "MapGimmickManager/MapGimmickManager.h"
#include "PlayerState/MainPlayerState.h"

class AMainPlayerState;
DEFINE_LOG_CATEGORY_STATIC(LogMainGameMode, Log, All);

AMainGameMode::AMainGameMode()
{
    GameStateClass = AMainGameState::StaticClass();
    PlayerStateClass = AMainPlayerState::StaticClass();



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

void AMainGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (GetWorld())
    {
        // 맵에 배치된 이벤트 매니저 수색 및 캐스팅
        AActor* FoundEventActor = UGameplayStatics::GetActorOfClass(GetWorld(), ABC_EventManager::StaticClass());
        BC_EventManager = Cast<ABC_EventManager>(FoundEventActor);

        // 맵에 배치된 맵 기믹 매니저 수색 및 캐스팅
        AActor* FoundGimmickActor = UGameplayStatics::GetActorOfClass(GetWorld(), AMapGimmickManager::StaticClass());
        MapGimmickManager = Cast<AMapGimmickManager>(FoundGimmickActor);

        // 맵에 배치된 제품 선반 매니저 수색 및 캐스팅
        AActor* FoundShelfActor = UGameplayStatics::GetActorOfClass(GetWorld(), AProductShelfManager::StaticClass());
        ProductShelfManager = Cast<AProductShelfManager>(FoundShelfActor);
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

    //매니저 배치 확인
    if (!BC_EventManager) UE_LOG(LogTemp, Error, TEXT("[GameMode] 맵에 BC_EventManager가 배치되지 않았습니다"));
    if (!MapGimmickManager) UE_LOG(LogTemp, Error, TEXT("[GameMode] 맵에 MapGimmickManager가 배치되지 않았습니다"));
    if (!ProductShelfManager) UE_LOG(LogTemp, Error, TEXT("[GameMode] 맵에 ProductShelfManager가 배치되지 않았습니다"));

    if(BC_EventManager && MapGimmickManager && ProductShelfManager)
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode] 모든 매니저가 성공적으로 등록되었습니다."));
    }
}

void AMainGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    if (CartPawnClass)
    {
        DefaultPawnClass = CartPawnClass;
        UE_LOG(LogMainGameMode, Warning, TEXT("카트 설정"));
    }
    else
    {
        UE_LOG(LogMainGameMode, Warning, TEXT("설정된 카트가 존재하지 않습니다."));
    }

    Super::InitGame(MapName, Options, ErrorMessage);
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

        if (BC_EventManager)
        {
            BC_EventManager->StartSaleEvent();
        }

        break;

    case ERoundPhase::PremiumRespawn:
        // 중앙 고급 상품 스폰
        PhaseName = TEXT("PremiumRespawn (중앙 고급 상품 스폰)");

        if (BC_EventManager)
        {
            BC_EventManager->StartLimitedEvent();
        }
        break;

    case ERoundPhase::FinalWarningOneOpen:
        // 마지막 30초 경고 및 계산대 1게만 오픈
        PhaseName = TEXT("FinalWarningOneOpen (마지막 30초 경고 및 계산대 1개만 오픈)");
        if (CheckoutManagerRef)
        {
            CheckoutManagerRef->PrepareNextCheckoutZoneState(1);
        }
        break;

    case ERoundPhase::RoundEnd:
        // 라운드 종료 계산대 전부 닫음
        // 결과화면 표시 및 점수 집계
        PhaseName = TEXT("RoundEnd (라운드 종료 계산대 전부 닫음)");
        if (CheckoutManagerRef)
        {
            CheckoutManagerRef->PrepareNextCheckoutZoneState(0);
        }

        GetWorldTimerManager().ClearTimer(Timer_RoundTick);

        UpdateAllPlayerRanks();

        // 결과 화면 노출 시간 이후 로비로 복귀
        GetWorldTimerManager().SetTimer(Timer_ReturnToLobby, this, &AMainGameMode::ReturnAllPlayersToLobby, ResultScreenDuration, false);
        break;

    default:
        break;
    }

    UE_LOG(LogMainGameMode, Warning, TEXT("[PHASE CHANGE] 새로운 페이즈 진입: %s"), *PhaseName);
}


//모든 플레이어 랭크 점수에 맞춰 정렬
void AMainGameMode::UpdateAllPlayerRanks()
{
    AMainGameState* GS = GetGameState<AMainGameState>();
    if (!GS) return;

    TArray<AMainPlayerState*> SortedStates;
    for (APlayerState* PS : GS->PlayerArray)
    {
        if (AMainPlayerState* MPS = Cast<AMainPlayerState>(PS))
        {
            SortedStates.Add(MPS);
        }
    }

    if (SortedStates.Num() == 0) return;


    //점수 내림차순 정렬
    SortedStates.Sort([](const AMainPlayerState& A, const AMainPlayerState& B)
    {
        return A.GetPlayerScore() > B.GetPlayerScore();
    });

    //동점자는 같은 등수
    int32 CurrentRank = 1;
    for (int32 Index = 0; Index < SortedStates.Num(); ++Index)
    {
        // 바로 앞 사람과 점수가 같으면 같은 등수 부여 / 처음에는 확인 X
        if (Index > 0 && SortedStates[Index]->GetPlayerScore() == SortedStates[Index - 1]->GetPlayerScore())
        {
            SortedStates[Index]->SetRank(SortedStates[Index - 1]->GetRank());
        }
        else
        {
            SortedStates[Index]->SetRank(CurrentRank);
        }

        ++CurrentRank;
    }

    //1등 명단 추출
    TArray<FString> WinnerNames;
    for (AMainPlayerState* MPS : SortedStates)
    {
        if (MPS->GetRank() == 1)
        {
            WinnerNames.Add(MPS->GetPlayerName());
        }
    }

    // 1등 명단 MainGameState에 저장
    GS->SetFinalWinners(WinnerNames);
}

// 라운드 종료 시 모든 플레이어 로비로 복귀
void AMainGameMode::ReturnAllPlayersToLobby()
{
    UE_LOG(LogMainGameMode, Warning, TEXT("결과 화면 종료, 로비로 복귀"));
    GetWorld()->ServerTravel(TEXT("/Game/Developers/LSJae/Levels/TestLobbyLevel"));
}
