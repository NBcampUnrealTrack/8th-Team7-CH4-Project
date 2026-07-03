// MainGameInstanceSubsystem.cpp

#include "GameInstance/GameInstanceSubsystem/MainGameInstanceSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Engine/Engine.h"
#include "GameInstance/MainGameInstance.h"


IOnlineSessionPtr UMainGameInstanceSubsystem::GetSessionInterface() const
{
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        UE_LOG(LogTemp, Error, TEXT("[EOS] OnlineSubsystem을 찾을 수 없습니다."));
        return nullptr;
    }
    return OnlineSub->GetSessionInterface();
}

IOnlineIdentityPtr UMainGameInstanceSubsystem::GetIdentityInterface() const
{
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        UE_LOG(LogTemp, Error, TEXT("[EOS] OnlineSubsystem을 찾을 수 없습니다."));
        return nullptr;
    }
    return OnlineSub->GetIdentityInterface();
}


// 로그인
void UMainGameInstanceSubsystem::Login(const FString& CredentialName)
{
    IOnlineIdentityPtr Identity = GetIdentityInterface();
    if (!Identity.IsValid()) return;

    if (Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 이미 로그인되어 있습니다."));
        return;
    }

    FOnlineAccountCredentials Credentials;
    //개발자 테스트용
    /*
    Credentials.Type = TEXT("Developer");
    Credentials.Id = TEXT("Localhost:7777");
    Credentials.Token = CredentialName;


    //패키징 테스트용

    Credentials.Type = TEXT("AccountPortal");
    Credentials.Id = TEXT("");
    Credentials.Token = TEXT("");
    */



    // 에디터(PIE) 환경 - DevAuthTool을 이용한 개발자 테스트용 로그인
    Credentials.Type  = TEXT("Developer");
    Credentials.Id    = TEXT("Localhost:7777");
    Credentials.Token = CredentialName;

    UE_LOG(LogTemp, Log, TEXT("[EOS] (Editor) Developer 로그인 시도 - Credential: %s"), *CredentialName);
    /*
    // 배포 빌드 - AccountPortal을 통한 실제 로그인 (Id/Token은 EOS가 자동 처리)
    Credentials.Type  = TEXT("AccountPortal");
    Credentials.Id    = TEXT("");
    Credentials.Token = TEXT("");
    */
    UE_LOG(LogTemp, Log, TEXT("[EOS] (Build) AccountPortal 로그인 시도"));


    Identity->ClearOnLoginCompleteDelegates(0, this);
    Identity->OnLoginCompleteDelegates[0].AddUObject(this, &UMainGameInstanceSubsystem::OnLoginComplete);
    Identity->Login(0, Credentials);
}

void UMainGameInstanceSubsystem::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
    IOnlineIdentityPtr Identity = GetIdentityInterface();
    if (Identity.IsValid())
    {
        Identity->ClearOnLoginCompleteDelegates(LocalUserNum, this);
    }

    if (bWasSuccessful)
    {
        UE_LOG(LogTemp, Log, TEXT("[EOS] 로그인 성공. UserId: %s"), *UserId.ToString());

        if (Identity.IsValid())
        {
            TSharedPtr<FUserOnlineAccount> Account = Identity->GetUserAccount(UserId);
            if (Account.IsValid())
            {
                CachedDisplayName = Account->GetDisplayName();
            }
        }

        OnLoginResult.Broadcast(true, CachedDisplayName);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 로그인 실패: %s"), *Error);
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
        Sessions->OnDestroySessionCompleteDelegates.AddUObject(this, &UMainGameInstanceSubsystem::OnDestroySessionComplete);
        Sessions->DestroySession(NAME_GameSession);
        return;
    }

    CreateSessionInternal();
}

void UMainGameInstanceSubsystem::CreateSessionInternal()
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid()) return;

    //세션 세팅
    FOnlineSessionSettings SessionSettings;
    SessionSettings.bIsDedicated          = false;  // 리슨 서버
    SessionSettings.bIsLANMatch           = false;
    SessionSettings.bShouldAdvertise      = true;   // 검색 가능
    SessionSettings.bAllowJoinInProgress  = true;
    SessionSettings.bUsesPresence         = false;  // 친구 목록 보고 게임 참가 가능 여부
    SessionSettings.bAllowInvites         = true; // 친구 초대 여부
    SessionSettings.bUseLobbiesIfAvailable= true;   // EOS Lobby 사용
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
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 세션 생성 실패: %s"), *SessionName.ToString());
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[EOS] 세션 생성 성공: %s"), *SessionName.ToString());

    UMainGameInstance* MainGI = Cast<UMainGameInstance>(GetGameInstance());
    if (!MainGI) return;

    const FString LobbyPath = GetLevelPath(MainGI->LobbyLevel);
    if (LobbyPath.IsEmpty())
    {
        return;
    }

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
        CreateSessionInternal();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 기존 세션 제거 실패: %s"), *SessionName.ToString());
    }
}

