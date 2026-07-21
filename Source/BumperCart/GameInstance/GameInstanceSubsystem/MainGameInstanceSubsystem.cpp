// MainGameInstanceSubsystem.cpp

#include "GameInstance/GameInstanceSubsystem/MainGameInstanceSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemModule.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSubsystemUtils.h"
#include "Modules/ModuleManager.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Engine/Engine.h"
#include "GameInstance/MainGameInstance.h"
#include "PlayerController/LobbyPlayerController.h"
#include "HAL/PlatformProcess.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CoreMisc.h"


IOnlineSessionPtr UMainGameInstanceSubsystem::GetSessionInterface() const
{
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        UE_LOG(LogTemp, Error, TEXT("[Steam] OnlineSubsystem을 찾을 수 없습니다."));
        return nullptr;
    }
    return OnlineSub->GetSessionInterface();
}

IOnlineIdentityPtr UMainGameInstanceSubsystem::GetIdentityInterface() const
{
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        UE_LOG(LogTemp, Error, TEXT("[Steam] OnlineSubsystem을 찾을 수 없습니다."));
        return nullptr;
    }
    return OnlineSub->GetIdentityInterface();
}


// 로그인 — Steam은 로그인 상태를 Steam 클라이언트가 관리 (CredentialName 미사용, 시그니처는 UI 호환용 유지)
void UMainGameInstanceSubsystem::Login(const FString& CredentialName)
{
    //패키지 실행인데 Steam을 못 잡은 경우(클라 미실행/미로그인) — 에디터(PIE)의 Null 폴백은 로컬 LAN 테스트용이라 통과
    if (IsUsingNullFallback() && IsRunningGame())
    {
        IOnlineSubsystem* Before = Online::GetSubsystem(GetWorld());
        UE_LOG(LogTemp, Warning, TEXT("[Steam] Reload 시도 전 서브시스템: %s"), Before ? *Before->GetSubsystemName().ToString() : TEXT("None"));


        //그 사이 Steam 클라이언트에 로그인했을 수 있으니 서브시스템 재생성 후 재판정 (타이틀 시점이라 안전)
        FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem").ReloadDefaultSubsystem();

        IOnlineSubsystem* After = Online::GetSubsystem(GetWorld());
        UE_LOG(LogTemp, Warning, TEXT("[Steam] Reload 시도 후 서브시스템: %s"), After ? *After->GetSubsystemName().ToString() : TEXT("None"));

        if (IsUsingNullFallback())
        {
            if (!bSteamLaunchURLSent)
            {
                FPlatformProcess::LaunchURL(TEXT("steam://open/main"), nullptr, nullptr);
                bSteamLaunchURLSent = true;
            }
            //여전히 실패 => Steam 클라이언트 실행을 띄워주고 안내. 로그인 후 Login 버튼 재시도 가능
            OnLoginResult.Broadcast(false, TEXT("Steam 로그인이 필요합니다. Steam 로그인 후 다시 시도해주세요."));
            return;
        }
    }

    IOnlineIdentityPtr Identity = GetIdentityInterface();
    if (!Identity.IsValid())
    {
        OnLoginResult.Broadcast(false, TEXT("Steam OnlineSubsystem을 찾을 수 없습니다. Steam 클라이언트 실행을 확인하세요."));
        return;
    }

    //Steam 클라이언트에 이미 로그인되어 있으면 바로 성공 처리
    if (Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn)
    {
        CachedDisplayName = Identity->GetPlayerNickname(0);
        UE_LOG(LogTemp, Log, TEXT("[Steam] 로그인 확인. 닉네임: %s"), *CachedDisplayName);
        OnLoginResult.Broadcast(true, CachedDisplayName);
        return;
    }

    // 이미 비동기 로그인 요청이 진행 중이면 중복 호출 방지
    if (bIsLoginRequestPending) return;

    bIsLoginRequestPending = true;
    //미로그인 상태 => 자동 로그인 시도 (자격증명은 Steam 클라이언트가 처리)
    Identity->ClearOnLoginCompleteDelegates(0, this);
    Identity->OnLoginCompleteDelegates[0].AddUObject(this, &UMainGameInstanceSubsystem::OnLoginComplete);
    Identity->Login(0, FOnlineAccountCredentials());
}

