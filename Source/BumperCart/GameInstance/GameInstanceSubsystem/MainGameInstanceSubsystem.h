// MainGameInstanceSubsystem.h

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "MainGameInstanceSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionsFoundSignature, int32, FoundCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLoginResult, bool, bWasSuccessful, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJoinPasswordIncorrect);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLeaveSessionResult, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuickMatchNoSessionFound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPrivateRoomNotFound);



UCLASS(Blueprintable)
class BUMPERCART_API UMainGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    // 플레이어 로그인
    UFUNCTION(BlueprintCallable, Category = "Online|Session")
    void Login(const FString& CredentialName);

    // 호스트가 되어 방 생성
    // 방 제목은 필수, 방 비밀번호는 선택
    UFUNCTION(BlueprintCallable, Category = "Online|Session")
    void HostListenServer(const FString& InRoomName, const FString& InRoomPassword = TEXT(""));

    // 생성된 방 검색
    UFUNCTION(BlueprintCallable, Category = "Online|Session")
    void FindSessions(const FString& RoomNameFilter = TEXT(""));

    // 검색된 방 참가
    UFUNCTION(BlueprintCallable, Category = "Online|Session")
    void JoinFoundSession(int32 Index, const FString& InputPassword);

    // 비공개 방 참가
    UFUNCTION(BlueprintCallable, Category = "Online|Session")
    void JoinPrivateRoomByName(const FString& InRoomName, const FString& InRoomPassword);

    // 방 나가기
    UFUNCTION(BlueprintCallable, Category = "Online|Session")
    void LeaveSession();

    // 퀵 매치
    UFUNCTION(BlueprintCallable, Category = "Online|Session")
    void QuickMatch();

    // 세션 검색 처리 결과
    UPROPERTY(BlueprintAssignable, Category = "Online|Session")
    FOnSessionsFoundSignature OnSessionsFound;

    // 로그인 결과 나올 시 브로드 캐스트
    UPROPERTY(BlueprintAssignable, Category = "Online")
    FOnLoginResult OnLoginResult;

    // 방 참가 시 비밀번호 오입력 시 브로드 캐스트
    UPROPERTY(BlueprintAssignable, Category = "Online|Session")
    FOnJoinPasswordIncorrect OnJoinPasswordIncorrect;

    // 세션 나가기 성공했을 때 브로드캐스트
    UPROPERTY(BlueprintAssignable, Category = "Online|Session")
    FOnLeaveSessionResult OnLeaveSessionResult;

    // 퀵매치 시 참가 가능한 방이 없을 때 브로드캐스트
    UPROPERTY(BlueprintAssignable, Category = "Online|Session")
    FOnQuickMatchNoSessionFound OnQuickMatchNoSessionFound;

    // 비공개 방 참가 시 해당 방 제목의 방을 찾지 못했을 때 브로드캐스트
    UPROPERTY(BlueprintAssignable, Category = "Online|Session")
    FOnPrivateRoomNotFound OnPrivateRoomNotFound;

    // 로그인된 유저 이름/ID를 UI에서 바로 가져다 쓸 수 있게 캐싱
    UPROPERTY(BlueprintReadOnly, Category = "Online")
    FString CachedDisplayName;

    // 검색 결과 인덱스에 해당하는 방 제목 조회
    UFUNCTION(BlueprintCallable, Category = "Online|Session")
    FString GetFoundSessionRoomName(int32 Index) const;

    // 검색 결과 인덱스에 해당하는 방 소유자 이름 조회
    UFUNCTION(BlueprintCallable, Category = "Online|Session")
    FString GetFoundSessionOwnerName(int32 Index) const;

    // 검색 결과 인덱스에 해당하는 방 인원 조회
    UFUNCTION(BlueprintCallable, Category = "Online|Session")
    int32 GetFoundSessionUserCount(int32 Index) const;

    // 비공개 방인지 아닌지 조회
    UFUNCTION(BlueprintCallable, Category = "Online|Session")
    bool GetFoundSessionIsPrivate(int32 Index) const;

    // 검색 결과 개수 조회
    UFUNCTION(BlueprintCallable, Category = "Online|Session")
    int32 GetFoundSessionCount() const;

    // 현재 참가한 방의 이름 조회
    UFUNCTION(BlueprintCallable, Category = "Online|Session")
    FString GetRoomName() const { return RoomName; }

    // 클라이언트가 호스트한테 나가라라는 알림을 받았을 때 해당 PlayerController 호출
    void NotifyLeaveRequestedByHost();
    // 알림을 받은 클라이언트가 나갔다는 ACK를 받았을 때 PlayerController 호출
    void OnClientAckLeftSession(APlayerController* FromPC);