// 공통으로 사용하는 검색 세팅 설정
void UMainGameInstanceSubsystem::CreateSearchSettings()
{
    SearchSettings = MakeShareable(new FOnlineSessionSearch());
    SearchSettings->MaxSearchResults = 50;
    SearchSettings->bIsLanQuery = false;
    SearchSettings->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
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

// 퀵 매치 분기 추가
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
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 세션 검색 실패"));
        return;
    }

    FilteredResults.Reset();
    for (const FOnlineSessionSearchResult& SearchResult : SearchSettings->SearchResults)
    {
        //비밀번호가 설정되어있으면 비공개이므로 제외
        FString FoundPassword;
        SearchResult.Session.SessionSettings.Get(SETTING_ROOMPASSWORD, FoundPassword);
        if (!FoundPassword.IsEmpty()) continue;

        //방 제목과 일치하는 방 검색
        if (!SearchRoomNameFilter.IsEmpty())
        {
            FString FoundName;
            SearchResult.Session.SessionSettings.Get(SETTING_ROOMNAME, FoundName);

            if (!FoundName.Contains(SearchRoomNameFilter, ESearchCase::IgnoreCase)) continue;
        }

        FilteredResults.Add(SearchResult);
    }

    UE_LOG(LogTemp, Log, TEXT("[EOS] 전체 세션 %d개 발견, 필터링 후 %d개"),
    SearchSettings->SearchResults.Num(),
    FilteredResults.Num());


    // UI에 결과 전달 필요 시 해당 부분에 작성
    OnSessionsFound.Broadcast(FilteredResults.Num());
}



