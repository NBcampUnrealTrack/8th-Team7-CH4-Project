#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPCRushGimmick.generated.h"

class UBoxComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;

UCLASS()
class BUMPERCART_API ANPCRushGimmick : public AActor
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

    UPROPERTY(EditAnywhere, Category = "Gimmick | NPC Rush")
    TObjectPtr<UNiagaraComponent> NPCRushFX;

#pragma endregion

protected:
    UFUNCTION()
    void OnCartOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

};
