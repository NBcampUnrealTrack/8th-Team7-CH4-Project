// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProductTypes.h"
#include "GameFramework/Actor.h"
#include "ProductBase.generated.h"

class UProductDataAsset;
class UStaticMeshComponent;
class USphereComponent;
class UStaticMesh;


UCLASS()
class BUMPERCART_API AProductBase : public AActor
{
	GENERATED_BODY()

public:
	AProductBase();

    virtual void Destroyed() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable)
    void StartSpawn(const FVector& StartLocation, const FVector& EndLocation, AActor* IgnoreActor);

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

    virtual void Tick(float DeltaTime) override;

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
    TObjectPtr<USphereComponent> ProductCollision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Product|Component")
    TObjectPtr<UStaticMeshComponent> Mesh;

    /* Product 기본 변수 */

    // 사용할 데이터 에셋
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
    TObjectPtr<UProductDataAsset> ProductDataAsset;

    // 상품의 현재 상태, 드롭 위치를 저장하는 구조체
    UPROPERTY(ReplicatedUsing = OnRep_ProductState, VisibleAnywhere, BlueprintReadOnly, Category = "Product")
    FProductRepState ProductState;

    // 이벤트 대상 상품인지 확인하는 변수
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Product")
    bool bOnSale;


    /* 메시의 기본 상대좌표 값 캐싱용 */

    // 메시의 기본 상대 위치값
    UPROPERTY()
    FVector BaseMeshLocation;

    // 메시의 기본 상대 회전값
    UPROPERTY()
    FRotator BaseMeshRotation;


    /* Bobbing 관련 변수들 */

    // 회전한지 얼마나 지났는지 누적용
    float ElapsedTime;

    // 배치할 높이 오프셋
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Bob")
    float HeightOffset;

    // 위아래 진폭
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Bob")
    float BobbingAmplitude;

    // 공중에서 위아래로 움직이는 속도
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Bob")
    float BobbingSpeed;

    // 공중에서 좌우로 회전하는 속도
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Bob")
    float RotationSpeed;

    // 포물선 운동 경과시간
    float LaunchElapsedTime;


    /* Falling 관련 변수들 */

    // Falling 시 최소 높이
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Falling")
    float FallingMinHeight;

    // Falling 시 최대 높이
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Falling")
    float FallingMaxHeight;

    // Falling 시 도착지점 수평 오프셋 값
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Falling")
    float FallingHorizontalOffset;

    // Falling 포물선 운동 지속시간
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Falling")
    float FallingDuration;


    /* Spawning 관련 변수들 */

    // 스폰 시 연출 지속시간
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Spawn")
    float SpawningDuration;

    // 스폰시 튀어오르는 높이
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product|Spawn")
    float SpawningHeight;

    // 스폰할때 무시할 액터(가판대)
    UPROPERTY()
    TWeakObjectPtr<AActor> LaunchIgnoredActor;

private:
    bool CanLoad() const;

    bool CanGrab() const;

    void TickDisplay(float DeltaTime);

    // 포물선 운동 시작
    void StartLaunch(EProductState State, const FVector& StartLocation, const FVector& EndLocation,
        float InHeight, float InDuration, AActor* IgnoreActor);

    // 포물선 운동 진행
    void TickLaunch(float DeltaTime);

    bool IsLaunchState() const;

    // 가판대 안쪽이 DropEnd가 되지 않게 바깥쪽 위치를 구해주는 함수
    FVector GetSafeLocation(const FVector& Start, const FVector& End, AActor* IgnoreActor);

};
