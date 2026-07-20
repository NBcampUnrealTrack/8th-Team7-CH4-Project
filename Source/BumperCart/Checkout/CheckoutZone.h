#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CheckoutTypes.h"
#include "CheckoutZone.generated.h"

class ACheckoutBarrier;
class ACartPawn;
class UChildActorComponent;
class USceneComponent;
class UStaticMeshComponent;
class USphereComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USoundBase;
class USoundAttenuation;
class UAudioComponent;
class UNiagaraComponent;
class UNiagaraSystem;

struct FLoadedProductInfo;
struct FLoadInfo;

DECLARE_MULTICAST_DELEGATE_OneParam(
    FOnCheckoutCompleted,
    ACheckoutZone*
);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnCheckoutZoneStateChanged,
    int32,
    CheckoutZoneID,
    ECheckoutZoneState,
    NewState
);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnCheckoutSessionChanged,
    int32,
    CheckoutZoneID,
    ACartPawn*,
    CheckoutPlayer,
    bool,
    bInProgress
);

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
// Getter
// ------------------------------------------------------------
public:
    // 서버 시간 기준 정산 진행도 리턴
    UFUNCTION(BlueprintPure, Category = " Checkout|Progress")
    float GetCheckoutProgress() const;

    // 필요 정산 시간
    UFUNCTION(BlueprintPure, Category = "Checkout|Progress")
    float GetRequiredCheckoutTime() const;

    // 남은 정산 시간
    UFUNCTION(BlueprintPure, Category = "Checkout|Progress")
    float GetRemainingCheckoutTime() const;

    // 정산 중인지 리턴
    UFUNCTION(BlueprintPure, Category = " Checkout|Progress")
    bool IsCheckoutInProgress() const;

    // 현재 정산중인 플레이어 리턴
    UFUNCTION(BlueprintPure, Category = " Checkout|Progress")
    ACartPawn* GetCurrentCheckoutPlayer() const;

    // 현재 계산대 상태 리턴
    UFUNCTION(BlueprintPure, Category = " Checkout|Condition")
    ECheckoutZoneState GetCheckoutZoneState() const;

    // 계산대 ID 리턴
    UFUNCTION(BlueprintPure, Category = " Checkout|Identity")
    int32 GetCheckoutZoneID() const;

    // 계산대 내부에 있는지
    UFUNCTION(BlueprintPure, Category = "Checkout|Player")
    bool IsPlayerInsideCheckoutZone(const ACartPawn* PlayerCharacter) const;

// ------------------------------------------------------------
// Setter
// ------------------------------------------------------------
public:
    // 계산대 상태 변경
    void SetCheckoutZoneState(ECheckoutZoneState NewState);

// ------------------------------------------------------------
// 이벤트
// ------------------------------------------------------------

public:
    // 정산 완료 사실을 Manager에게 전달하기 위한 이벤트
    FOnCheckoutCompleted OnCheckoutCompleted;

    // 계산대 상태 변경을 조회하기 위한 이벤트
    UPROPERTY(BlueprintAssignable, Category = "Checkout|Event")
    FOnCheckoutZoneStateChanged OnCheckoutZoneStateChanged;

    // 계산 시작, 취소, 완료 이벤트
    UPROPERTY(BlueprintAssignable, Category = "Checkout|Event")
    FOnCheckoutSessionChanged OnCheckoutSessionChanged;

