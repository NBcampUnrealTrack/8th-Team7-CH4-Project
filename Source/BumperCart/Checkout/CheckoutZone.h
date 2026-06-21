#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CheckoutTypes.h"
#include "CheckoutZone.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UBoxComponent;

struct FLoadedProductInfo;

UCLASS(Blueprintable)
class BUMPERCART_API ACheckoutZone : public AActor
{
	GENERATED_BODY()
	
public:	
	ACheckoutZone();

protected:
	virtual void BeginPlay() override;

public:
    void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

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
    UPROPERTY(VisibleAnywhere, Category = "_Checkout|Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "_Checkout|Components")
    TObjectPtr<UStaticMeshComponent> CounterMesh;

    UPROPERTY(VisibleAnywhere, Category = "_Checkout|Components")
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
    UPROPERTY(VisibleAnywhere, Category = "_Checkout|Player")
    TArray<TObjectPtr<ACharacter>> PlayersInZone;

    // 계산 중인 플레이어
    // 복제 데이터
    UPROPERTY(VisibleAnywhere, Replicated, Category = "_Checkout|Player")
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

    // 클라이언트에서 값을 복제 받을 때 호출되는 콜백 함수
    // 색상, UI 표시용
    UFUNCTION()
    void OnRep_CurrentCounterState();

public:
    // 현재 계산대 오픈 상태
    // 복제 데이터
    UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_CurrentCounterState, Category = "_Checkout|Condition")
    ECounterState CurrentCounterState = ECounterState::Open;

    // 현재 계산 진행 중인지
    // 복제 데이터
    UPROPERTY(VisibleAnywhere, Replicated, Category = "_Checkout|Condition")
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
    UPROPERTY(VisibleInstanceOnly, Category = "_Checkout|Progress")
    float CheckoutProgress = 0.0f;

    // 계산 시간 타이머
    FTimerHandle CheckoutTimerHandle;

    // 현재 월드 시간 기준 계산 시작 시간
    // 복제 데이터
    UPROPERTY(VisibleInstanceOnly, Replicated, Category = "_Checkout|Progress")
    float CheckoutStartTime = 0.0f;

    // 계산 경과 시간
    float ElapsedCheckoutTime = 0.0f;

    // 목표 계산 시간
    // 복제 데이터
    UPROPERTY(VisibleInstanceOnly, Replicated, Category = "_Checkout|Progress")
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
    UPROPERTY(EditAnywhere, Category = "_Checkout|Time")
    float BaseCheckoutTime = 2.0f;

    // 추가 정산 시간
    UPROPERTY(EditAnywhere, Category = "_Checkout|Time")
    float AdditionalCheckoutTime = 1.0f;

// ------------------------------------------------------------
// 최종 점수 계산
// ------------------------------------------------------------
private:
    // 최종 점수 계산
    int32 CalculateCheckoutScore(const TArray<FLoadedProductInfo>& Products) const;

public:
    int32 CheckoutScore = 0;

// ------------------------------------------------------------
// Getter
// ------------------------------------------------------------
public:
    // 서버 시간 기준 정산 진행도 리턴
    UFUNCTION(BlueprintPure, Category = "_Checkout|Progress")
    float GetCheckoutProgress() const;

    // 정산 중인지 리턴
    UFUNCTION(BlueprintPure, Category = "_Checkout|Progress")
    bool IsCheckoutInProgress() const;

    // 현재 정산중인 플레이어 리턴
    UFUNCTION(BlueprintPure, Category = "_Checkout|Progress")
    ACharacter* GetCurrentCheckoutPlayer() const;

};
