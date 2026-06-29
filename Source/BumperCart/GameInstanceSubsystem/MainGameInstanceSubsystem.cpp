// MainGameInstanceSubsystem.cpp

#include "GameInstanceSubsystem/MainGameInstanceSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Engine/Engine.h"




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

    Credentials.Type = TEXT("Developer");
    Credentials.Id = TEXT("Localhost:7777");
    Credentials.Token = CredentialName;


    //패키징 테스트용
    /*
    Credentials.Type = TEXT("AccountPortal");
    Credentials.Id = TEXT("");
    Credentials.Token = TEXT("");
    */

    UE_LOG(LogTemp, Log, TEXT("[EOS] 로그인 시도 - Credential: %s"), *CredentialName);

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
    RoomPassword = InRoomPassword;

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
    SessionSettings.bUsesPresence         = false;  // 친구 초대/프레즌스
    SessionSettings.bAllowInvites         = true;
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

   //?listen이 핵심 해당 클라이언트가 서버가 되는 부분
    GetWorld()->ServerTravel(TEXT("/Game/Developers/LSJae/Levels/TestLobbyLevel?listen"));
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

//만들어진 세션 찾기
void UMainGameInstanceSubsystem::FindSessions()
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid()) return;


    SearchSettings = MakeShareable(new FOnlineSessionSearch());
    SearchSettings->MaxSearchResults = 50;
    SearchSettings->bIsLanQuery = false;
    SearchSettings->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
    SearchSettings->QuerySettings.Set(SEARCH_KEYWORDS, FString(TEXT("MyRoom")), EOnlineComparisonOp::Equals);

    Sessions->OnFindSessionsCompleteDelegates.AddUObject(this, &UMainGameInstanceSubsystem::OnFindSessionComplete);
    Sessions->FindSessions(0, SearchSettings.ToSharedRef());
}

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

    UE_LOG(LogTemp, Log, TEXT("[EOS] 세션 %d개 발견"), SearchSettings->SearchResults.Num());

    for (int32 i = 0; i < SearchSettings->SearchResults.Num(); ++i)
    {
        const FOnlineSessionSearchResult& Result = SearchSettings->SearchResults[i];
        UE_LOG(LogTemp, Log, TEXT("  [%d] %s (Ping: %d)"), i, *Result.Session.OwningUserName, Result.PingInMs);
    }

    //UI에 결과 전달 필요 시 해당 부분에 작성
    OnSessionsFound.Broadcast(SearchSettings->SearchResults.Num());
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
    FString StoredPassword;
    Result.Session.SessionSettings.Get(SETTING_ROOMPASSWORD, StoredPassword);

    if (!StoredPassword.IsEmpty() && StoredPassword != InputPassWord)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EOS] 비밀번호가 일치하지 않습니다. (Index: %d)"), Index);
        OnJoinPasswordIncorrect.Broadcast();
        return;
    }

    Sessions->OnJoinSessionCompleteDelegates.AddUObject(this, &UMainGameInstanceSubsystem::OnJoinSessionComplete);
    Sessions->JoinSession(0, NAME_GameSession, Result);
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
                    // 💡 만약 주소가 EOS: 로 시작하는데 언리얼 엔진이 드라이버 매핑을 못 다룬다면,
                    // 주소 앞에 명시적으로 프로토콜 포맷(bIsUsingP2PSockets에 대응하는 포맷)을 강제해 봅니다.
                    if (!ConnectString.StartsWith(TEXT("EOS:")))
                    {
                        // 일반적인 경우
                        UE_LOG(LogTemp, Log, TEXT("[EOS] 세션 접속 시도: %s"), *ConnectString);
                        PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
                    }
                    else
                    {
                        // 💡 핵심: 언리얼 엔진의 NetDriverEOS가 주소를 명확히 인지하도록
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