// ------------------------------------------------------------
// 기본 컴포넌트
// ------------------------------------------------------------
private:
    UPROPERTY(VisibleAnywhere, Category = " Checkout|Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = " Checkout|Components")
    TObjectPtr<UStaticMeshComponent> CheckoutZoneMesh;

    UPROPERTY(VisibleAnywhere, Category = " Checkout|Components")
    TObjectPtr<USphereComponent> CheckoutTrigger;

private:
    // 원형 차단벽
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkout|Barrier", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UChildActorComponent> CheckoutBarrierComponent;

    // 배출점
    // BeginPlay에서 CreateEjectPointsFromComponents를 통해 생성
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkout|Barrier", meta = (AllowPrivateAccess = "true"))
    TArray<TObjectPtr<USceneComponent>> EjectPoints;

// ------------------------------------------------------------
// 머티리얼
// ------------------------------------------------------------
private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkout|Components",meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> CheckoutZoneVisual;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> CheckoutZoneVisualMID;

private:
    UPROPERTY(EditDefaultsOnly, Category = "Checkout|Components")
    FCheckoutZoneVisualStyle OpenCheckoutZoneStyle;

    UPROPERTY(EditDefaultsOnly, Category = "Checkout|Components")
    FCheckoutZoneVisualStyle ClosingSoonCheckoutZoneStyle;

    UPROPERTY(EditDefaultsOnly, Category = "Checkout|Components")
    FCheckoutZoneVisualStyle ClosedCheckoutZoneStyle;

private:
    // 링과 내부 채움의 동적 머티리얼 생성
    void InitializeCheckoutZoneMaterials();

    // 현재 계산대 상태에 맞춰 범위 연출 갱신
    void UpdateCheckoutZoneVisual();

    // 링과 내부 채움에 색상 및 밝기 적용
    void ApplyCheckoutZoneVisual(const FCheckoutZoneVisualStyle& Style);

// ------------------------------------------------------------
// 사운드
// ------------------------------------------------------------
private:
    // 서버와 모든 클라이언트에서 정산 완료음 재생
    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayCheckoutCompleteSound();

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastPlayCheckoutStateChangeSound();

private:
    // 정산 중 반복할 비프음
    UPROPERTY(EditDefaultsOnly, Category = "Checkout|Sound")
    TObjectPtr<UAudioComponent> CheckoutProcessingAudio;

    // 정산 완료 및 점수 획득 
    UPROPERTY(EditDefaultsOnly, Category = "Checkout|Sound")
    TObjectPtr<USoundBase> CheckoutCompleteSound;

    // 정산 완료음 거리 감쇠
    UPROPERTY(EditDefaultsOnly, Category = "Checkout|Sound")
    TObjectPtr<USoundAttenuation> CheckoutCompleteAttenuation;

    // 계산대 열림/폐쇄 사운드
    UPROPERTY(EditDefaultsOnly, Category = "Checkout|Sound")
    TObjectPtr<USoundBase> CheckoutStateChangeSound;

// ------------------------------------------------------------
// 나이아가라
// ------------------------------------------------------------
private:
    // 계산대 상태에 따라 이펙트 재생
    void UpdateCheckoutEffect();

    // 서버와 모든 클라이언트에서 정산 완료 이펙트 재생
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastPlayCheckoutCompleteEffect(ACartPawn* CompletedPlayer);

private:
    // Open 상태에서만 재생할 계산대 상승 링 이펙트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkout|Effect", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UNiagaraComponent> OpenStateEffect;

    // 정산 완료 이펙트
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Checkout|Effect", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UNiagaraSystem> CheckoutCompleteEffect;

    // 정산 중 이펙트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkout|Effect", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UNiagaraComponent> CheckoutProcessingEffect;

    UPROPERTY(EditDefaultsOnly, Category = "Checkout|Effect|Processing")
    FLinearColor OpenWaveColor = FLinearColor(0.15f, 1.0f, 0.3f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "Checkout|Effect|Processing")
    FLinearColor ClosingSoonWaveColor = FLinearColor(0.15f, 1.0f, 0.3f, 1.0f);

// ------------------------------------------------------------
// 차단벽
// ------------------------------------------------------------
public:
    // 게임 시작 전 차단 방식 사용 여부 설정
    UFUNCTION(BlueprintCallable, Category = "Checkout|Barrier")
    void SetUseCheckoutBarrier(bool bUseBarrier);

    // 현재 차단 방식을 사용하는지
    UFUNCTION(BlueprintPure, Category = "Checkout|Barrier")
    bool IsUsingCheckoutBarrier() const;

    // 실제 입구 차단벽 활성화
    UFUNCTION(BlueprintCallable, Category = "Checkout|Barrier")
    void SetCheckoutBarrierEnabled(bool bIsEnabled);

private:
    // 차단 방식 설정이 클라이언에 복제될 때
    UFUNCTION()
    void OnRep_UseCheckoutBarrier();

    // 실제 입구 차단벽 상태가 클라이언트에 복제될 때
    UFUNCTION()
    void OnRep_CheckoutBarrierEnabled();

    // 메시와 Collision 적용
    void ApplyBarrierState();

    ACheckoutBarrier* GetCheckoutBarrier() const;

private:
    // false: 기존 계산대 방식
    // true: 정산 중 원형 차단벽 사용
    // 복제 데이터
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_UseCheckoutBarrier, Category = "Checkout|Barrier", meta = (AllowPrivateAccess = "true"))
    bool bUseCheckoutBarrier = false;

    // 입구 차단벽의 활성 상태
    // 복제 데이터
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_CheckoutBarrierEnabled, Category = "Checkout|Barrier", meta = (AllowPrivateAccess = "true"))
    bool bIsCheckoutBarrierEnabled = false;

