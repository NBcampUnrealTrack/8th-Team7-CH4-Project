// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LobbyGameMode.h"

#include "GameFramework/GameSession.h"
#include "GameInstance/MainGameInstance.h"
#include "GameInstance/GameInstanceSubsystem/MainGameInstanceSubsystem.h"
#include "GameState/LobbyGameState.h"
#include "PlayerState/LobbyPlayerState.h"

ALobbyGameMode::ALobbyGameMode()
{
    GameStateClass = ALobbyGameState::StaticClass();
    PlayerStateClass = ALobbyPlayerState::StaticClass();

    bUseSeamlessTravel = true;
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    if (HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("플레이어 접속: %s"), *NewPlayer->GetName());

        // 온라인 세션에 실제로 등록
        if (GameSession && NewPlayer && NewPlayer->PlayerState)
        {
            const FUniqueNetIdRepl& PlayerUniqueId = NewPlayer->PlayerState->GetUniqueId();
            if (PlayerUniqueId.IsValid())
            {
                GameSession->RegisterPlayer(NewPlayer, PlayerUniqueId, false);
                UE_LOG(LogTemp, Warning, TEXT("[Lobby] 세션에 플레이어 등록: %s"), *NewPlayer->GetName());

                // 세션 인원 등록 후 서버에 변경 내역 업데이트
                if (UMainGameInstance* GI = GetGameInstance<UMainGameInstance>())
                {
                    if (UMainGameInstanceSubsystem* Subsystem = GI->GetSubsystem<UMainGameInstanceSubsystem>())
                    {
                        Subsystem->UpdateCurrentSession();
                    }
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[Lobby] %s 등록 실패"), *NewPlayer->GetName());
            }
        }
    }

    ALobbyGameState* GS = GetGameState<ALobbyGameState>();
    if (!GS) return;

    // 로비 입장 순서대로 아직 아무도 선택하지 않은 캐릭터를 자동 배정
    if (ALobbyPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<ALobbyPlayerState>() : nullptr)
    {
        const int32 CharacterIndex = GS->GetNextAvailableCharacterIndex();
        if (CharacterIndex != INDEX_NONE)
        {
            PS->SelectCharacter(CharacterIndex);
            UE_LOG(LogTemp, Warning, TEXT("[Lobby] %s 에게 캐릭터 index %d 자동 배정"), *NewPlayer->GetName(), CharacterIndex);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[Lobby] %s 에게 배정 가능한 캐릭터가 없습니다."), *NewPlayer->GetName());
        }
    }



    // 로비 입장 순서대로 index 부여
    if (ALobbyPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<ALobbyPlayerState>() : nullptr)
    {
        if (UMainGameInstance* GI = GetGameInstance<UMainGameInstance>())
        {
            // 이미 배정된 적 있는지 확인
            int32 LobbyIndex = GI->GetPlayerIndex(PS->GetUniqueId());

            // 없으면 새로 배정
            if (LobbyIndex == INDEX_NONE)
            {
                LobbyIndex = GI->GetNextLobbyIndex();
                GI->SetPlayerIndex(PS->GetUniqueId(), LobbyIndex);
                UE_LOG(LogTemp, Warning, TEXT("[Lobby] %s 에게 새로 배정 index = %d"), *NewPlayer->GetName(), LobbyIndex);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[Lobby] %s 에게 배정된 index = %d"), *NewPlayer->GetName(), LobbyIndex);
            }

            GI->LogAllPlayerIndices();
        }
    }

    GS->RefreshPlayerInfos();
}

void ALobbyGameMode::Logout(AController* Exiting)
{
    APlayerState* ExitingPS = Exiting ? Exiting->PlayerState : nullptr;

    // 온라인 세션에서 플레이어 등록 해제 / PlayerState가 살아있는 Super::Logout 전에 호출해야함
    if (HasAuthority() && GameSession && Exiting)
    {
        if (APlayerController* ExitingPC = Cast<APlayerController>(Exiting))
        {
            GameSession->UnregisterPlayer(ExitingPC);
            UE_LOG(LogTemp, Warning, TEXT("[Lobby] 세션에서 플레이어 등록 해제: %s"), *Exiting->GetName());

            // 세션 인원 해제 후 서버에 변경 내역 업데이트
            if (UMainGameInstance* GI = GetGameInstance<UMainGameInstance>())
            {
                if (UMainGameInstanceSubsystem* Subsystem = GI->GetSubsystem<UMainGameInstanceSubsystem>())
                {
                    Subsystem->UpdateCurrentSession();
                }
            }
        }
    }

    Super::Logout(Exiting);
    UE_LOG(LogTemp, Warning, TEXT("플레이어 퇴장: %s"), *Exiting->GetName());

    // 나간 플레이어의 로비 index 정보를 삭제하고, 뒤 순번들을 당김
    if (ExitingPS)
    {
        if (UMainGameInstance* GI = GetGameInstance<UMainGameInstance>())
        {
            GI->RemovePlayerIndex(ExitingPS->GetUniqueId());

            GI->LogAllPlayerIndices();
        }
    }

    if (ALobbyGameState* GS = GetGameState<ALobbyGameState>())
    {
        GS->RefreshPlayerInfos(ExitingPS);
    }
}

void ALobbyGameMode::BeginPlay()
{
    Super::BeginPlay();

}

//게임 시작(게임 맵으로 이동)
void ALobbyGameMode::StartGame()
{
    if (!HasAuthority()) return;

    ALobbyGameState* GS = GetGameState<ALobbyGameState>();
    if (!GS || GS->PlayerArray.Num() < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Lobby] 인원이 부족하여 게임을 시작할 수 없습니다. 현재 인원: %d"),
            GS ? GS->PlayerArray.Num() : 0);
        return;
    }

    //모든 플레이어 준비완료 확인
    if (!GS->bIsAllPlayersReady())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Lobby] 아직 모든 플레이어가 준비 완료 상태가 아니므로 게임을 시작할 수 없습니다."));
        return;
    }

    if (UMainGameInstance* GI = GetGameInstance<UMainGameInstance>())
    {
        GI->SetExpectedPlayerCount(GS->PlayerArray.Num());
    }

    UMainGameInstance* MainGI = Cast<UMainGameInstance>(GetGameInstance());
    if (!MainGI) return;

    const FString LevelPath = MainGI->GamePlayLevel.ToSoftObjectPath().GetLongPackageName();

    GetWorld()->ServerTravel(LevelPath);
}
