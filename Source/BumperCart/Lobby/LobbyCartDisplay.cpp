//BumperCart - B(카트/플레이어 조작) 파트

#include "Lobby/LobbyCartDisplay.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameInstance/MainGameInstance.h"
#include "DataAsset/CharacterSelectionConfig.h"
#include "Materials/MaterialInstanceDynamic.h"
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
}

//캐릭터 색을 전 슬롯 MID로 적용 (인게임 카트와 동일 방식, 파라미터 없는 슬롯은 무동작)
void ALobbyCartDisplay::BeginPlay()
{
	Super::BeginPlay();

	const UMainGameInstance* GI = GetGameInstance<UMainGameInstance>();
	if (!GI || !GI->CharacterSelectionConfig)
	{
		return;
	}

	const FLinearColor Color = GI->CharacterSelectionConfig->GetColor(CharacterIndex);
	const int32 NumMaterials = CartMesh->GetNumMaterials();
	for (int32 i = 0; i < NumMaterials; ++i)
	{
		if (UMaterialInstanceDynamic* MID = CartMesh->CreateAndSetMaterialInstanceDynamic(i))
		{
			MID->SetVectorParameterValue(FName("CartColor"), Color);
		}
	}
}
