// MainGameInstanceSubsystem.h

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnlineSubsystem.h"
#include "Interfaces//OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "MainGameInstanceSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionsFoundSignature, int32, FoundCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLoginResult, bool, bWasSuccessful, const FString&, ErrorMessage);


UCLASS()
class BUMPERCART_API UMainGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    void Login(const FString& CredentialName);

    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    void HostListenServer();

    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    void FindSessions();

    UFUNCTION(BlueprintCallable, Category = "EOS|Session")
    void JoinFoundSession(int32 Index);

    UPROPERTY(BlueprintAssignable, Category = "EOS|Session")
    FOnSessionsFoundSignature OnSessionsFound;

    UPROPERTY(BlueprintAssignable, Category = "EOS")
    FOnLoginResult OnLoginResult;

    // 로그인된 유저 이름/ID를 UI에서 바로 가져다 쓸 수 있게 캐싱
    UPROPERTY(BlueprintReadOnly, Category = "EOS")
    FString CachedDisplayName;

private:
    void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
    void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void OnFindSessionComplete(bool bWasSuccessful);
    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

    TSharedPtr<FOnlineSessionSearch> SearchSettings;
    IOnlineSessionPtr GetSessionInterface() const;
    IOnlineIdentityPtr GetIdentityInterface() const;

};
