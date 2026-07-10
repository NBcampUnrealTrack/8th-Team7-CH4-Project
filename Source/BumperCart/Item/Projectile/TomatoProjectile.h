#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatoProjectile.generated.h"

class ACartPawn;
class UCartScreenFXComponent;
class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class USoundAttenuation;

UCLASS()
class BUMPERCART_API ATomatoProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	ATomatoProjectile();

protected:
	virtual void BeginPlay() override;

// ------------------------------------------------------------
// 충돌 판정
// ------------------------------------------------------------
private:
    UFUNCTION()
    void OnProjectileBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

// ------------------------------------------------------------
// 컴포넌트
// ------------------------------------------------------------
private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tomato|Component", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USphereComponent> CollisionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tomato|Component", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> TomatoMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tomato|Component", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UNiagaraComponent> TrailNiagaraComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tomato|Movement", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
    
// ------------------------------------------------------------
// 투사체 발사
// ------------------------------------------------------------
public:
    // 토마토 생성 직후 발사 방향 설정
    void FireInDirection(const FVector& Direction);

private:
    // 플레이어가 맞았을 때
    void HandleHitCart(ACartPawn* HitPlayer);

    // 다른 액터가 맞았을 때
    void HandleHitOtherActor(AActor* HitActor);

private:
    // 투사체 속도
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tomato|Movement", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
    float ProjectileSpeed = 1500.0f;

    // 중력
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tomato|Movement", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
    float ProjectileGravityScale = 0.1f;

    // Destroy 시간
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tomato|Movement", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
    float MaxLifeTime = 3.0f;

// ------------------------------------------------------------
// 화면 가림 UI 적용
// ------------------------------------------------------------
private:
    // 가려질 시간
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tomato|Effect", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
    float ScreenBlockDuration = 3.0f;

// ------------------------------------------------------------
// 궤적 이펙트
// ------------------------------------------------------------
private:
    // 투사체 궤적 Niagara
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tomato|Effect", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UNiagaraSystem> TrailNiagaraSystem;
// ------------------------------------------------------------
// 사운드
// ------------------------------------------------------------
private:
    // 토마토 생성 시 로컬에서 던지는 사운드 재생
    void PlayThrowTomatoSound() const;

    // 토마토가 오브젝트와 충돌 시 주변 플레이어에게 사운드 재생
    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayHitTomatoSound(const FVector& SoundLocation);

    // 사운드 거리
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tomato|Sound", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USoundAttenuation> HitSoundAttenuation;

private:
    // 토마토 던질 때 사운드
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tomato|Sound", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USoundBase> ThrowTomatoSound;

    // 토마토 맞았을 때 사운드
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tomato|Sound", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USoundBase> HitTomatoSound;

};
