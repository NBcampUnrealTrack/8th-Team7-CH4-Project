#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Cart/Bumpable.h"
#include "CustomerAI.generated.h"

class ATargetPoint;

UENUM(BlueprintType)
enum class ECustomerState : uint8
{
    Idle,
    Patrolling,
    MovingToTarget,
    Shopping,
};

UCLASS()
class BUMPERCART_API ACustomerAI : public ACharacter, public IBumpable
{
	GENERATED_BODY()

public:
	ACustomerAI();

    virtual void BeginPlay() override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

#pragma region Setting
protected:
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    bool bIsShoppingState;

    // 최소 쇼핑 시간
    UPROPERTY(EditAnywhere, Category = "Customer | Setup")
    float MinShoppingTime = 3.0f;

    // 최대 쇼핑 시간
    UPROPERTY(EditAnywhere, Category = "Customer | Setup")
    float MaxShoppingTime = 7.0f;

public:
    FORCEINLINE float GetRandomShoppingTime() const { return FMath::FRandRange(MinShoppingTime, MaxShoppingTime); }

    FORCEINLINE bool SetIsShoppingState(bool IsShopping) { return bIsShoppingState = IsShopping; }
#pragma endregion

#pragma region WayPoint
private:
    UPROPERTY()
    TArray<TWeakObjectPtr<ATargetPoint>> CachedTargetPoints;

public:
    const TArray<TWeakObjectPtr<ATargetPoint>>& GetCachedTargetPoints() const { return CachedTargetPoints; }

#pragma endregion

};