void UMainGameInstanceSubsystem::StartAutoLoginRetry(float IntervalSeconds)
{
    if (bAutoLoginRetryActive) return;

    bAutoLoginRetryActive = true;
    bSteamLaunchURLSent = false;

    OnLoginResult.AddDynamic(this, &UMainGameInstanceSubsystem::HandleAutoLoginResult);

    // 최초 1회 로그인 시도
    Login(TEXT(""));

    if (bAutoLoginRetryActive)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(LoginRetryTimerHandle, this, &UMainGameInstanceSubsystem::AutoLoginTick, IntervalSeconds, true);
        }
    }
}

void UMainGameInstanceSubsystem::AutoLoginTick()
{
    Login(TEXT(""));
}

void UMainGameInstanceSubsystem::StopAutoLoginRetry()
{
    if (!bAutoLoginRetryActive) return;
    bAutoLoginRetryActive = false;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(LoginRetryTimerHandle);
    }

    OnLoginResult.RemoveDynamic(this, &UMainGameInstanceSubsystem::HandleAutoLoginResult);
}

void UMainGameInstanceSubsystem::HandleAutoLoginResult(bool bWasSuccessful, const FString& ErrorMessage)
{
    if (!bAutoLoginRetryActive) return;

    if (bWasSuccessful)
    {
        StopAutoLoginRetry();
    }
}

void UMainGameInstanceSubsystem::Deinitialize()
{
    StopAutoLoginRetry();
    Super::Deinitialize();
}

void UMainGameInstanceSubsystem::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
    bIsLoginRequestPending = false;

    IOnlineIdentityPtr Identity = GetIdentityInterface();
    if (Identity.IsValid())
    {
        Identity->ClearOnLoginCompleteDelegates(LocalUserNum, this);
    }

    if (bWasSuccessful)
    {
        UE_LOG(LogTemp, Log, TEXT("[Steam] 로그인 성공. UserId: %s"), *UserId.ToString());

        if (Identity.IsValid())
        {
            CachedDisplayName = Identity->GetPlayerNickname(LocalUserNum); //Steam 닉네임
        }

        OnLoginResult.Broadcast(true, CachedDisplayName);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 로그인 실패: %s"), *Error);
        OnLoginResult.Broadcast(false, Error);
    }
}

//세션 호스팅(리슨서버)
void UMainGameInstanceSubsystem::HostListenServer(const FString& InRoomName, const FString& InRoomPassword)
{
    //호스팅할 방 정보
    RoomName     = InRoomName;
    RoomPassword = InRoomPassword.TrimStartAndEnd();

    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid()) return;


    //같은 이름의 세션 존재 시 해당 세션 우선 제거 후 진행
    if (Sessions->GetNamedSession(NAME_GameSession))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 기존 GameSession이 남아있어 먼저 파괴합니다."));
        Sessions->OnDestroySessionCompleteDelegates.AddUObject(this, &UMainGameInstanceSubsystem::OnDestroySessionComplete);
        Sessions->DestroySession(NAME_GameSession);
        return;
    }

    CreateSessionInternal();
}


//Steam을 못 잡아 Null로 폴백된 상태인지 (에디터 PIE 등) => LAN 모드로 로컬 멀티 테스트 가능하게
bool UMainGameInstanceSubsystem::IsUsingNullFallback() const
{
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    return OnlineSub && OnlineSub->GetSubsystemName() == NULL_SUBSYSTEM;
}

