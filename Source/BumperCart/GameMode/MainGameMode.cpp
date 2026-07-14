// MainGameMode.cpp


#include "GameMode/MainGameMode.h"

#include "Checkout/CheckoutManager.h"
#include "DataAsset/CharacterSelectionConfig.h"
#include "Kismet/GameplayStatics.h"
#include "ProductShelfSubsystem/ProductShelfSubsystem.h"
#include "EventManager/BC_EventSubsystem.h"
#include "GameInstance/MainGameInstance.h"
#include "MapGimmickManager/MapGimmickManager.h"
#include "PlayerState/MainPlayerState.h"
#include "Item/Spawn/ItemSpawnManager.h"

class AMainPlayerState;
DEFINE_LOG_CATEGORY_STATIC(LogMainGameMode, Log, All);

AMainGameMode::AMainGameMode()
{
    GameStateClass = AMainGameState::StaticClass();
    PlayerStateClass = AMainPlayerState::StaticClass();

    // 시작 및 종료 연출 시간
    StartDelay = 3.f;
    ResultScreenDuration = 10.f;

    bUseSeamlessTravel = true;

    // 라운드 전체 시간 초기화(기본값 설정 추후 수정 가능)
    PhaseScheduleMap.Add(0.f,   ERoundPhase::RoundStart);
    PhaseScheduleMap.Add(30.f,  ERoundPhase::RandomOpenTwo);
    PhaseScheduleMap.Add(60.f,  ERoundPhase::SaleEvent);
    PhaseScheduleMap.Add(90.f,  ERoundPhase::PremiumRespawn);
    PhaseScheduleMap.Add(150.f, ERoundPhase::FinalWarningOneOpen);
    PhaseScheduleMap.Add(180.f, ERoundPhase::RoundEnd);

    CheckoutManagerRef = nullptr;
    ItemSpawnManager = nullptr;

    // 모든 플레이어 접속 확인
    PlayerLoadCheckInterval = 0.5f;
    PlayerLoadWaitTimeout = 15.f;
    PlayerLoadWait = 0.f;
}

void AMainGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (auto* ProductShelfSubsystem = GetWorld()->GetSubsystem<UProductShelfSubsystem>())
    {
        UE_LOG(LogTemp, Log, TEXT("[게임모드] 선반 서브시스템 설정 초기화 실행"));
        ProductShelfSubsystem->InitializeConfig(ProductShelfManagerConfig);
    }

    if (auto* BC_EventSubsystem = GetWorld()->GetSubsystem<UBC_EventSubsystem>())
    {
        UE_LOG(LogTemp, Log, TEXT("[게임모드] 세일 이벤트 설정 초기화 실행"));
        BC_EventSubsystem->InitializeSaleEventConfig(SaleEventConfig);

        UE_LOG(LogTemp, Log, TEXT("[게임모드] 한정 이벤트 설정 초기화 실행"));
        BC_EventSubsystem->InitializeLimitedEventConfig(LimitedEventConfig);
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

    AActor* FoundActor2 = UGameplayStatics::GetActorOfClass(GetWorld(), AMapGimmickManager::StaticClass());
    if (FoundActor2)
    {
        MapGimmickManager = Cast<AMapGimmickManager>(FoundActor2);
    }

    AActor* FoundActor3 = UGameplayStatics::GetActorOfClass(GetWorld(), AItemSpawnManager::StaticClass());
    if (FoundActor3)
    {
        ItemSpawnManager = Cast<AItemSpawnManager>(FoundActor3);
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

// GameState, PlayerState가 세팅된 이후 게임 시작 가능한 시점에서 호출
void AMainGameMode::HandleMatchHasStarted()
{
    Super::HandleMatchHasStarted();

    UE_LOG(LogMainGameMode, Warning, TEXT("=== HandleMatchHasStarted: 매치가 시작되었습니다! ==="));

    // 모듶 플레이어가 접속했는지 체크
    CheckAllPlayersLoaded();

}

// 모든 플레이어가 접속했는지 체크
void AMainGameMode::CheckAllPlayersLoaded()
{
    // GameState가 존재하지 않으면 타이머를 통해 다시 체크
    AMainGameState* GS = GetGameState<AMainGameState>();
    if (!GS)
    {
        GetWorldTimerManager().SetTimer(Timer_PlayerLoadCheck, this, &AMainGameMode::CheckAllPlayersLoaded, PlayerLoadCheckInterval, false);
        return;
    }

    // 플레이어 기다린 시간 증가
    PlayerLoadWait += PlayerLoadCheckInterval;

    int32 ExpectedPlayerCount = 0;
    if (UMainGameInstance* GI = GetGameInstance<UMainGameInstance>())
    {
        ExpectedPlayerCount = GI->GetExpectedPlayerCount();
    }

   // 모든 플레이어가 접속했는지 체크
    bool bIsAllLoaded = true;
    for (APlayerState* PS : GS->PlayerArray)
    {
        APlayerController* PC = PS ? Cast<APlayerController>(PS->GetOwner()) : nullptr;

        if (!PC || !PC->HasClientLoadedCurrentWorld())
        {
            bIsAllLoaded = false;
            break;
        }
    }

   bool bTimedOut = false;
   if(PlayerLoadWait >= PlayerLoadWaitTimeout)
   {
       bTimedOut = true;
   }

    // 모든 플레이어가 접속했거나 접속 대기 시간 초과 시 강제 시작
    if (bIsAllLoaded || bTimedOut)
    {
        // 어떤 식으로 시작했는지 확인용 로그
        if (bTimedOut &&  !bIsAllLoaded)
        {
            UE_LOG(LogMainGameMode, Warning,
                TEXT("[플레이어 접속 체크] 타임아웃 발생 - 접속된 인원(%d/%d)으로 강제 시작"),
                GS->PlayerArray.Num(), ExpectedPlayerCount);
        }
        else
        {
            UE_LOG(LogMainGameMode, Warning,
                TEXT("[플레이어 접속 체크] 모든 플레이어(%d명) 접속 완료"), GS->PlayerArray.Num());
        }

        // 시작 연출 대기 이후 실제 라운드 시작
        OnGameStartEvent.Broadcast();
        GetWorldTimerManager().SetTimer(Timer_StartDelay, this, &AMainGameMode::StartRound, StartDelay, false);
        return;
    }

    UE_LOG(LogMainGameMode, Log,
      TEXT("[플레이어 접속 체크] 대기 중 (%d/%d명 접속, 경과 %.2f초)"), GS->PlayerArray.Num(), ExpectedPlayerCount, PlayerLoadWait);

    // 시작 조건 미충족 시 다시 타이머를 통해 다음 주기에 다시 체크
    GetWorldTimerManager().SetTimer(Timer_PlayerLoadCheck, this, &AMainGameMode::CheckAllPlayersLoaded, PlayerLoadCheckInterval, false);
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
            CheckoutManagerRef->StartCheckoutRotation();
        }

        if (ItemSpawnManager)
        {
            ItemSpawnManager->StartSpawning();
        }

        if (UProductShelfSubsystem* ShelfSubsystem = GetWorld()->GetSubsystem<UProductShelfSubsystem>())
        {
            // 스폰 시작 함수 호출
            ShelfSubsystem->StartProductSpawning();
        }

        if (MapGimmickManager)
        {
            MapGimmickManager->StartGimmickSpawning();
        }

        break;

    case ERoundPhase::RandomOpenTwo:
        // 랜덤 오픈 시작 / 계산대 3개중 2개 오픈
        PhaseName = TEXT("RandomOpenTwo (계산대 3개 중 2개 오픈)");

        break;

    case ERoundPhase::SaleEvent:
        // 세일 상품 이벤트 시작
        PhaseName = TEXT("SaleEvent (세일 상품 이벤트 시작)");

        if (UBC_EventSubsystem * EventSubsystem = GetWorld()->GetSubsystem<UBC_EventSubsystem>())
        {
            EventSubsystem->StartSaleEvent();
        }


        break;

    case ERoundPhase::PremiumRespawn:
        // 중앙 고급 상품 스폰
        PhaseName = TEXT("PremiumRespawn (중앙 고급 상품 스폰)");

        if (UBC_EventSubsystem * EventSubsystem = GetWorld()->GetSubsystem<UBC_EventSubsystem>())
        {
            EventSubsystem->StartLimitedEvent();
        }

        break;

    case ERoundPhase::FinalWarningOneOpen:
        // 마지막 30초 경고 및 계산대 1게만 오픈
        PhaseName = TEXT("FinalWarningOneOpen (마지막 30초 경고 및 계산대 1개만 오픈)");

        break;

    case ERoundPhase::RoundEnd:
        // 라운드 종료 계산대 전부 닫음
        // 결과화면 표시 및 점수 집계
        PhaseName = TEXT("RoundEnd (라운드 종료 계산대 전부 닫음)");
        if (CheckoutManagerRef)
        {
            CheckoutManagerRef->StopCheckoutRotation();
        }

        if (ItemSpawnManager)
        {
            ItemSpawnManager->StopSpawning();
        }

        GetWorldTimerManager().ClearTimer(Timer_RoundTick);

        // 보너스 점수 계산 전 등수 집계
        UpdateAllPlayerRanks();

        // 칭호 적용
        ApplyTitles();


        OnRoundResultsReady.Broadcast();

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

    UMainGameInstance* MainGI = Cast<UMainGameInstance>(GetGameInstance());
    if (!MainGI) return;

    const FString LevelPath = MainGI->LobbyLevel.ToSoftObjectPath().GetLongPackageName();

    GetWorld()->ServerTravel(LevelPath);
}

void AMainGameMode::ApplyTitles()
{
    AMainGameState* GS = GetGameState<AMainGameState>();
    if (!GS) return;

    TArray<AMainPlayerState*> AllPlayers;
    for (APlayerState* PS : GS->PlayerArray)
    {
        if (AMainPlayerState* MainPS = Cast<AMainPlayerState>(PS))
        {
            AllPlayers.Add(MainPS);
        }
    }

    //  마트 지배자 칭호 부여
    for (int32 i = 0; i < AllPlayers.Num(); ++i)
    {
        AMainPlayerState* MainPS = AllPlayers[i];
        if (MainPS->GetRank() == 1)
        {
            MainPS->SetTitle(ETitleType::MartKing);
            AllPlayers.RemoveAt(i);
        }
    }

    // 통로의 재앙 칭호 부여
    int32 BestBumpCount = -1;
    for (AMainPlayerState* MainPS : AllPlayers)
    {
        if (MainPS->GetCartBumpCount() >= 15)
        {
            BestBumpCount = FMath::Max(BestBumpCount, MainPS->GetCartBumpCount());
        }
    }
    for (int32 i = 0; i < AllPlayers.Num(); ++i)
    {
        AMainPlayerState* MainPS = AllPlayers[i];
        if (MainPS->GetCartBumpCount() == BestBumpCount)
        {
            MainPS->SetTitle(ETitleType::BumpKing);
            AllPlayers.RemoveAt(i);
        }
    }

    // 3) 파괴왕 칭호 부여
    int32 BestDroppedCount = -1;
    for (AMainPlayerState* MainPS : AllPlayers)
    {
        if (MainPS->GetDroppedItemCount() >= 8)
        {
            BestDroppedCount = FMath::Max(BestDroppedCount, MainPS->GetDroppedItemCount());
        }
    }
    for (int32 i = 0; i < AllPlayers.Num(); ++i)
    {
        AMainPlayerState* MainPS = AllPlayers[i];
        if (MainPS->GetDroppedItemCount() == BestDroppedCount)
        {
            MainPS->SetTitle(ETitleType::DestroyKing);
            AllPlayers.RemoveAt(i);
        }
    }

    // 4) 영수증 컬렉터 칭호 부여
    int32 BestCheckoutCount = -1;
    for (AMainPlayerState* MainPS : AllPlayers)
    {
        if (MainPS->GetCheckoutCount() >= 4)
        {
            BestCheckoutCount = FMath::Max(BestCheckoutCount, MainPS->GetCheckoutCount());
        }
    }
    for (int32 i = 0; i < AllPlayers.Num(); ++i)
    {
        AMainPlayerState* MainPS = AllPlayers[i];
        if (MainPS->GetCheckoutCount() == BestCheckoutCount)
        {
            MainPS->SetTitle(ETitleType::ReceiptCollector);
            AllPlayers.RemoveAt(i);
        }
    }
}