void UMainGameInstanceSubsystem::JoinFoundSession(int32 Index, const FString& InputPassWord)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();

    if (!Sessions.IsValid() || !SearchSettings.IsValid()) return;

    if (!SearchSettings->SearchResults.IsValidIndex(Index))
    {
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 잘못된 세션 인덱스: %d"), Index);
        return;
    }

    const FOnlineSessionSearchResult& Result = SearchSettings->SearchResults[Index];

    // 방에 설정된 비밀번호와 입력값 비교
    FString CurrentRoomPassword;
    Result.Session.SessionSettings.Get(SETTING_ROOMPASSWORD, CurrentRoomPassword);

    // 비밀번호가 비어있으면 그냥 입장 가능
    if (!CurrentRoomPassword.IsEmpty() && CurrentRoomPassword != InputPassWord)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 비밀번호가 일치하지 않습니다. (Index: %d)"), Index);
        OnJoinPasswordIncorrect.Broadcast();
        return;
    }


    Result.Session.SessionSettings.Get(SETTING_ROOMNAME, RoomName);

    Sessions->OnJoinSessionCompleteDelegates.AddUObject(this, &UMainGameInstanceSubsystem::OnJoinSessionComplete);
    // Index에 들어온 값에따라 방 참가
    Sessions->JoinSession(Index, NAME_GameSession, Result);
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
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 비공개 방 참가: 세션 검색 실패"));
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
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 비공개 방 참가: 검색 결과가 없습니다."));
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
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 비공개 방 참가: '%s' 방을 찾을 수 없습니다."), *PrivateRoomName);
        OnPrivateRoomNotFound.Broadcast();
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[EOS] 비공개 방 참가: Index %d 방 발견, 참가 시도"), FoundIndex);

    //비밀번호 확인은 해당 함수에서 확인
    JoinFoundSession(FoundIndex, PrivateRoomPassword);
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
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 퀵매치: 세션 검색 실패"));
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

        // 비밀번호가 없는 방 존재 X
        if (BestIndex == INDEX_NONE)
        {
            UE_LOG(LogTemp, Warning, TEXT("[EOS] 퀵매치: 참가 가능한(비밀번호 없는) 방을 찾지 못했습니다."));
            OnQuickMatchNoSessionFound.Broadcast();
            return;
        }

        UE_LOG(LogTemp, Log, TEXT("[EOS] 퀵매치: Index %d 방 참가 시도 (Ping: %d ms)"), BestIndex, BestPing);

        JoinFoundSession(BestIndex, TEXT(""));
    }
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
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 나갈 세션이 없습니다. 타이틀로 이동"));
        ReturnToTitle();
        return;
    }


    //DestroySession은 비동기이므로 델리게이트 등록하여 추후에 콜백 받음
    Sessions->OnDestroySessionCompleteDelegates.AddUObject(this, &UMainGameInstanceSubsystem::OnLeaveSessionComplete);
    Sessions->DestroySession(NAME_GameSession);
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
        UE_LOG(LogTemp, Log, TEXT("[EOS] 세션 나가기 성공: %s"), *SessionName.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 세션 나가기(파괴) 실패: %s - 그래도 타이틀로 이동합니다."), *SessionName.ToString());
    }

    //방 정보 초기화
    RoomName.Empty();
    RoomPassword.Empty();
    bIsHardMode = false;

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
        UE_LOG(LogTemp, Log, TEXT("[EOS] 호스트: 타이틀 이동 "));
        World->ServerTravel(LevelPath, true);
    }
    else if (CurrentNetMode == NM_Client)
    {
        APlayerController* PC = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController(GetWorld()) : nullptr;
        if (PC)
        {
            UE_LOG(LogTemp, Warning, TEXT("[EOS] 참가자 타이틀로 이동"));
            PC->ClientTravel(LevelPath, TRAVEL_Absolute);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[EOS] PlayerController를 찾을 수 없어 타이틀로 이동할 수 없습니다."));
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
                    // 만약 주소가 EOS: 로 시작하는데 언리얼 엔진이 드라이버 매핑을 못 다룬다면,
                    // 주소 앞에 명시적으로 프로토콜 포맷(bIsUsingP2PSockets에 대응하는 포맷)을 강제해 봅니다.
                    if (!ConnectString.StartsWith(TEXT("EOS:")))
                    {
                        // 일반적인 경우
                        UE_LOG(LogTemp, Log, TEXT("[EOS] 세션 접속 시도: %s"), *ConnectString);
                        PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
                    }
                    else
                    {
                        // 언리얼 엔진의 NetDriverEOS가 주소를 명확히 인지하도록
                        // 에디터/플러그인 버그 방지용 강제 파싱 포맷 적용
                        FString ForcedEOSURL = ConnectString;

                        // 주소 체계가 "EOS:계정ID/레벨경로" 형태로 들어오는 경우
                        // 엔진 내부 드라이버 채택을 유도하기 위해 URL을 재정리하거나 그대로 태웁니다.
                        UE_LOG(LogTemp, Log, TEXT("[EOS-Forced] 강제 URL 접속 시도: %s"), *ForcedEOSURL);
                        PC->ClientTravel(ForcedEOSURL, ETravelType::TRAVEL_Absolute);
                    }
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[EOS] 접속 주소를 가져오지 못했습니다."));
            }
            break;
        }
    case EOnJoinSessionCompleteResult::SessionIsFull:
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 세션이 가득 찼습니다."));
        break;
    case EOnJoinSessionCompleteResult::SessionDoesNotExist:
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 세션이 더 이상 존재하지 않습니다."));
        break;
    case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 호스트 주소를 가져오지 못했습니다."));
        break;
    case EOnJoinSessionCompleteResult::AlreadyInSession:
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 이미 해당 세션에 참여 중입니다."));
        break;
    default:
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 알 수 없는 오류로 조인 실패."));
        break;
    }
}

FString UMainGameInstanceSubsystem::GetFoundSessionRoomName(int32 Index) const
{
    if (!SearchSettings.IsValid() || !SearchSettings->SearchResults.IsValidIndex(Index)) return FString();

    FString FoundRoomName;
    SearchSettings->SearchResults[Index].Session.SessionSettings.Get(SETTING_ROOMNAME, FoundRoomName);
    return FoundRoomName;
}

FString UMainGameInstanceSubsystem::GetFoundSessionOwnerName(int32 Index) const
{
    if (!SearchSettings.IsValid() || !SearchSettings->SearchResults.IsValidIndex(Index)) return FString();

    FString FoundOwnerName;
    SearchSettings->SearchResults[Index].Session.SessionSettings.Get(SETTING_OWNERNAME, FoundOwnerName);
    return FoundOwnerName;
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
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 방장(호스트)만 난이도를 변경할 수 있습니다."));
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
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 에디터에서 레벨이 설정되지 않았습니다."));
        return FString();
    }

    return Level.ToSoftObjectPath().GetLongPackageName();
}
