#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Cart/Bumpable.h"
#include "NPCRushGimmick.generated.h"

class UBoxComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;
class ACartPawn;
class UNiagaraSystem;

UCLASS()
class BUMPERCART_API ANPCRushGimmick : public AActor, public IBumpable
{
	GENERATED_BODY()
	
public:	
	ANPCRushGimmick();

#pragma region Override
protected:
    virtual void BeginPlay() override;

#pragma endregion

#pragma region Component
protected:
    // 매쉬
    UPROPERTY(EditAnywhere, Category = "Component")
    TObjectPtr<UStaticMeshComponent> CartMesh;

    // 콜리전
    UPROPERTY(EditAnywhere, Category = "Component")
    TObjectPtr<UBoxComponent> BoxCollision;

    // 발사체 무브먼트 컴포넌트
    UPROPERTY(VisibleAnywhere, Category = "Component")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

    UPROPERTY(EditAnywhere, Category = "Compoonent | Naiagara")
    TObjectPtr<UNiagaraComponent> NPCRushFX;

    UPROPERTY(VisibleAnywhere, Category = "Component | Sound")
    TObjectPtr<UAudioComponent> NPCRushAudioComponent;

#pragma endregion

private:
    UPROPERTY(EditAnywhere, Category = "Gimmick | Config")
    float Strength = 500.0f;

    float LastHitTime = 0.0f;

protected:
    UPROPERTY(EditAnywhere, Category = "Sound | Spawn")
    USoundBase* SpawnSound;

    UPROPERTY(EditAnywhere, Category = "Sound | Knokback")
    USoundBase* KnockbackSound;

    // 충돌시 사용할 이펙트
    UPROPERTY(EditAnywhere, Category = "FX")
    TObjectPtr<UNiagaraSystem> KnockbackFX;

    UFUNCTION()
    void OnCartOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    void Knockback(ACartPawn* PlayerCart, const FHitResult& SweepResult);

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayerHitEffect(FVector SpawnLocation, FRotator SpawnRotation);
};
