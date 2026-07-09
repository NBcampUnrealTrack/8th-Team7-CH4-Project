#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Cart/Bumpable.h"
#include "CustomerAI.generated.h"

UENUM(BlueprintType)
enum class ECustomerState : uint8
{
    Patrolling,
    Shopping,
    KnockedOut
};

UCLASS()
class BUMPERCART_API ACustomerAI : public ACharacter, public IBumpable
{
	GENERATED_BODY()

public:
	ACustomerAI();

#pragma region Override
protected:
    virtual void BeginPlay() override;

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
#pragma endregion

#pragma region State
protected:
    // 현재 이동, 쇼핑, 충돌
    UPROPERTY(ReplicatedUsing = OnRep_CustomerState, BlueprintReadOnly, Category = "Customer | State")
    ECustomerState CurrentState;

    // 최소 쇼핑 시간
    UPROPERTY(EditAnywhere, Category = "Customer | Setup")
    float MinShoppingTime;

    // 최대 쇼핑 시간
    UPROPERTY(EditAnywhere, Category = "Customer | Setup")
    float MaxShoppingTime;

    UFUNCTION()
    void OnRep_CustomerState();

public:
    UFUNCTION(BlueprintCallable, Category = "Customer | AI")
    void StartShoppingAtShelf();

    UFUNCTION(BlueprintCallable, Category = "Customer | AI")
    void ResumePatrol();

    UFUNCTION(BlueprintCallable, Category = "Customer | AI")
    void SetKnockedOut();

    FORCEINLINE ECustomerState GetCurrentState() const { return CurrentState; }

    FORCEINLINE float GetRandomShoppingTime() const { return FMath::FRandRange(MinShoppingTime, MaxShoppingTime); }
#pragma endregion


};
