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

protected:
	virtual void BeginPlay() override;

    virtual void OnConstruction(const FTransform& Transform) override;

    // SphereCollision과 충돌 감지하는 함수
    UFUNCTION()
    void OnBeginOverlapCart(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    virtual void ProcessBeginOverlap(AActor* OtherActor);

    void ApplyDataAsset();

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
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Product")
    EProductState ProductState;
};
