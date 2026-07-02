#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Cart/Bumpable.h"
#include "CheckoutBarrier.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class BUMPERCART_API ACheckoutBarrier : public AActor, public IBumpable
{
	GENERATED_BODY()
	
public:	
	ACheckoutBarrier();

protected:
    virtual void BeginPlay() override;

// ------------------------------------------------------------
// 컴포넌트
// ------------------------------------------------------------

private:
    UPROPERTY(VisibleAnywhere, Category = "Checkout|Barrier")
    TObjectPtr<USceneComponent> SceneRoot;

    // 메시
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkout|Barrier", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> BarrierMesh;

// ------------------------------------------------------------
// 차단벽 활성 여부
// ------------------------------------------------------------

public:
    // 차단벽 활성화/비활성화
    UFUNCTION(BlueprintCallable, Category = "Checkout|Barrier")
    void SetBarrierEnabled(bool bIsEnabled);

    UFUNCTION(BlueprintPure, Category = "Checkout|Barrier")
    bool IsBarrierEnabled() const;

private:
    UPROPERTY(EditAnywhere, Category = "Checkout|Barrier")
    bool bIsBarrierEnabled = false;

// ------------------------------------------------------------
// 머티리얼
// ------------------------------------------------------------

private:
    // 동적 머티리얼 생성
    void InitializeBarrierMaterial();

    // Reveal 애니메이션 시작
    void StartRevealAnimation(float TargetReveal);

    // Reveal 값 갱신
    void UpdateRevealAnimation();

    // Reveal 애니메이션 종료 처리
    void FinishRevealAnimation();

    // 머티리얼 Reveal 값 적용
    void SetRevealValue(float RevealValue);

private:
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> BarrierMID;

    // 생성 및 제거 연출 시간
    UPROPERTY(EditAnywhere, Category = "Checkout|Barrier|Visual", meta = (ClampMin = "0.0"))
    float RevealDuration = 0.2f;

    // 머티리얼 갱신 간격
    UPROPERTY(EditAnywhere, Category = "Checkout|Barrier|Visual", meta = (ClampMin = "0.001"))
    float RevealUpdateInterval = 0.015f;

    float CurrentRevealValue = 0.0f;
    float RevealStartValue = 0.0f;
    float RevealTargetValue = 0.0f;
    float RevealAnimationStartTime = 0.0f;

    FTimerHandle RevealTimerHandle;

    // ------------------------------------------------------------
    // Collision
    // ------------------------------------------------------------

private:
    // 시각 연출과 별도로 충돌만 설정
    void SetBarrierCollisionEnabled(bool bIsEnabled);


};
