// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProductTypes.h"
#include "GameFramework/Actor.h"
#include "ProductBase.generated.h"

class UProductDataAsset;
class UStaticMeshComponent;
class USphereComponent;
class UProductDropConfig;


UCLASS()
class BUMPERCART_API AProductBase : public AActor
{
	GENERATED_BODY()

public:
	AProductBase();

    // 스폰할 때 위치 설정 및 상태를 초기화하는 함수
    void Initialize(const FVector& SpawnLocation);

    // 상품의 상태를 설정하는 함수, 서버에서 처리함
    void SetProductState(EProductState NewState);

    // Loaded 상태로 변환 시도하는 함수
    bool TrySetLoaded();

    // Grabbed 상태로 변환 시도하는 함수
    bool TrySetGrabbed();

    // 카트에서 해당 상품을 떨어뜨리는 함수
    void DropFromCart(AActor* CartActor);

    UFUNCTION(BlueprintPure)
    int32 GetWeight() const;

    UFUNCTION(BlueprintPure)
    int32 GetValue() const;

    UFUNCTION(BlueprintPure)
    EProductState GetProductState() const;

    UFUNCTION(BlueprintPure)
    FLoadedProductInfo GetLoadedProductInfo() const;

    UStaticMesh* GetProductMesh() const;

    UFUNCTION(BlueprintCallable)
    void SetOnSale(bool NewValue);

    UFUNCTION(BlueprintPure)
    bool IsOnSale() const;

protected:
	virtual void BeginPlay() override;

    virtual void OnConstruction(const FTransform& Transform) override;

    void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

    // 데이터 에셋을 적용하는 함수
    void ApplyDataAsset();

    // 함수의 상태를 적용하는 함수
    void ApplyProductState();

    UFUNCTION()
    void OnRep_ProductState();

protected:
    /* 컴포넌트 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Product|Component")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Product|Component")
    TObjectPtr<USphereComponent> GrabCollision;


    /* Product 기본 변수 */

    // 사용할 데이터 에셋
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
    TObjectPtr<UProductDataAsset> ProductDataAsset;

    // Drop 관련 설정
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Drop")
    TObjectPtr<UProductDropConfig> DropConfig;

    // 상품의 현재 상태, 드롭 위치를 저장하는 구조체
    UPROPERTY(ReplicatedUsing = OnRep_ProductState, VisibleAnywhere, BlueprintReadOnly, Category = "Product")
    FProductRepState ProductState;

    // 이벤트 대상 상품인지 확인하는 변수
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Product")
    bool bOnSale;

private:
    UFUNCTION()
    void HandleReturnDisplay();

    bool CanLoad() const;

    bool CanGrab() const;

private:
    FTimerHandle ReturnDisplayTimer;
};
