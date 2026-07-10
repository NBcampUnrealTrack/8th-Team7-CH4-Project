//BumperCart - B(카트/플레이어 조작) 파트

#include "Lobby/LobbyCartDisplay.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameInstance/MainGameInstance.h"
#include "GameState/LobbyGameState.h"
#include "DataAsset/CharacterSelectionConfig.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ALobbyCartDisplay::ALobbyCartDisplay()
{
	PrimaryActorTick.bCanEverTick = false;

	CartMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CartMesh"));
	SetRootComponent(CartMesh);
	CartMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//기본 메시 = 카트 본체 (레벨 인스턴스에서 교체 가능)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/Game/Developers/dongh/Cart/Mesh/SM_Shopping_Cart.SM_Shopping_Cart"));
	if (MeshFinder.Succeeded())
	{
		CartMesh->SetStaticMesh(MeshFinder.Object);
	}

	//인게임 카트(BP_CartPawn)와 같은 머티리얼로 통일 — 메시 기본 머티리얼엔 CartColor 파라미터가 없음
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Game/Developers/dongh/Cart/Materials/MI_Shopping_Cart_Custom.MI_Shopping_Cart_Custom"));
	if (MeshFinder.Succeeded() && MaterialFinder.Succeeded())
	{
		for (int32 i = 0; i < CartMesh->GetNumMaterials(); ++i)
		{
			CartMesh->SetMaterial(i, MaterialFinder.Object);
		}
	}
}

void ALobbyCartDisplay::BeginPlay()
{
	Super::BeginPlay();

	//빈 슬롯 복원용 기본색 캐시 (MID 만들기 전 원본 머티리얼에서)
	if (const UMaterialInterface* BaseMaterial = CartMesh->GetMaterial(0))
	{
		BaseMaterial->GetVectorParameterValue(FHashedMaterialParameterInfo(FName("CartColor")), DefaultColor);
	}

	BindLobbyState();
}

//LobbyGameState 변경 델리게이트에 바인딩 — 클라는 GameState 복제 전일 수 있어 잡힐 때까지 재시도
void ALobbyCartDisplay::BindLobbyState()
{
	ALobbyGameState* GS = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr;
	if (!GS)
	{
		GetWorldTimerManager().SetTimer(BindRetryHandle, this, &ALobbyCartDisplay::BindLobbyState, 0.2f, false);
		return;
	}

	GS->OnLobbyPlayersChanged.AddDynamic(this, &ALobbyCartDisplay::RefreshColor);
	RefreshColor();
}

//슬롯 플레이어의 선택 색으로 전 슬롯 MID 갱신 (빈 슬롯/미선택이면 기본색)
void ALobbyCartDisplay::RefreshColor()
{
	const ALobbyGameState* GS = GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr;
	const UMainGameInstance* GI = GetGameInstance<UMainGameInstance>();
	if (!GS || !GI || !GI->CharacterSelectionConfig)
	{
		return;
	}

	FLinearColor Color = DefaultColor;
	const TArray<FLobbyPlayerInfo>& Infos = GS->GetReplicatedPlayerInfos();
	if (Infos.IsValidIndex(PlayerSlotIndex) && Infos[PlayerSlotIndex].SelectedCharacterIndex != INDEX_NONE)
	{
		Color = GI->CharacterSelectionConfig->GetColor(Infos[PlayerSlotIndex].SelectedCharacterIndex);
	}

	const int32 NumMaterials = CartMesh->GetNumMaterials();
	for (int32 i = 0; i < NumMaterials; ++i)
	{
		if (UMaterialInstanceDynamic* MID = CartMesh->CreateAndSetMaterialInstanceDynamic(i))
		{
			MID->SetVectorParameterValue(FName("CartColor"), Color);
		}
	}
}
