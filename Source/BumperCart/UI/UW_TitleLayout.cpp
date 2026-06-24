// UW_TitleLayout.cpp


#include "UI/UW_TitleLayout.h"

#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableText.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "GameInstanceSubsystem/MainGameInstanceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"



UUW_TitleLayout::UUW_TitleLayout(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UUW_TitleLayout::NativeOnInitialized()
{
    Super::NativeOnInitialized();


    if (PlayerSelectCombo)
    {
        PlayerSelectCombo->AddOption(TEXT("Player1"));
        PlayerSelectCombo->AddOption(TEXT("Player2"));
        PlayerSelectCombo->AddOption(TEXT("Player3"));
        PlayerSelectCombo->AddOption(TEXT("Player4"));
        PlayerSelectCombo->SetSelectedIndex(0);
    }


    if (LoginButton) LoginButton->OnClicked.AddDynamic(this, &UUW_TitleLayout::OnLoginClicked);
    if (HostButton)  HostButton->OnClicked.AddDynamic(this, &UUW_TitleLayout::OnHostClicked);
    if (FindButton)  FindButton->OnClicked.AddDynamic(this, &UUW_TitleLayout::OnFindClicked);
    if (JoinButton)  JoinButton->OnClicked.AddDynamic(this, &UUW_TitleLayout::OnJoinClicked);

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UMainGameInstanceSubsystem* Sub = GI->GetSubsystem<UMainGameInstanceSubsystem>())
        {
            Sub->OnSessionsFound.AddDynamic(this, &UUW_TitleLayout::OnSessionsFoundHandler);
            Sub->OnLoginResult.AddDynamic(this, &UUW_TitleLayout::HandleLoginResult);
        }
    }
}


void UUW_TitleLayout::OnLoginClicked()
{
    if (UMainGameInstanceSubsystem* Sub = GetGameInstance()->GetSubsystem<UMainGameInstanceSubsystem>())
    {
        FString SelectedName = PlayerSelectCombo ? PlayerSelectCombo->GetSelectedOption() : TEXT("Player1");
        Sub->Login(SelectedName);

        if (StatusText)
        {
            StatusText->SetText(FText::FromString(TEXT("로그인 시도 중...")));
        }
    }
}


void UUW_TitleLayout::OnHostClicked()
{

    //const FName LobbyLevelName = FName(TEXT("TestLobbyLevel"));
    //UGameplayStatics::OpenLevel(GetWorld(), LobbyLevelName, true, TEXT("Listen"));

    if (UMainGameInstanceSubsystem* Sub = GetGameInstance()->GetSubsystem<UMainGameInstanceSubsystem>())
    {
        Sub->HostListenServer();
        if (StatusText) StatusText->SetText(FText::FromString(TEXT("세션 생성 중...")));
    }

}

void UUW_TitleLayout::OnFindClicked()
{
    if (UMainGameInstanceSubsystem* Sub = GetGameInstance()->GetSubsystem<UMainGameInstanceSubsystem>())
    {
        Sub->FindSessions();
        if (StatusText) StatusText->SetText(FText::FromString(TEXT("세션 검색 중...")));
    }
}

void UUW_TitleLayout::OnJoinClicked()
{
    /*
    FString IPAddress = ServerIPEditableText->GetText().ToString();
    if (IPAddress.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("IP is empty"));
        return;
    }

    APlayerController* PC = GetOwningPlayer<APlayerController>();
    if (IsValid(PC) == true)
    {
        PC->ClientTravel(IPAddress, TRAVEL_Absolute);
    }
    */

    if (UMainGameInstanceSubsystem* Sub = GetGameInstance()->GetSubsystem<UMainGameInstanceSubsystem>())
    {
        Sub->JoinFoundSession(0); // 첫 번째 검색 결과로 조인
        if (StatusText) StatusText->SetText(FText::FromString(TEXT("세션 접속 시도 중...")));
    }
}


void UUW_TitleLayout::OnSessionsFoundHandler(int32 FoundCount)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(FString::Printf(TEXT("세션 %d개 발견됨"), FoundCount)));
    }
}

void UUW_TitleLayout::HandleLoginResult(bool bWasSuccessful, const FString& Message)
{
    if (!StatusText) return;

    if (bWasSuccessful)
    {
        StatusText->SetText(FText::FromString(FString::Printf(TEXT("로그인 성공! (%s)"), *Message)));
    }
    else
    {
        StatusText->SetText(FText::FromString(FString::Printf(TEXT("로그인 실패: %s"), *Message)));
    }
}