void UMainGameInstanceSubsystem::CreateSessionInternal()
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid()) return;

    //세션 세팅
    FOnlineSessionSettings SessionSettings;
    SessionSettings.bIsDedicated          = false;  // 리슨 서버
    SessionSettings.bIsLANMatch           = IsUsingNullFallback(); //PIE(Null 폴백)에선 LAN 세션
    SessionSettings.bShouldAdvertise      = true;   // 검색 가능
    SessionSettings.bAllowJoinInProgress  = true;
    SessionSettings.bUsesPresence         = true;   // Steam 로비 검색/참가에 필요
    SessionSettings.bAllowInvites         = true; // 친구 초대 여부
    SessionSettings.bAllowJoinViaPresence = true;
    SessionSettings.bUseLobbiesIfAvailable= true;   // Steam Lobby 사용
    SessionSettings.NumPublicConnections  = 4;      // 최대 인원 수
    SessionSettings.Set(SEARCH_KEYWORDS, FString(TEXT("MyRoom")), EOnlineDataAdvertisementType::ViaOnlineService);

    // 방 제목 - 검색 결과에서 UI에 표시하기 위해 온라인 서비스로 광고
    SessionSettings.Set(SETTING_ROOMNAME, RoomName, EOnlineDataAdvertisementType::ViaOnlineService);
    // 방 비밀번호
    SessionSettings.Set(SETTING_ROOMPASSWORD, RoomPassword, EOnlineDataAdvertisementType::ViaOnlineService);
    // 방장 이름(DevAuthTool 활용한 로컬 테스트 시 이름 변수가 자동으로 채워지지 않아서 커스텀 세팅 진행)
    SessionSettings.Set(SETTING_OWNERNAME, CachedDisplayName, EOnlineDataAdvertisementType::ViaOnlineService);

    Sessions->OnCreateSessionCompleteDelegates.AddUObject(this, &UMainGameInstanceSubsystem::OnCreateSessionComplete);
    Sessions->CreateSession(0, NAME_GameSession, SessionSettings);
}

void UMainGameInstanceSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (Sessions.IsValid())
    {
        Sessions->ClearOnCreateSessionCompleteDelegates(this);
    }

    if (!bWasSuccessful)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 세션 생성 실패: %s"), *SessionName.ToString());
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[Steam] 세션 생성 성공: %s"), *SessionName.ToString());

    UMainGameInstance* MainGI = Cast<UMainGameInstance>(GetGameInstance());
    if (!MainGI) return;

    const FString LobbyPath = GetLevelPath(MainGI->LobbyLevel);
    if (LobbyPath.IsEmpty())
    {
        return;
    }
    UE_LOG(LogTemp, Warning, TEXT("주소: %s"), *LobbyPath)
   //?listen이 핵심 해당 클라이언트가 서버가 되는 부분
    GetWorld()->ServerTravel(LobbyPath+TEXT("?listen"));
}

void UMainGameInstanceSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (Sessions.IsValid())
    {
        Sessions->ClearOnDestroySessionCompleteDelegates(this);
    }

    if (bWasSuccessful)
    {
        // 기존 세션 정리가 끝났으니 다시 호스팅 시도
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 기존 세션 파괴 성공 → 재생성 진행"));
        CreateSessionInternal();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 기존 세션 제거 실패: %s"), *SessionName.ToString());
    }
}

// 공통으로 사용하는 검색 세팅 설정
void UMainGameInstanceSubsystem::CreateSearchSettings()
{
    SearchSettings = MakeShareable(new FOnlineSessionSearch());
    SearchSettings->MaxSearchResults = 50;
    SearchSettings->bIsLanQuery = IsUsingNullFallback(); //PIE(Null 폴백)에선 LAN 검색
    SearchSettings->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals); //Steam 로비 검색
    SearchSettings->QuerySettings.Set(SEARCH_KEYWORDS, FString(TEXT("MyRoom")), EOnlineComparisonOp::Equals);
}

// 찾을 세션 설정 및 찾기 시도
void UMainGameInstanceSubsystem::FindSessions(const FString& RoomNameFilter)
{
    SearchRoomNameFilter = RoomNameFilter;

    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid()) return;

    CreateSearchSettings();

    Sessions->OnFindSessionsCompleteDelegates.AddUObject(this, &UMainGameInstanceSubsystem::OnFindSessionComplete);
    Sessions->FindSessions(0, SearchSettings.ToSharedRef());
}

// 방 찾은 이후 정보 가공
void UMainGameInstanceSubsystem::OnFindSessionComplete(bool bWasSuccessful)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (Sessions.IsValid())
    {
        Sessions->ClearOnFindSessionsCompleteDelegates(this);
    }

    if (!bWasSuccessful || !SearchSettings.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 세션 검색 실패"));
        return;
    }

    FilteredResults.Reset();
    for (const FOnlineSessionSearchResult& SearchResult : SearchSettings->SearchResults)
    {
        //방 제목과 일치하는 방 검색
        if (!SearchRoomNameFilter.IsEmpty())
        {
            FString FoundName;
            SearchResult.Session.SessionSettings.Get(SETTING_ROOMNAME, FoundName);

            if (!FoundName.Contains(SearchRoomNameFilter, ESearchCase::IgnoreCase)) continue;
        }

        FilteredResults.Add(SearchResult);
    }

    UE_LOG(LogTemp, Log, TEXT("[Steam] 전체 세션 %d개 발견, 필터링 후 %d개"),
    SearchSettings->SearchResults.Num(),
    FilteredResults.Num());


    // UI에 결과 전달 필요 시 해당 부분에 작성
    OnSessionsFound.Broadcast(FilteredResults.Num());
}



