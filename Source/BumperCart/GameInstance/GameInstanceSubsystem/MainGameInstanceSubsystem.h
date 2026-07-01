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


UCLASS()
class BUMPERCART_API UMainGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    //플레이어 로그인
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    void Login(const FString& CredentialName);

    //호스트가 되어 방 생성
    //방 제목은 필수, 방 비밀번호는 선택
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    void HostListenServer(const FString& InRoomName, const FString& InRoomPassword = TEXT(""));

    //생성된 방 검색
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    void FindSessions(const FString& RoomNameFilter = TEXT(""));

    //검색된 방 참가
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    void JoinFoundSession(int32 Index, const FString& InputPassword);

    UPROPERTY(BlueprintAssignable, Category = "EOS|Session")
    FOnSessionsFoundSignature OnSessionsFound;

    UPROPERTY(BlueprintAssignable, Category = "EOS")
    FOnLoginResult OnLoginResult;

    UPROPERTY(BlueprintAssignable, Category = "EOS|Session")
    FOnJoinPasswordIncorrect OnJoinPasswordIncorrect;

    // 로그인된 유저 이름/ID를 UI에서 바로 가져다 쓸 수 있게 캐싱
    UPROPERTY(BlueprintReadOnly, Category = "EOS")
    FString CachedDisplayName;

    //검색 결과 인덱스에 해당하는 방 제목 조회
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    FString GetFoundSessionRoomName(int32 Index) const;

    //검색 결과 인덱스에 해당하는 방 소유자 이름 조회
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    FString GetFoundSessionOwnerName(int32 Index) const;

    //검색 결과 개수 조회
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
    // 세션 참가 완료 시 호출
    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    // 기존 세션 제거 성공 시 호출
    void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

    //실제 세션 생성 로직 (재시도/재생성 시에도 캐싱된 RoomName/RoomPassword 사용)
    void CreateSessionInternal();


    //인터페이스 획득 헬퍼
    TSharedPtr<FOnlineSessionSearch> SearchSettings;
    IOnlineSessionPtr GetSessionInterface() const;
    IOnlineIdentityPtr GetIdentityInterface() const;

    //방 생성 이름 및 비밀번호
    FString RoomName;
    FString RoomPassword;

    //방 검색시 사용될 방 이름
    FString SearchRoomNameFilter;

    //검색된 방 결과들
    TArray<FOnlineSessionSearchResult> FilteredResults;

    //세션 커스텀 세팅 키
    static inline const FName SETTING_ROOMNAME     = TEXT("ROOMNAME");
    static inline const FName SETTING_ROOMPASSWORD = TEXT("ROOMPW");
    static inline const FName SETTING_OWNERNAME    = TEXT("OWNERNAME");

};
