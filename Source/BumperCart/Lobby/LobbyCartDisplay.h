//BumperCart - B(카트/플레이어 조작) 파트
//로비 전시용 카트 — 슬롯 플레이어가 선택한 캐릭터 색을 실시간 표시 (레벨 배치 후 PlayerSlotIndex만 지정)

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

	//이 전시 카트가 나타내는 플레이어 슬롯(입장 순서). 그 플레이어의 선택 색을 표시
	UPROPERTY(EditAnywhere, Category = "Lobby")
	int32 PlayerSlotIndex = 0;

private:
	//LobbyGameState의 변경 델리게이트에 바인딩 (GameState 복제 전이면 타이머로 재시도)
	void BindLobbyState();

	//슬롯 플레이어의 선택 색으로 갱신 (빈 슬롯/미선택이면 기본색)
	UFUNCTION()
	void RefreshColor();

	//머티리얼 원래 CartColor (빈 슬롯 복원용, BeginPlay에서 캐시)
	FLinearColor DefaultColor = FLinearColor::White;

	FTimerHandle BindRetryHandle;
};
