// UW_TitleLayout.cpp


#include "UI/UW_TitleLayout.h"

#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableText.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/GameInstance.h"
#include "GameInstance/GameInstanceSubsystem/MainGameInstanceSubsystem.h"



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
    if (CreateRoomConfirmButton) CreateRoomConfirmButton->OnClicked.AddDynamic(this, &UUW_TitleLayout::OnCreateRoomConfirmClicked);
    if (CreateRoomBackButton)    CreateRoomBackButton->OnClicked.AddDynamic(this, &UUW_TitleLayout::OnCreateRoomBackClicked);

    ShowCreateRoomPanel(false);

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

    if (RoomNameInputText) RoomNameInputText->SetText(FText::GetEmpty());
    if (RoomPasswordInputText) RoomPasswordInputText->SetText(FText::GetEmpty());

    ShowCreateRoomPanel(true);

    if (CreateRoomWarningText)
    {
        CreateRoomWarningText->SetText(FText::GetEmpty());
        CreateRoomWarningText->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (StatusText) StatusText->SetText(FText::FromString(TEXT("방 제목과 비밀번호를 입력하세요.")));


}

void UUW_TitleLayout::OnCreateRoomConfirmClicked()
{
    const FString InputRoomName = RoomNameInputText     ? RoomNameInputText->GetText().ToString()     : FString();
    const FString InputRoomPassword = RoomPasswordInputText ? RoomPasswordInputText->GetText().ToString() : FString();

    const bool bRoomNameEmpty = InputRoomName.IsEmpty();
    const bool bRommPasswordEmpty = InputRoomPassword.IsEmpty();

    // 제목/비밀번호 중 하나라도 비어있으면 생성하지 않고 안내 문구 표시
    if (bRoomNameEmpty || bRommPasswordEmpty)
    {
        if (CreateRoomWarningText)
        {
            FString WarningMessage;
            if (bRoomNameEmpty && bRommPasswordEmpty)
            {
                WarningMessage = TEXT("방 제목과 비밀번호를 모두 입력해주세요.");
            }
            else if (bRoomNameEmpty)
            {
                WarningMessage = TEXT("방 제목을 입력해주세요.");
            }
            else
            {
                WarningMessage = TEXT("비밀번호를 입력해주세요.");
            }

            CreateRoomWarningText->SetText(FText::FromString(WarningMessage));
            CreateRoomWarningText->SetVisibility(ESlateVisibility::Visible);
        }
        return;
    }

    if (CreateRoomWarningText)
    {
        CreateRoomWarningText->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (UMainGameInstanceSubsystem* Sub = GetGameInstance()->GetSubsystem<UMainGameInstanceSubsystem>())
    {
        Sub->HostListenServer(InputRoomName, InputRoomPassword);

        if (StatusText) StatusText->SetText(FText::FromString(TEXT("세션 생성 중...")));
    }

    // 생성 요청 후에는 메인 메뉴로 돌아가서 진행 상태(StatusText)를 보여줌
    ShowCreateRoomPanel(false);
}

void UUW_TitleLayout::OnCreateRoomBackClicked()
{
    ShowCreateRoomPanel(false);

    if (CreateRoomWarningText) CreateRoomWarningText->SetVisibility(ESlateVisibility::Collapsed);
    if (StatusText) StatusText->SetText(FText::FromString(TEXT("")));

}

void UUW_TitleLayout::ShowCreateRoomPanel(bool bShow)
{
    if (!MenuSwitcher) return;;

    MenuSwitcher->SetActiveWidgetIndex(bShow ? 1 : 0);
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
        const FString InputPassword = JoinPasswordInputText ? JoinPasswordInputText->GetText().ToString() : FString();

        Sub->JoinFoundSession(0, InputPassword); // 첫 번째 검색 결과로 조인
        if (StatusText) StatusText->SetText(FText::FromString(TEXT("세션 접속 시도 중...")));
    }
}


void UUW_TitleLayout::OnSessionsFoundHandler(int32 FoundCount)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(FString::Printf(TEXT("세션 %d개 발견됨"), FoundCount)));
    }

    RefreshSessionListUI(FoundCount);
}

void UUW_TitleLayout::RefreshSessionListUI(int32 FoundCount)
{
    if (!SessionListBox) return;

    // 이전 검색 결과로 그려둔 항목들 전부 제거
    SessionListBox->ClearChildren();

    UMainGameInstanceSubsystem* Sub = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMainGameInstanceSubsystem>() : nullptr;
    if (!Sub) return;

    for (int32 i = 0; i < FoundCount; ++i)
    {
        const FString RoomName = Sub->GetFoundSessionRoomName(i);
        const FString OwnerName = Sub->GetFoundSessionOwnerName(i);

        UTextBlock* EntryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (!EntryText) continue;

        EntryText->SetText(FText::FromString(
            FString::Printf(TEXT("[%d] %s  (방장: %s)"), i, *RoomName, *OwnerName)
        ));

        SessionListBox->AddChildToVerticalBox(EntryText);
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