void UMainGameInstanceSubsystem::JoinFoundSession(int32 Index, const FString& InputPassWord)
{
    // UI 목록은 FilteredResults 기준 => 인덱스도 같은 배열로 조회 (SearchResults 원본과 순서가 다를 수 있음)
    if (!FilteredResults.IsValidIndex(Index))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 잘못된 세션 인덱스: %d"), Index);
        return;
    }

    JoinSessionInternal(FilteredResults[Index], InputPassWord);
}

// 세션 참가 공통 처리 — 비밀번호 검증 후 JoinSession
void UMainGameInstanceSubsystem::JoinSessionInternal(const FOnlineSessionSearchResult& Result, const FString& InputPassword)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid()) return;

    // 방에 설정된 비밀번호와 입력값 비교
    FString CurrentRoomPassword;
    Result.Session.SessionSettings.Get(SETTING_ROOMPASSWORD, CurrentRoomPassword);

    // 비밀번호가 비어있으면 그냥 입장 가능
    if (!CurrentRoomPassword.IsEmpty() && CurrentRoomPassword != InputPassword)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 비밀번호가 일치하지 않습니다."));
        OnJoinPasswordIncorrect.Broadcast();
        return;
    }

    Result.Session.SessionSettings.Get(SETTING_ROOMNAME, RoomName);

    Sessions->OnJoinSessionCompleteDelegates.AddUObject(this, &UMainGameInstanceSubsystem::OnJoinSessionComplete);
    Sessions->JoinSession(0, NAME_GameSession, Result);
}

void UMainGameInstanceSubsystem::JoinPrivateRoomByName(const FString& InRoomName, const FString& InRoomPassword)
{
    PrivateRoomName = InRoomName.TrimStartAndEnd();
    PrivateRoomPassword = InRoomPassword.TrimStartAndEnd();

    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid()) return;

    CreateSearchSettings();

    // 비공개 방 참가 전용 콜백 바인딩
    Sessions->OnFindSessionsCompleteDelegates.AddUObject(this, &UMainGameInstanceSubsystem::OnPrivateJoinFindSessionComplete);
    Sessions->FindSessions(0, SearchSettings.ToSharedRef());
}

//비공개 방 검색 결과 확인
void UMainGameInstanceSubsystem::OnPrivateJoinFindSessionComplete(bool bWasSuccessful)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (Sessions.IsValid())
    {
        Sessions->ClearOnFindSessionsCompleteDelegates(this);
    }

    if (!bWasSuccessful || !SearchSettings.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 비공개 방 참가: 세션 검색 실패"));
        OnPrivateRoomNotFound.Broadcast();
        return;
    }

    TryJoinPrivateSession();
}

//비공개 방 참가 시도
void UMainGameInstanceSubsystem::TryJoinPrivateSession()
{
    if (!SearchSettings.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 비공개 방 참가: 검색 결과가 없습니다."));
        OnPrivateRoomNotFound.Broadcast();
        return;
    }

    const TArray<FOnlineSessionSearchResult>& Results = SearchSettings->SearchResults;


    // 입력한 방 이름과 일치하는 방 검색
    int32 FoundIndex = INDEX_NONE;
    for (int32 i = 0; i < Results.Num(); ++i)
    {
        const FOnlineSessionSearchResult& Result = Results[i];
        if (!Result.IsValid()) continue;

        FString FoundRoomName;
        Result.Session.SessionSettings.Get(SETTING_ROOMNAME, FoundRoomName);

        // 검색한 방 이름과 일치하는지 확인 / 찾으면 반복문 탈출
        if (FoundRoomName.Equals(PrivateRoomName, ESearchCase::IgnoreCase))
        {
            FoundIndex = i;
            break;
        }
    }

    if (FoundIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 비공개 방 참가: '%s' 방을 찾을 수 없습니다."), *PrivateRoomName);
        OnPrivateRoomNotFound.Broadcast();
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[Steam] 비공개 방 참가: Index %d 방 발견, 참가 시도"), FoundIndex);

    //비밀번호 확인은 공통 처리에서 (FoundIndex는 SearchResults 기준이라 결과를 직접 전달)
    JoinSessionInternal(Results[FoundIndex], PrivateRoomPassword);
}

void UMainGameInstanceSubsystem::QuickMatch()
{
    SearchRoomNameFilter.Empty();
    bIsQuickMatchRequest = true;

    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid())
    {
        bIsQuickMatchRequest = false;
        return;
    }

    CreateSearchSettings();

    Sessions->OnFindSessionsCompleteDelegates.AddUObject(this, &UMainGameInstanceSubsystem::OnQuickMatchFindSessionComplete);
    Sessions->FindSessions(0, SearchSettings.ToSharedRef());
}

