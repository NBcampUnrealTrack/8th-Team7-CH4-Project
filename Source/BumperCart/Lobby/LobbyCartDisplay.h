//BumperCart - B(카트/플레이어 조작) 파트
//로비 전시용 카트 — 캐릭터 색을 입혀 보여주는 장식 액터 (레벨 배치 후 CharacterIndex만 지정)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LobbyCartDisplay.generated.h"

class UStaticMeshComponent;

UCLASS()
class BUMPERCART_API ALobbyCartDisplay : public AActor
{
	GENERATED_BODY()

public:
	ALobbyCartDisplay();

protected:
	virtual void BeginPlay() override;

	//전시 메시 (기본: SM_Shopping_Cart, 인스턴스에서 교체 가능)
	UPROPERTY(VisibleAnywhere, Category = "Lobby")
	TObjectPtr<UStaticMeshComponent> CartMesh;

	//이 전시 카트가 나타내는 캐릭터 번호 — CharacterSelectionConfig의 색을 입힘
	UPROPERTY(EditAnywhere, Category = "Lobby")
	int32 CharacterIndex = 0;
};
