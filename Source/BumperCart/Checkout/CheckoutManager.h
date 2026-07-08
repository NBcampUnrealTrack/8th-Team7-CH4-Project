#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CheckoutManager.generated.h"

class ACheckoutZone;

// GameState에서 라운드 변경될 때마다 PrepareNextCheckoutZoneState(int32 OpenCount) 호출해야 됨
// 이 함수는 계산대의 상태를 즉시 변경하지 않고, 10초 뒤에 변경하기 때문에
// 첫 호출을 20초에 시작하고, 30초 간격으로 반복 호출해야 됨


USTRUCT(BlueprintType)
struct FCheckoutRotationStep
{
    GENERATED_BODY()

public:
    // 라운드 시작 후 몇 초에 이 상태를 실제 적용할지
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CheckoutManager|Rotation", meta = (ClampMin = "0.0"))
    float ApplyTime = 30.0f;

    // 해당 시점에 열어둘 계산대 개수
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CheckoutManager|Rotation", meta = (ClampMin = "0"))
    int32 OpenCount = 2;
};

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

    // 계산대 존재하는지
    bool AreCheckoutZonesValid() const;

private:
    // Manager가 관리할 계산대 목록
    UPROPERTY(EditInstanceOnly, Category = " CheckoutManager|CheckoutZones")
    TArray<TObjectPtr<ACheckoutZone>> CheckoutZones;

// ------------------------------------------------------------
// 계산대 상태 변경
// ------------------------------------------------------------
public:
    UFUNCTION(BlueprintCallable, Category = "CheckoutManager|Rotation")
    void StartCheckoutRotation();

    UFUNCTION(BlueprintCallable, Category = "CheckoutManager|Rotation")
    void StopCheckoutRotation();

private:
    // ClosingSoon 상태 예약
    void ScheduleNextRotationStepWarning();

    // ClosingSoon 시작
    void HandleRotationStepWarning();

    // 모든 계산대 즉시 Open
    void OpenAllCheckoutZones();

    // 모든 계산대 즉시 Closed
    void CloseAllCheckoutZones();

private:
    // 라운드 시작 후 적용할 스케줄
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CheckoutManager|Rotation", meta = (AllowPrivateAccess = "true"))
    TArray<FCheckoutRotationStep> RotationSteps;

    // 현재 처리 중인 스케줄 인덱스
    int32 CurrentRotationStepIndex = INDEX_NONE;

    // 자동 회전 중인지
    bool bIsCheckoutRotationRunning = false;

    // 자동 회전 시작 월드 시간
    float CheckoutRotationStartWorldTime = 0.0f;

    // ClosingSoon 시작 타이머
    FTimerHandle RotationStepWarningTimerHandle;

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
    // 상태 변경 후 Open할 계산대 목록
    UPROPERTY(VisibleAnywhere, Category = "CheckoutManager|State")
    TArray<TObjectPtr<ACheckoutZone>> RandomOpenCheckoutZones;

    // Closing Soon 시간
    // 예: 10이면 ApplyTime 30초 Step은 20초에 ClosingSoon 시작
    UPROPERTY(EditAnywhere, Category = "CheckoutManager|State", meta = (ClampMin = "0.0"))
    float ClosingSoonDuration = 10.0f;

    // Closing Soon 이후 실제 상태 확정 타이머
    FTimerHandle ClosingSoonTimerHandle;
};