// 퀵 매치로 들어갈 방 찾을 시 호출
void UMainGameInstanceSubsystem::OnQuickMatchFindSessionComplete(bool bWasSuccessful)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (Sessions.IsValid())
    {
        Sessions->ClearOnFindSessionsCompleteDelegates(this);
    }

    if (!bWasSuccessful || !SearchSettings.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 퀵매치: 세션 검색 실패"));
        OnQuickMatchNoSessionFound.Broadcast();
        return;
    }

    TryJoinQuickMatchSession();
}

// 퀵 매치로 방 진입 시도
void UMainGameInstanceSubsystem::TryJoinQuickMatchSession()
{
    if (!SearchSettings.IsValid())
    {
        OnQuickMatchNoSessionFound.Broadcast();
        return;
    }

    const TArray<FOnlineSessionSearchResult>& Results = SearchSettings->SearchResults;

    int32 BestIndex = INDEX_NONE;
    int32 BestPing = MAX_int32;

    for (int32 i = 0; i < Results.Num(); ++i)
    {
        const FOnlineSessionSearchResult& Result = Results[i];
        if (!Result.IsValid()) continue;


        // 방 정원 가득 찾을 시 넘어감
        if (Result.Session.NumOpenPublicConnections <= 0)
        {
            continue;
        }


        //방에 비밀번호가 존재할 시 넘어감
        FString Password;
        Result.Session.SessionSettings.Get(SETTING_ROOMPASSWORD, Password);
        if (!Password.IsEmpty()) continue;


        //핑을 비교하여 핑이 더 낮은 index 저장
        if (Result.PingInMs < BestPing)
        {
            BestPing = Result.PingInMs;
            BestIndex = i;
        }
    }

    // 비밀번호가 없는 방 존재 X (루프 안에서 첫 방에 바로 참가하던 것 => 전체 비교 후 최저 핑 방 선택, 없으면 브로드캐스트)
    if (BestIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 퀵매치: 참가 가능한(비밀번호 없는) 방을 찾지 못했습니다."));
        OnQuickMatchNoSessionFound.Broadcast();
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[Steam] 퀵매치: Index %d 방 참가 시도 (Ping: %d ms)"), BestIndex, BestPing);

    JoinSessionInternal(Results[BestIndex], TEXT(""));
}

// 세션 나가기
// 호스트가 호출 세션 파괴
// 참가자가 호출 시 세션에서 이탈
void UMainGameInstanceSubsystem::LeaveSession()
{

    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid() || !Sessions->GetNamedSession(NAME_GameSession))
    {
        //나갈 세션이 존재하지 않으므로 바로 타이틀로 이동 안전코드
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 나갈 세션이 없습니다. 타이틀로 이동"));
        ReturnToTitle();
        return;
    }

    const UWorld* World = GetWorld();
    const ENetMode CurrentNetMode = World ? World->GetNetMode() : NM_MAX;
    if (CurrentNetMode == NM_ListenServer)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 호스트 나가기 시작: 클라이언트 우선 퇴장"));

        NotifyClientsToLeaveBeforeHostDestroy();

       return;
    }

    //DestroySession은 비동기이므로 델리게이트 등록하여 추후에 콜백 받음
    Sessions->OnDestroySessionCompleteDelegates.AddUObject(this, &UMainGameInstanceSubsystem::OnLeaveSessionComplete);
    Sessions->DestroySession(NAME_GameSession);
}

