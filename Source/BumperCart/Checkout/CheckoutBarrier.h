#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Cart/Bumpable.h"
#include "CheckoutBarrier.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UBoxComponent;

UCLASS(Blueprintable)
class BUMPERCART_API ACheckoutBarrier : public AActor, public IBumpable
{
	GENERATED_BODY()
	
public:	
	ACheckoutBarrier();

public:
    // 차단벽 활성화/비활성화
    UFUNCTION(BlueprintCallable, Category = "Checkout|Barrier")
    void SetBarrierEnabled(bool bIsEnabled);

    UFUNCTION(BlueprintPure, Category = "Checkout|Barrier")
    bool IsBarrierEnabled() const;

private:
    UPROPERTY(VisibleAnywhere, Category = "Checkout|Barrier")
    TObjectPtr<USceneComponent> SceneRoot;

    // 메시
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkout|Barrier", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> BarrierMesh;

    // 실제 충돌 판정
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkout|Barrier", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UBoxComponent> BarrierCollision;

    UPROPERTY(VisibleAnywhere, Category = "Checkout|Barrier")
    bool bIsBarrierEnabled = true;
};
