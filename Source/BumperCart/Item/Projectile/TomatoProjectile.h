#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TomatoProjectile.generated.h"

class ACartPawn;
class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;


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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tomato|Movement", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
    
// ------------------------------------------------------------
// 투사체 발사
// ------------------------------------------------------------
public:
    // 투사체 생성 직후 발사 방향 설정
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

    // Destroy 시간
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tomato|Movement", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
    float MaxLifeTime = 3.0f;

};
