#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CheckoutManager.generated.h"

class ACheckoutZone;

// GameState에서 라운드 변경될 때마다 PrepareNextCheckoutZoneState(int32 OpenCount) 호출해야 됨
// 이 함수는 계산대의 상태를 즉시 변경하지 않고, 10초 뒤에 변경하기 때문에
// 첫 호출을 20초에 시작하고, 30초 간격으로 반복 호출해야 됨


UCLASS(Blueprintable)
class BUMPERCART_API ACheckoutManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ACheckoutManager();

protected:
	virtual void BeginPlay() override;

// ------------------------------------------------------------
// 차단벽 설정
// ------------------------------------------------------------
public:
    // 모든 계산대에 차단벽 사용 여부 적용
    UFUNCTION(BlueprintCallable, Category = "CheckoutManager|Barrier")
    void SetUseCheckoutBarrier(bool bUseBarrier);

    UFUNCTION(BlueprintPure, Category = "CheckoutManager|Barrier")
    bool IsUsingCheckoutBarrier() const;

private:
    // 게임 시작 전 기본 설정값
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CheckoutManager|Barrier",meta = (AllowPrivateAccess = "true"))
    bool bUseCheckoutBarrier = true;

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
