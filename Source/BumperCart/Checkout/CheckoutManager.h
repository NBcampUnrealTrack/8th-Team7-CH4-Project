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
// 계산대 목록 및 세팅
// ------------------------------------------------------------

private:
    // 계산대 초기 세팅
    bool InitializeCheckoutZones();

private:
    // Manager가 관리할 계산대 목록
    UPROPERTY(EditInstanceOnly, Category = " CheckoutManager|CheckoutZones")
    TArray<TObjectPtr<ACheckoutZone>> CheckoutZones;

// ------------------------------------------------------------
// 계산대 상태 변경
// ------------------------------------------------------------
private:
    // 정산 완료된 계산대 처리
    void HandleCheckoutCompleted(ACheckoutZone* CompletedCheckoutZone);

public:
    // 다음 턴에 열릴 계산대를 랜덤 선택
    // 닫힐 계산대는 ClosingSoon 적용
    UFUNCTION(BlueprintCallable, Category = " CheckoutManager|State")
    void PrepareNextCheckoutZoneState(int32 OpenCount);

    // Open할 계산대 랜덤 선택
    TArray<TObjectPtr<ACheckoutZone>> SelectRandomOpenCheckoutZones(int32 OpenCount) const;

    // 다음 턴에 적용할 계산대 상태
    void ChangeNextCheckoutZoneState();

private:
    // 상태 변경 후 랜덤 오픈할 계산대 목록
    UPROPERTY(VisibleAnywhere)
    TArray<TObjectPtr<ACheckoutZone>> RandomOpenCheckoutZones;

    // Closing Soon 시간
    UPROPERTY(EditAnywhere, Category=" CheckoutManager|State")
    float ClosingSoonDuration = 10.0f;

    // Closing Soon 타이머
    FTimerHandle ClosingSoonTimerHandle;
};
