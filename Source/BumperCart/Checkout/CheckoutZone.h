#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CheckoutTypes.h"
#include "CheckoutZone.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UBoxComponent;
class UMaterialInterface;

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
    UPROPERTY(VisibleAnywhere, Category = " Checkout|Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = " Checkout|Components")
    TObjectPtr<UStaticMeshComponent> CheckoutZoneMesh;

    UPROPERTY(VisibleAnywhere, Category = " Checkout|Components")
    TObjectPtr<UBoxComponent> CheckoutTrigger;

private:
    UPROPERTY(EditAnywhere, Category = " Checkout|Material")
    TObjectPtr<UMaterialInterface> OpenMaterial;

    UPROPERTY(EditAnywhere, Category = " Checkout|Material")
    TObjectPtr<UMaterialInterface> ClosingSoonMaterial;

    UPROPERTY(EditAnywhere, Category = " Checkout|Material")
    TObjectPtr<UMaterialInterface> ClosedMaterial;

// ------------------------------------------------------------
// 계산대 정보
// ------------------------------------------------------------

private:
    // 계산대 ID
    UPROPERTY(EditInstanceOnly, Category = " Checkout|Identity")
    int32 CheckoutZoneID = INDEX_NONE;

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
    UPROPERTY(VisibleAnywhere, Category = " Checkout|Player")
    TArray<TObjectPtr<ACharacter>> PlayersInZone;

    // 계산 중인 플레이어
    // 복제 데이터
    UPROPERTY(VisibleAnywhere, Replicated, Category = " Checkout|Player")
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
    void OnRep_CurrentCheckoutZoneState();

private:
    // 현재 계산대 오픈 상태
    // 복제 데이터
    UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_CurrentCheckoutZoneState, Category = " Checkout|Condition")
    ECheckoutZoneState CurrentCheckoutZoneState = ECheckoutZoneState::Open;

    // 현재 계산 진행 중인지
    // 복제 데이터
    UPROPERTY(VisibleAnywhere, Replicated, Category = " Checkout|Condition")
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
    UPROPERTY(VisibleInstanceOnly, Category = " Checkout|Progress")
    float CheckoutProgress = 0.0f;

    // 계산 시간 타이머
    FTimerHandle CheckoutTimerHandle;

    // 현재 월드 시간 기준 계산 시작 시간
    // 복제 데이터
    UPROPERTY(VisibleInstanceOnly, Replicated, Category = " Checkout|Progress")
    float CheckoutStartTime = 0.0f;

    // 계산 경과 시간
    float ElapsedCheckoutTime = 0.0f;

    // 목표 계산 시간
    // 복제 데이터
    UPROPERTY(VisibleInstanceOnly, Replicated, Category = " Checkout|Progress")
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
    UPROPERTY(EditAnywhere, Category = " Checkout|Time")
    float BaseCheckoutTime = 2.0f;

    // 추가 정산 시간
    UPROPERTY(EditAnywhere, Category = " Checkout|Time")
    float AdditionalCheckoutTime = 1.0f;

// ------------------------------------------------------------
// 최종 점수 계산
// ------------------------------------------------------------

public:
    // 세일 보너스
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkout|Score")
    float SaleBonusMultiplier = 1.5f;

    // 마지막 계산 보너스
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkout|Score")
    float LastCheckoutBonusMultiplier = 1.5f;

    // 마지막 보너스 점수 계산 테스트용
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkout|Score")
    bool bIsApplyLastCheckoutBonusForTest = false;

    // 정산 점수
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkout|Score")
    int32 CheckoutScore = 0;

// ------------------------------------------------------------
// Getter
// ------------------------------------------------------------
public:
    // 서버 시간 기준 정산 진행도 리턴
    UFUNCTION(BlueprintPure, Category = " Checkout|Progress")
    float GetCheckoutProgress() const;

    // 정산 중인지 리턴
    UFUNCTION(BlueprintPure, Category = " Checkout|Progress")
    bool IsCheckoutInProgress() const;

    // 현재 정산중인 플레이어 리턴
    UFUNCTION(BlueprintPure, Category = " Checkout|Progress")
    ACharacter* GetCurrentCheckoutPlayer() const;

    // 현재 계산대 상태 리턴
    UFUNCTION(BlueprintPure, Category = " Checkout|Condition")
    ECheckoutZoneState GetCheckoutZoneState() const;

// ------------------------------------------------------------
// Setter
// ------------------------------------------------------------
public:
    UFUNCTION(BlueprintPure, Category = " Checkout|Identity")
    int32 GetCheckoutZoneID() const;

    // 계산대 상태 변경
    void SetCheckoutZoneState(ECheckoutZoneState NewState);

// ------------------------------------------------------------
// 델리게이트
// ------------------------------------------------------------

public:
    // 정산 완료 사실을 Manager에게 전달하기 위한 Delegate
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnCheckoutCompleted, ACheckoutZone*);
    FOnCheckoutCompleted OnCheckoutCompleted;
};