void UMainGameInstanceSubsystem::NotifyClientsToLeaveBeforeHostDestroy()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        DestroyHostSession();
        return;
    }

    PendingHostLeaveAcks.Reset();


    // 나가야할 클라이언트 추가
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();

        if (!PC || PC->IsLocalController()) continue;

        if (ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(PC))
        {
            PendingHostLeaveAcks.Add(LobbyPC);

            LobbyPC->OnDestroyed.AddDynamic(this, &UMainGameInstanceSubsystem::OnNotifiedClientDestroyed);
            LobbyPC->Client_NotifyHostIsLeaving();
        }
    }

    // 모든 클라이언트가 나갔으면 호스트 세션 파괴
    if (PendingHostLeaveAcks.Num() == 0)
    {
        DestroyHostSession();
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[Steam] %d명의 클라이언트 ACK 대기 시작"), PendingHostLeaveAcks.Num());

    // 전원 ACK을 못 받는 이상 상황(클라 응답 없음 등)을 대비한 안전장치
    World->GetTimerManager().SetTimer(HostLeaveWaitTimerHandle,this, &UMainGameInstanceSubsystem::DestroyHostSession,3,false);
}

void UMainGameInstanceSubsystem::OnClientAckLeftSession(APlayerController* FromPC)
{
    if (!FromPC) return;

    const int32 RemoveCount = PendingHostLeaveAcks.Remove(FromPC);
    if (RemoveCount > 0 )
    {
        FromPC->OnDestroyed.RemoveDynamic(this, &UMainGameInstanceSubsystem::OnNotifiedClientDestroyed);
    }

    UE_LOG(LogTemp, Log, TEXT("[Steam] 클라이언트 퇴장 ACK 수신 (남은 대상: %d)"), PendingHostLeaveAcks.Num());

    // 모든 클라이언트 퇴장 완료 시
    if (PendingHostLeaveAcks.Num() == 0)
    {
        DestroyHostSession();
    }
}

// 강제종료와 같이 클라이언트 연결이 끝기면 대기 목록에서 제거 및 알림
void UMainGameInstanceSubsystem::OnNotifiedClientDestroyed(AActor* DestroyedActor)
{
    APlayerController* PC = Cast<APlayerController>(DestroyedActor);
    if (!PC) return;

    if (PendingHostLeaveAcks.Remove(PC) > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] ACK 없이 클라이언트 연결 종료 감지, 대기 목록에서 제거 (남은 대상: %d)"), PendingHostLeaveAcks.Num());

        if (PendingHostLeaveAcks.Num() == 0)
        {
            DestroyHostSession();
        }
    }
}

void UMainGameInstanceSubsystem::DestroyHostSession()
{
    if (bHostDestroyInProgress) return;

    bHostDestroyInProgress = true;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(HostLeaveWaitTimerHandle);
    }


    PendingHostLeaveAcks.Reset();

    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid() || !Sessions->GetNamedSession(NAME_GameSession))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 호스트 세션 파괴 시점에 세션이 이미 없습니다. 타이틀로 이동"));
        bHostDestroyInProgress = false;
        ReturnToTitle();
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[Steam] 클라이언트 퇴장 확인 완료. 호스트 세션 파괴 시도"));

    Sessions->ClearOnDestroySessionCompleteDelegates(this);
    Sessions->OnDestroySessionCompleteDelegates.AddUObject(this, &UMainGameInstanceSubsystem::OnLeaveSessionComplete);
    Sessions->DestroySession(NAME_GameSession);
}

void UMainGameInstanceSubsystem::NotifyLeaveRequestedByHost()
{
    bShouldAckHostOnLeaveComplete = true;
    LeaveSession();
}