// ------------------------------------------------------------
// 플레이어 동시 진입
// ------------------------------------------------------------
private:
    // 블루프린트에 배치된 EjectPoint 컴포넌트들 수집
    void CreateEjectPointsFromComponents();

    // 정산가 아닌 플레이어를 입구 바깥으로 밀어냄
    void EjectPlayer(ACartPawn* PlayerCharacter);

    // 계산대 내부에 있던 플레이어가 정산자가 아닌 경우 바깥으로 밀어냄
    void EjectNonCheckoutPlayers();

    // 배출 위치 찾기
    USceneComponent* FindBestEjectPoint(const ACartPawn* PlayerCharacter) const;

    // sweep 검사하여 다른 카트 or 계산대와 충돌할 지 감지
    bool IsEjectPathClear(const ACartPawn* PlayerCharacter, const FVector& TargetLocation) const;

private:
    // 현재 계산대 밖으로 배출될 플레이어
    UPROPERTY(VisibleAnywhere, Category = "Checkout|Player")
    TArray<TObjectPtr<ACartPawn>> EjectingPlayers;

    // 비점유 플레이어 배출 세기
    UPROPERTY(EditAnywhere, Category = "Checkout|Barrier", meta = (ClampMin = "0.0"))
    float EjectStrength = 300.0f;

// ------------------------------------------------------------
// 충돌 무시
// ------------------------------------------------------------
private:
    // 플레이어 계산대 차단 무시
    void SetPlayerBarrierIgnore(ACartPawn* PlayerCharacter,  bool bShouldIgnore);

    // 서버/클라이언트 모두 차단벽 무시
    UFUNCTION(NetMulticast, Reliable)
    void MulticastSetPlayerBarrierIgnore(ACartPawn* PlayerCharacter, bool bShouldIgnore);


    // 정산 시간 동안 차단벽 무시
    void UpdateCheckoutPlayerBarrierIgnore();

private:
    // 이전 플레이어의 Ignore를 정상 복구하기 위한 참조
    TWeakObjectPtr<ACartPawn> BarrierIgnoredPlayer;

// ------------------------------------------------------------
// 계산대 정보
// ------------------------------------------------------------

private:
    // 계산대 ID
    UPROPERTY(EditInstanceOnly, Category = " Checkout|Identity")
    int32 CheckoutZoneID = INDEX_NONE;

    // 현재 계산대 오픈 상태
    // 복제 데이터
    UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_CurrentCheckoutZoneState, Category = " Checkout|Condition")
    ECheckoutZoneState CurrentCheckoutZoneState = ECheckoutZoneState::Open;

