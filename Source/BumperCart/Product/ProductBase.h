// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProductTypes.h"
#include "GameFramework/Actor.h"
#include "ProductBase.generated.h"

class UProductDataAsset;
class UStaticMeshComponent;
class USphereComponent;

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

protected:
	virtual void BeginPlay() override;

    virtual void OnConstruction(const FTransform& Transform) override;

    void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

    // SphereCollision과 충돌 감지하는 함수
    UFUNCTION()
    void OnBeginOverlapCart(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    // BeginOverlap 내부에서 호출될 가상 함수
    virtual void ProcessBeginOverlap(AActor* OtherActor);

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
    TObjectPtr<USphereComponent> SphereCollision;


    /* Product 기본 변수 */

    // 사용할 데이터 에셋
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
    TObjectPtr<UProductDataAsset> ProductDataAsset;

    // 상품 정보
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Product")
    FProductData ProductData;

    // 상품의 현재 상태
    UPROPERTY(ReplicatedUsing = OnRep_ProductState, VisibleAnywhere, BlueprintReadOnly, Category = "Product")
    EProductState ProductState;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
    float ReturnDelay;

private:
    UFUNCTION()
    void HandleReturnDisplay();

private:
    FTimerHandle ReturnDisplayTimer;
};
