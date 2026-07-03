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
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    void Login(const FString& CredentialName);

    // 호스트가 되어 방 생성
    // 방 제목은 필수, 방 비밀번호는 선택
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    void HostListenServer(const FString& InRoomName, const FString& InRoomPassword = TEXT(""));

    // 생성된 방 검색
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    void FindSessions(const FString& RoomNameFilter = TEXT(""));

    // 검색된 방 참가
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    void JoinFoundSession(int32 Index, const FString& InputPassword);

    // 비공개 방 참가
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    void JoinPrivateRoomByName(const FString& InRoomName, const FString& InRoomPassword);

    // 방 나가기
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    void LeaveSession();

    // 퀵 매치
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    void QuickMatch();

    // 세션 검색 처리 결과
    UPROPERTY(BlueprintAssignable, Category = "EOS|Session")
    FOnSessionsFoundSignature OnSessionsFound;

    // 로그인 결과 나올 시 브로드 캐스트
    UPROPERTY(BlueprintAssignable, Category = "EOS")
    FOnLoginResult OnLoginResult;

    // 방 참가 시 비밀번호 오입력 시 브로드 캐스트
    UPROPERTY(BlueprintAssignable, Category = "EOS|Session")
    FOnJoinPasswordIncorrect OnJoinPasswordIncorrect;

    // 세션 나가기 성공했을 때 브로드캐스트
    UPROPERTY(BlueprintAssignable, Category = "EOS|Session")
    FOnLeaveSessionResult OnLeaveSessionResult;

    // 퀵매치 시 참가 가능한 방이 없을 때 브로드캐스트
    UPROPERTY(BlueprintAssignable, Category = "EOS|Session")
    FOnQuickMatchNoSessionFound OnQuickMatchNoSessionFound;

    // 비공개 방 참가 시 해당 방 제목의 방을 찾지 못했을 때 브로드캐스트
    UPROPERTY(BlueprintAssignable, Category = "EOS|Session")
    FOnPrivateRoomNotFound OnPrivateRoomNotFound;

    // 로그인된 유저 이름/ID를 UI에서 바로 가져다 쓸 수 있게 캐싱
    UPROPERTY(BlueprintReadOnly, Category = "EOS")
    FString CachedDisplayName;

    // 검색 결과 인덱스에 해당하는 방 제목 조회
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    FString GetFoundSessionRoomName(int32 Index) const;

    // 검색 결과 인덱스에 해당하는 방 소유자 이름 조회
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    FString GetFoundSessionOwnerName(int32 Index) const;

    // 검색 결과 개수 조회
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    int32 GetFoundSessionCount() const;

    // 현재 참가한 방의 이름 조회
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    FString GetRoomName() const { return RoomName; }

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
};