private:
    // 로그인 완료 시 호출
    void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
    // 세션 생성 완료 시 호출
    void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    // 세션 검색 완료 시 호출
    void OnFindSessionComplete(bool bWasSuccessful);
    // 세션 검색 완료 시 호출 (퀵매치 전용)
    void OnQuickMatchFindSessionComplete(bool bWasSuccessful);
    // 세션 검색 완료 시 호출 (비공개 방 참가 전용)
    void OnPrivateJoinFindSessionComplete(bool bWasSuccessful);
    // 세션 참가 완료 시 호출
    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    // 기존 세션 제거 성공 시 호출
    void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
    // 세션 나가기 완료 시 호출
    void OnLeaveSessionComplete(FName SessionName, bool bWasSuccessful);
    // 퀵 매치 진행
    void TryJoinQuickMatchSession();
    // 비공개 방 참가 시도
    void TryJoinPrivateSession();
    // 세션 참가 공통 처리 (비밀번호 검증 + JoinSession) — UI 조인/퀵매치/비공개방 공용
    void JoinSessionInternal(const FOnlineSessionSearchResult& Result, const FString& InputPassword);


    // 호스트 나가지 전, 클라이어트에게 먼저 나가라고 알림
    void NotifyClientsToLeaveBeforeHostDestroy();
    // 실제로 호스트 세션 파괴
    void DestroyHostSession();
    // ACK 없이 클라이언트 연결이 끊겼을 때 대기 목록에서 안전하게 제거
    UFUNCTION()
    void OnNotifiedClientDestroyed(AActor* DestroyedActor);

    // 실제 세션 생성 로직 (재시도/재생성 시에도 캐싱된 RoomName/RoomPassword 사용)
    void CreateSessionInternal();

    // 공통으로 사용하는 검색 세팅 설정
    void CreateSearchSettings();

    // 타이틀 화면으로 복귀
    void ReturnToTitle();


    FString GetLevelPath(const TSoftObjectPtr<UWorld>& Level) const;

    // 인터페이스 획득 헬퍼
    TSharedPtr<FOnlineSessionSearch> SearchSettings;
    IOnlineSessionPtr GetSessionInterface() const;
    IOnlineIdentityPtr GetIdentityInterface() const;

    // Steam을 못 잡아 Null로 폴백된 상태인지 (에디터 PIE 등 => LAN 모드로 로컬 테스트)
    bool IsUsingNullFallback() const;

    // 방 생성 시 사용되는 이름 및 비밀번호
    FString RoomName;
    FString RoomPassword;

    // 방 검색 시 사용될 방 이름
    FString SearchRoomNameFilter;

    // 비공개 방 검색 시 사용되는 방 이름 및 비밀번호
    FString PrivateRoomName;
    FString PrivateRoomPassword;

    // 진행 중인 검색이 퀵 매치를 통한 요청인지 확인
    bool bIsQuickMatchRequest;

    // 검색된 방 결과들
    TArray<FOnlineSessionSearchResult> FilteredResults;

    // 호스트가 나갈 때, ACK을 기다려야 하는 클라이언트 목록
    TSet<TWeakObjectPtr<APlayerController>> PendingHostLeaveAcks;

    // DestroyHostSession 중복 호출 방지
    bool bHostDestroyInProgress = false;

    // 호스트 알림으로 나가는 중이면 true. 세션 파괴 완료 시 호스트에게 ACK을 보내야 함을 표시.
    bool bShouldAckHostOnLeaveComplete = false;

    // ACK 대기용 타이머)
    FTimerHandle HostLeaveWaitTimerHandle;

    // 세션 커스텀 세팅 키
    static inline const FName SETTING_ROOMNAME     = TEXT("ROOMNAME");
    static inline const FName SETTING_ROOMPASSWORD = TEXT("ROOMPW");
    static inline const FName SETTING_OWNERNAME    = TEXT("OWNERNAME");

#pragma region RoomSettings
public:
    UFUNCTION()
    void UpdateRoomDifficulty(bool bHardMode);

    UFUNCTION(BlueprintPure, Category = "Room|Settings")
    bool GetRoomDifficulty() const { return bIsHardMode; }

private:
    bool bIsHardMode = false;


#pragma endregion

#pragma region Steam Login
public:
    void StartAutoLoginRetry(float IntervalSeconds);

    void StopAutoLoginRetry();

    virtual void Deinitialize() override;

private:
    UFUNCTION()
    void HandleAutoLoginResult(bool bWasSuccessful, const FString& ErrorMessage);

    void AutoLoginTick();

    FTimerHandle LoginRetryTimerHandle;
    bool bAutoLoginRetryActive = false;
    bool bSteamLaunchURLSent = false;
    bool bIsLoginRequestPending = false;
#pragma endregion
};
