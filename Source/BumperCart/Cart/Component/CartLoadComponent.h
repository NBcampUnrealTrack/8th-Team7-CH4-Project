// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Product/ProductTypes.h"
#include "CartLoadComponent.generated.h"


class AProductBase;

USTRUCT(BlueprintType)
struct FLoadInfo
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly)
    int32 CurrentLoadedCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 CurrentWeight = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnLoadInfoChanged,
    AActor*, OwnerActor,
    const FLoadInfo&, LoadInfo
);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BUMPERCART_API UCartLoadComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCartLoadComponent();

    // 기본 값으로 초기화하는 함수
    void Initialize();

    // 상품 적재를 시도하는 함수
    bool TryAddProduct(AProductBase* Product);

    UFUNCTION(BlueprintCallable)
    void RequestDropProduct(float Impulse);

    // 받은 충격량에 따라 상품을 떨어뜨리는 함수
    UFUNCTION(BlueprintCallable)
    void DropProducts(float Impulse);

    int32 GetTotalValue() const;

    int32 GetCurrentLoadedCount() const;

    // 계산대에서 상품 정산 시 호출하는 함수
    // 상품 정보 구조체 배열을 채우고, 현재 적재된 상품을 전부 제거함
    bool CheckoutProducts(TArray<FLoadedProductInfo>& OutProducts);

public:
    UPROPERTY(BlueprintAssignable)
    FOnLoadInfoChanged OnLoadInfoChanged;

protected:
    virtual void BeginPlay() override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION()
    void OnRep_LoadInfo();

private:
    // 충격량에 따라 몇개를 떨어뜨릴지 계산하는 함수
    int32 CalculateDropCount(float Impulse) const;

    // 적재 정보를 갱신하는 함수
    void UpdateLoadInfo();

    UFUNCTION(Server, Reliable)
    void Server_RequestDropProducts(float Impulse);

private:
    // TArray를 복제할지, 무게, 적재량을 복제할지
    // 결국 둘다 같은 타이밍에 복제되긴 함
    UPROPERTY()
    TArray<TObjectPtr<AProductBase>> LoadedProducts;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cart", meta = (AllowPrivateAccess = "true"))
    int32 MaxLoadedCount;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cart", meta = (AllowPrivateAccess = "true"))
    int32 MaxWeight;

    // 현재 무게, 적재 개수를 구조체로 관리
    UPROPERTY(ReplicatedUsing = OnRep_LoadInfo, VisibleAnywhere, BlueprintReadOnly, Category = "Cart", meta = (AllowPrivateAccess = "true"))
    FLoadInfo LoadInfo;
};
