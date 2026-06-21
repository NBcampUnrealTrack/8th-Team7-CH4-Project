#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CheckoutManager.generated.h"

class ACheckoutZone;

UCLASS(Blueprintable)
class BUMPERCART_API ACheckoutManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ACheckoutManager();

protected:
	virtual void BeginPlay() override;

// ------------------------------------------------------------
// 계산대 목록
// ------------------------------------------------------------
private:
    // Manager가 관리할 계산대 목록
    UPROPERTY(EditInstanceOnly, Category = "_CheckoutManager|CheckoutZones")
    TArray<TObjectPtr<ACheckoutZone>> CheckoutZones;

// ------------------------------------------------------------
// 계산대 세팅
// ------------------------------------------------------------
private:
    // 계산대 초기 세팅
    bool InitializeCheckoutZones();
};