void UMainGameInstanceSubsystem::OnLeaveSessionComplete(FName SessionName, bool bWasSuccessful)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (Sessions.IsValid())
    {
        Sessions->ClearOnDestroySessionCompleteDelegates(this);
    }

    //세션 파괴 실패해도 타이틀로 이동 다만 로그로 확인
    if (bWasSuccessful)
    {
        UE_LOG(LogTemp, Log, TEXT("[Steam] 세션 나가기 성공: %s"), *SessionName.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 세션 나가기(파괴) 실패: %s - 그래도 타이틀로 이동합니다."), *SessionName.ToString());
    }

    //방 정보 초기화
    RoomName.Empty();
    RoomPassword.Empty();
    bIsHardMode = false;
    bHostDestroyInProgress = false;

    if (bShouldAckHostOnLeaveComplete)
    {
        bShouldAckHostOnLeaveComplete = false;

        APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController(GetWorld()) : nullptr;
        if (ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(PC))
        {
            LobbyPC->Server_AckLeftSession();
        }
    }

    OnLeaveSessionResult.Broadcast(bWasSuccessful);

    ReturnToTitle();
}

void UMainGameInstanceSubsystem::ReturnToTitle()
{
    UWorld* World = GetWorld();
    if (!World) return;

    UMainGameInstance* MainGI = Cast<UMainGameInstance>(World->GetGameInstance());
    if (!MainGI) return;

    const FString LevelPath = GetLevelPath(MainGI->TitleLevel);
    if (LevelPath.IsEmpty()) return;

    const ENetMode CurrentNetMode = World->GetNetMode();
    if (CurrentNetMode == NM_ListenServer )
    {
        UE_LOG(LogTemp, Log, TEXT("[Steam] 호스트: 타이틀 이동 "));

        GEngine->ShutdownWorldNetDriver(World);

        UGameplayStatics::OpenLevel(this, FName(*LevelPath), true);
    }
    else if (CurrentNetMode == NM_Client)
    {
        APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController(GetWorld()) : nullptr;
        if (PC)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Steam] 참가자 타이틀로 이동"));
            PC->ClientTravel(LevelPath, TRAVEL_Absolute);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[Steam] PlayerController를 찾을 수 없어 타이틀로 이동할 수 없습니다."));
        }
    }
}

void UMainGameInstanceSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (Sessions.IsValid())
    {
        Sessions->ClearOnJoinSessionCompleteDelegates(this);
    }

    switch (Result)
    {
    case EOnJoinSessionCompleteResult::Success:
        {
            FString ConnectString;
            if (Sessions.IsValid() && Sessions->GetResolvedConnectString(SessionName, ConnectString))
            {
                APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController(GetWorld()) : nullptr;
                if (PC)
                {
                    //Steam P2P 주소(steam.<SteamID64>)는 SteamNetDriver가 그대로 처리
                    UE_LOG(LogTemp, Log, TEXT("[Steam] 세션 접속 시도: %s"), *ConnectString);
                    PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[Steam] 접속 주소를 가져오지 못했습니다."));
            }
            break;
        }
    case EOnJoinSessionCompleteResult::SessionIsFull:
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 세션이 가득 찼습니다."));
        break;
    case EOnJoinSessionCompleteResult::SessionDoesNotExist:
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 세션이 더 이상 존재하지 않습니다."));
        break;
    case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 호스트 주소를 가져오지 못했습니다."));
        break;
    case EOnJoinSessionCompleteResult::AlreadyInSession:
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 이미 해당 세션에 참여 중입니다."));
        break;
    default:
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 알 수 없는 오류로 조인 실패."));
        break;
    }
}

FString UMainGameInstanceSubsystem::GetFoundSessionRoomName(int32 Index) const
{
    if (!SearchSettings.IsValid() || !SearchSettings->SearchResults.IsValidIndex(Index)) return FString();

    FString FoundRoomName;
    FilteredResults[Index].Session.SessionSettings.Get(SETTING_ROOMNAME, FoundRoomName);
    return FoundRoomName;
}

FString UMainGameInstanceSubsystem::GetFoundSessionOwnerName(int32 Index) const
{
    if (!SearchSettings.IsValid() || !SearchSettings->SearchResults.IsValidIndex(Index)) return FString();

    FString FoundOwnerName;
    FilteredResults[Index].Session.SessionSettings.Get(SETTING_OWNERNAME, FoundOwnerName);
    return FoundOwnerName;
}

