// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Product/ProductTypes.h"
#include "Cart/CartLoadTypes.h"
#include "CartLoadComponent.generated.h"


class AProductBase;
class UStaticMeshComponent;
class UStaticMesh;
class UNiagaraSystem;
class USoundBase;

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

    // 상품 적재를 시도하는 함수
    bool TryAddProduct(AProductBase* Product);

    UFUNCTION(BlueprintCallable)
    void RequestDropProduct(float Impulse, EDropCollisionRole Role);

    // 받은 충격량에 따라 상품을 떨어뜨리는 함수
    UFUNCTION(BlueprintCallable)
    void DropProducts(float Impulse, EDropCollisionRole Role);

    int32 GetTotalValue() const;

    int32 GetCurrentLoadedCount() const;

    float GetLoadRatio() const;

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
    int32 CalculateDropCount(float Impulse, EDropCollisionRole Role) const;

    // 충돌 역할에따라 가중치를 반환하는 함수
    float GetCollisionRoleMultiplier(EDropCollisionRole Role) const;

    // 적재 정보를 갱신하는 함수
    void UpdateLoadInfo();

    // 서버 RPC, 클라이언트에서 테스트 용도
    UFUNCTION(Server, Reliable)
    void Server_RequestDropProducts(float Impulse, EDropCollisionRole Role);

    // 더미 메시 생성하는 함수
    void CreateDummyMeshes();

    // 더미 메시 연출 갱신하는 함수
    void UpdateLoadVisual();

    // 더미 상품을 몇개 보여줄지 계산하는 함수
    int32 GetVisibleDummyCount() const;

    // 카트 적재 이펙트를 재생하는 멀티캐스트 함수
    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayCartLoadEffect(int32 DummyIndex);

private:
    /* ---------------- 적재 연출 관련 ---------------- */

    // 적재된 더미 메시들
    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> LoadDummyMeshes;

    // 부착할 더미 소켓 이름 관리
    UPROPERTY(EditDefaultsOnly, Category = "Cart|Load|Visual")
    TArray<FName> LoadDummySocketNames;

    // 더미로 사용할 실제 메시
    UPROPERTY(EditDefaultsOnly, Category = "Cart|Load|Visual")
    TObjectPtr<UStaticMesh> LoadDummyMesh;

    // 적재 시 나이아가라 연출
    UPROPERTY(EditDefaultsOnly, Category = "Cart|Load|Visual")
    TObjectPtr<UNiagaraSystem> LoadEffect;

    UPROPERTY(EditDefaultsOnly, Category = "Cart|Grab|Sound")
    TObjectPtr<USoundBase> LoadSound;


    /* ---------------- 적재 관리 변수 ---------------- */

    // 적재중인 상품들을 관리하는 배열
    UPROPERTY()
    TArray<TObjectPtr<AProductBase>> LoadedProducts;

    // 최대 무게 제한이 아니고, 속도 임계값 용도로 사용
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cart|Load", meta = (AllowPrivateAccess = "true"))
    int32 MaxWeight;

    // 현재 무게, 적재 개수를 구조체로 관리
    UPROPERTY(ReplicatedUsing = OnRep_LoadInfo, VisibleAnywhere, BlueprintReadOnly, Category = "Cart|Load", meta = (AllowPrivateAccess = "true"))
    FLoadInfo LoadInfo;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cart|Drop", meta = (AllowPrivateAccess = "true"))
    TArray<FDropCountRule> DropCountRules;

    // 무게 비례 보정치, 현재 무게/최대 무게 비율을 최종 곱에 얼만큼 적용할건지
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cart|Load", meta = (AllowPrivateAccess = "true"))
    float WeightScaling;

    // 부스터 사용한 사람의 충돌 보정치, 얼마나 적게 떨어뜨릴건지
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cart|Load", meta = (AllowPrivateAccess = "true"))
    float BoosterInstigatorDropMultiplier;

    // 부스터 충돌 당한 사람의 드롭 보정치, 얼마나 더 떨어뜨릴건지
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cart|Load", meta = (AllowPrivateAccess = "true"))
    float BoostedTargetDropMultiplier;
};
