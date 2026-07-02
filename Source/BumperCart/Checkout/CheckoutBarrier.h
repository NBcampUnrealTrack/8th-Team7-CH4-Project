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

private:
    UPROPERTY(VisibleAnywhere, Category = "Checkout|Barrier")
    TObjectPtr<USceneComponent> SceneRoot;

    // 메시
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkout|Barrier", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> BarrierMesh;

public:
    // 차단벽 활성화/비활성화
    UFUNCTION(BlueprintCallable, Category = "Checkout|Barrier")
    void SetBarrierEnabled(bool bIsEnabled);

    UFUNCTION(BlueprintPure, Category = "Checkout|Barrier")
    bool IsBarrierEnabled() const;

private:
    UPROPERTY(EditAnywhere, Category = "Checkout|Barrier")
    bool bIsBarrierEnabled = false;
};