// ------------------------------------------------------------
// 구역 내 플레이어
// ------------------------------------------------------------
private:
    // 플레이어가 범위 안에 들어오면 배열에 추가
    void AddPlayerInZone(ACartPawn* PlayerCharacter);

    // 플레이어가 범위 밖으로 나가면 배열에서 삭제
    void RemovePlayerFromZone(ACartPawn* PlayerCharacter);

    // 계산대 범위 내에서 상품을 획득할 경우
    UFUNCTION()
    void HandleLoadInfoChanged(AActor* OwnerActor, const FLoadInfo& LoadInfo);

private:
    // 범위 내 플레이어 배열
    UPROPERTY(VisibleAnywhere, Category = " Checkout|Player")
    TArray<TObjectPtr<ACartPawn>> PlayersInZone;

    // 계산 중인 플레이어
    // 복제 데이터
    UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_CheckoutSession, Category = " Checkout|Player")
    TObjectPtr<ACartPawn> CurrentCheckoutPlayer;

    // 정산 중 마지막으로 확인한 적재 상품 수
    int32 LastLoadedProductCount = 0;

    UPROPERTY(VisibleAnywhere, Category = "Checkout|Player")
    TArray<TObjectPtr<ACartPawn>> CancelCheckoutPlayers;

// ------------------------------------------------------------
// 계산 조건
// ------------------------------------------------------------
private:
    // 계산 가능 여부 확인
    bool CanStartCheckout(ACartPawn* PlayerCharacter) const;

    // 계산 시도
    void TryStartCheckout();

    // 다음 계산 대상 찾기
    ACartPawn* FindNextCheckoutPlayer();

private:
    // 현재 계산 진행 중인지
    // 복제 데이터
    UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_CheckoutSession, Category = " Checkout|Condition")
    bool bIsCheckoutInProgress = false;

    // 최소 정산 가능한 상품 개수
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkout|Condition", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
    int32 MinimumCheckoutProductCount = 1;

    // 단일 상품 정산 여부
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkout|Condition", meta = (AllowPrivateAccess = "true"))
    bool bIsUseSingleCheckout = false;

// ------------------------------------------------------------
// 계산 진행
// ------------------------------------------------------------
private:
    // 계산 시작
    void StartCheckout(ACartPawn* PlayerCharacter);

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
    float CalculateCheckoutDuration(int32 ProductCount, bool bIsFinalPhase) const;

private:
    // 기본 정산 시간
    UPROPERTY(EditAnywhere, Category = " Checkout|Time")
    float BaseCheckoutTime = 2.0f;

    // 추가 정산 시간
    UPROPERTY(EditAnywhere, Category = " Checkout|Time")
    float AdditionalCheckoutTime = 1.0f;

    // 최대 정산 시간
    UPROPERTY(EditAnywhere, Category = "Checkout|Time", meta = (ClampMin = "0.1"))
    float MaxCheckoutTime = 5.0f;

    // 마지막 라운드 정산 시간
    UPROPERTY(EditAnywhere, Category = " Checkout|Time")
    float FinalPhaseCheckoutTime = 0.5f;

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
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkout|Score")
    int32 LastCheckoutScore = 0;

    // 현재 연속 정산 횟수
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Checkout|Score")
    int32 CurrentCheckoutCount = 0;

    // 두 번째 정산부터 지급하는 고정 콤보 점수
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkout|Score")
    int32 ComboBonusPerCheckout = 10;

// ------------------------------------------------------------
// RepNotify
// ------------------------------------------------------------
private:
    // 클라이언트에서 값을 복제 받을 때 호출되는 콜백 함수
    // 색상, UI 표시용
    UFUNCTION()
    void OnRep_CurrentCheckoutZoneState();

    // 정산 중인지 확인
    UFUNCTION()
    void OnRep_CheckoutSession();
};