int32 UMainGameInstanceSubsystem::GetFoundSessionUserCount(int32 Index) const
{
    if (!SearchSettings.IsValid() || !SearchSettings->SearchResults.IsValidIndex(Index)) return 0;

    const FOnlineSession& FoundSession = FilteredResults[Index].Session;

    const int32 MaxPublicConnections  = FoundSession.SessionSettings.NumPublicConnections;
    const int32 OpenPublicConnections = FoundSession.NumOpenPublicConnections;
    UE_LOG(LogTemp, Warning, TEXT("최대 방 인원수: %d / 빈 자리: %d / 접속 인원: %d"), MaxPublicConnections,  OpenPublicConnections, FMath::Clamp(MaxPublicConnections-OpenPublicConnections, 0, MaxPublicConnections))
    return FMath::Clamp(MaxPublicConnections-OpenPublicConnections, 0, MaxPublicConnections);
}

// 방에 비밀번호가 존재하다면 비공개 방으로 판단
bool UMainGameInstanceSubsystem::GetFoundSessionIsPrivate(int32 Index) const
{
    if (!FilteredResults.IsValidIndex(Index)) return false;

    FString FoundPassword;
    FilteredResults[Index].Session.SessionSettings.Get(SETTING_ROOMPASSWORD, FoundPassword);
    return !FoundPassword.IsEmpty();
}

int32 UMainGameInstanceSubsystem::GetFoundSessionCount() const
{
    if (!SearchSettings.IsValid()) return 0;
    return SearchSettings->SearchResults.Num();
}

void UMainGameInstanceSubsystem::UpdateRoomDifficulty(bool bHardMode)
{
    //호스트만 난이도 변경가능하게 방지
    const ENetMode CurrentNetMode = GetWorld() ? GetWorld()->GetNetMode() : NM_MAX;
    if (CurrentNetMode != NM_ListenServer && CurrentNetMode != NM_Standalone)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 방장(호스트)만 난이도를 변경할 수 있습니다."));
        return;
    }

    if (bIsHardMode == bHardMode) return;

    bIsHardMode = bHardMode;
    UE_LOG(LogTemp, Warning, TEXT("[Room] 난이도 변경: %s"), bIsHardMode ? TEXT("HardMode") : TEXT("NormalMode"));
}

FString UMainGameInstanceSubsystem::GetLevelPath(const TSoftObjectPtr<UWorld>& Level) const
{
    if (Level.IsNull())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] 에디터에서 레벨이 설정되지 않았습니다."));
        return FString();
    }

    return Level.ToSoftObjectPath().GetLongPackageName();
}

void UMainGameInstanceSubsystem::UpdateCurrentSession()
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid()) return;

    // 현재 활성화된 방의 정보 조회
    FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession);
    if (!Session) return;

    // 진행 중이면 완료 콜백에서 다시 한번 최신 상태로 갱신하도록 예약만 해두고 리턴
    if (bSessionUpdateInProgress)
    {
        bSessionUpdatePending = true;
        UE_LOG(LogTemp, Verbose, TEXT("[Steam] UpdateSession 진행 중 → 이번 요청은 예약만 함"));
        return;
    }


    bSessionUpdateInProgress = true;
    Sessions->OnUpdateSessionCompleteDelegates.AddUObject(this, &UMainGameInstanceSubsystem::OnUpdateSessionComplete);

    // 세션 정보 갱신 요청
    const bool bUpdateStarted = Sessions->UpdateSession(NAME_GameSession, Session->SessionSettings, true);
    if (!bUpdateStarted)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Steam] UpdateSession 요청 시작 실패"));
        Sessions->ClearOnUpdateSessionCompleteDelegates(this);
        bSessionUpdateInProgress = false;
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[Steam] 세션 인원수 갱신 (UpdateSession) 요청 시작"));
    }
}

void UMainGameInstanceSubsystem::OnUpdateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (Sessions.IsValid())
    {
        Sessions->ClearOnUpdateSessionCompleteDelegates(this);
    }

    bSessionUpdateInProgress = false;

    UE_LOG(LogTemp, Log, TEXT("[Steam] 세션 인원수 갱신 (UpdateSession) %s: %s"),
        bWasSuccessful ? TEXT("성공") : TEXT("실패"), *SessionName.ToString());

    // 갱신이 진행되는 동안 추가로 들어온 요청이 있었다면, 최신 인원 수 기준으로 한 번 더 갱신
    if (bSessionUpdatePending)
    {
        bSessionUpdatePending = false;
        UpdateCurrentSession();
    }
}
