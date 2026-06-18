#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CheckoutTypes.h"
#include "CheckoutZone.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UBoxComponent;

UCLASS(Blueprintable)
class BUMPERCART_API ACheckoutZone : public AActor
{
	GENERATED_BODY()
	
public:	
	ACheckoutZone();

protected:
	virtual void BeginPlay() override;

// ------------------------------------------------------------
// Overlap 이벤트
// ------------------------------------------------------------
private:
    UFUNCTION()
    void OnCheckoutZoneBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

    UFUNCTION()
    void OnCheckoutZoneEndOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        int32 OtherBodyIndex
    );

// ------------------------------------------------------------
// 컴포넌트
// ------------------------------------------------------------
private:
    UPROPERTY(VisibleAnywhere, Category = "Checkout|Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "Checkout|Components")
    TObjectPtr<UStaticMeshComponent> CounterMesh;

    UPROPERTY(VisibleAnywhere, Category = "Checkout|Components")
    TObjectPtr<UBoxComponent> CheckoutTrigger;

// ------------------------------------------------------------
// 구역 내 플레이어
// ------------------------------------------------------------
private:
    // 플레이어가 범위 안에 들어오면 배열에 추가
    void AddPlayerInZone(ACharacter* PlayerCharacter);

    // 플레이어가 범위 밖으로 나가면 배열에서 삭제
    void RemovePlayerFromZone(ACharacter* PlayerCharacter);

private:
    // 범위 내 플레이어 배열
    UPROPERTY(VisibleAnywhere, Category = "Checkout|Player")
    TArray<TObjectPtr<ACharacter>> PlayersInZone;

    // 계산 중인 플레이어
    UPROPERTY(VisibleAnywhere, Category = "Checkout|Player")
    TObjectPtr<ACharacter> CurrentCheckoutPlayer;

// ------------------------------------------------------------
// 계산 조건
// ------------------------------------------------------------
private:
    // 계산 가능 여부 확인
    bool CanStartCheckout(ACharacter* PlayerCharacter) const;

    // 계산 시도
    void TryStartCheckout();

    // 다음 계산 대상 찾기
    ACharacter* FindNextCheckoutPlayer();

private:
    // 현재 계산대 오픈 상태
    UPROPERTY(EditAnywhere, Category = "Checkout|Condition")
    ECounterState CurrentCounterState = ECounterState::OPEN;

    // 현재 계산 진행 중인지
    bool bIsCheckoutInProgress = false;

// ------------------------------------------------------------
// 계산 진행
// ------------------------------------------------------------
private:
    // 계산 시작
    void StartCheckout(ACharacter* PlayerCharacter);

    // 계산 진행도 업데이트
    void UpdateCheckoutProgress();

private:
    // 계산 진행도
    UPROPERTY(VisibleInstanceOnly, Category = "Checkout|Checkout")
    float CheckoutProgress = 0.0f;

    // 계산 시간 타이머
    FTimerHandle CheckoutTimerHandle;

    // 현재 월드 시간 기준 계산 시작 시간
    float CheckoutStartTime = 0.0f;
    // 계산 경과 시간
    float ElapsedCheckoutTime = 0.0f;
    // 목표 계산 시간
    float RequiredCheckoutTime = 0.0f;

// ------------------------------------------------------------
// 계산 종료
// ------------------------------------------------------------
private:
    // 계산 성공
    void CompleteCheckout();

    // 계산 취소
    void CancelCheckout();

    // 계산 초기화
    void ResetCheckout();

// ------------------------------------------------------------
// 정산 시간 계산
// ------------------------------------------------------------
private:
    // 전체 정산 시간 계산
    float CalculateCheckoutDuration(int32 ProductCount) const;

private:
    // 기본 정산 시간
    UPROPERTY(EditAnywhere, Category = "Checkout|Time")
    float BaseCheckoutTime = 2.0f;

    // 추가 정산 시간
    UPROPERTY(EditAnywhere, Category = "Checkout|Time")
    float AdditionalCheckoutTime = 1.0f;

// ------------------------------------------------------------
// 최종 점수 계산
// ------------------------------------------------------------
private:
    // 최종 점수 계산
    int32 CalculateCheckoutScore() const;

public:
    int32 CheckoutScore = 0;
};
