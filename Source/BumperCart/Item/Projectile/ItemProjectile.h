// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemProjectile.generated.h"

class ACartPawn;
class UCartScreenFXComponent;
class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class USoundAttenuation;

UCLASS(Abstract)
class BUMPERCART_API AItemProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AItemProjectile();

protected:
	virtual void BeginPlay() override;

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
protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Component")
    TObjectPtr<USphereComponent> CollisionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Component")
    TObjectPtr<UStaticMeshComponent> ProjectileMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Component")
    TObjectPtr<UNiagaraComponent> TrailNiagaraComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Movement")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;


// ------------------------------------------------------------
// 투사체 발사
// ------------------------------------------------------------
public:
    // 투사체 생성 직후 발사 방향 설정
    void FireInDirection(const FVector& Direction);

protected:
    // 플레이어가 맞았을 때 투사체별 처리하는 함수
    virtual void OnHitCart(ACartPawn* HitPlayer);

private:
    // 플레이어가 맞았을 때
    void HandleHitCart(ACartPawn* HitPlayer);

    // 다른 액터가 맞았을 때
    void HandleHitOtherActor(AActor* HitActor);

protected:
    // 투사체 속도
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Movement", meta = (ClampMin = "0.0"))
    float ProjectileSpeed = 1500.0f;

    // 중력
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Movement", meta = (ClampMin = "0.0"))
    float ProjectileGravityScale = 0.1f;

    // Destroy 시간
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Movement", meta = (ClampMin = "0.0"))
    float MaxLifeTime = 3.0f;

    // 한 번 이상 충돌 했는지
    bool bHasHit = false;


// ------------------------------------------------------------
// 이펙트
// ------------------------------------------------------------
private:
    // 모든 클라이언트에서 충돌 이펙트 재생
    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayHitProjectileEffect(const FVector& EffectLocation);

protected:
    // 투사체 궤적 Niagara
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Effect")
    TObjectPtr<UNiagaraSystem> TrailNiagaraSystem;

    // 투사체 충돌 Niagara
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Effect")
    TObjectPtr<UNiagaraSystem> HitNiagaraSystem;

// ------------------------------------------------------------
// 사운드
// ------------------------------------------------------------
private:
    // 투사체 생성 시 로컬에서 던지는 사운드 재생
    void PlayThrowProjectileSound() const;

    // 투사체가 오브젝트와 충돌 시 주변 플레이어에게 사운드 재생
    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayHitProjectileSound(const FVector& SoundLocation);

protected:
    // 사운드 거리
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Sound")
    TObjectPtr<USoundAttenuation> HitSoundAttenuation;

    // 투사체 던질 때 사운드
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Sound")
    TObjectPtr<USoundBase> ThrowProjectileSound;

    // 튜사체 맞았을 때 사운드
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Sound")
    TObjectPtr<USoundBase> HitProjectileSound;

};
