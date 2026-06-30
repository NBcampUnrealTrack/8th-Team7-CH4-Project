// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CartGrabComponent.generated.h"

class AProductBase;
class UInputMappingContext;
class UInputAction;
class USplineMeshComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UDecalComponent;

UENUM(BlueprintType)
enum class EGrabVisualState : uint8
{
    None,
    Extending,
    Returning
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BUMPERCART_API UCartGrabComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCartGrabComponent();

    // IMC, IA를 설정하는 함수, Pawn의 SetupPlayerInputComponent에서 호출함
    // 테스트용으로 UFUNCTION 열어둠
    UFUNCTION(BlueprintCallable)
    void SetupInput();

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // 게임 시작 시 마우스 조준선 타이머 시작하는 함수
    void StartAimTimer();

protected:
    virtual void BeginPlay() override;

    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cart|Input")
    TObjectPtr<UInputMappingContext> GrabMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cart|Input")
    TObjectPtr<UInputAction> GrabAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cart|Input")
    int32 MappingPriority;

private:
    // 타이머에 바인딩되어 그랩 종료시 호출하는 함수
    // 실제 상품을 적재하는 처리를 함
    UFUNCTION()
    void HandleFinishGrab();

    // 타이머에 바인딩되어 조준선을 갱신하는 함수
    UFUNCTION()
    void UpdateGrabAim();

    UFUNCTION()
    void TryGrabProduct();

    // Grab Action에 바인딩되어 호출하는 함수
    // 그랩 방향을 구해 서버로 요청 준비
    void RequestGrab();

    // 서버 RPC, 조준된 방향으로 상품을 잡는 함수
    UFUNCTION(Server, Reliable)
    void Server_GrabProduct(FVector_NetQuantizeNormal AimDirection, float AimDistance);

    // 멀티캐스트 RPC, 로봇손 뻗는 연출 실행하라고 요청하는 함수
    // Duration을 받아서 해당 시간동안 손을 뻗고, 회수하면 됨
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayGrab(FVector_NetQuantize Start, FVector_NetQuantize Target);

    bool PerformGrabTrace(FVector_NetQuantizeNormal AimDirection, float AimDistance, FHitResult& Hit);

    // 게임 종료 시 마우스 조준선 타이머 끄는 함수
    void StopAimTimer();

    void SetGrabCooldown(float Duration);

    // 로봇손 연출 갱신하는 함수
    void UpdateGrabVisual(const FVector& Location);

    // 로봇손 뻗는 연출 시작하는 함수
    void PlayGrabVisual(const FVector& Start, const FVector& Target);

    // 로봇손 연출 종료하는 함수
    void FinishGrabVisual();

    // 연출용 메시 보여주기
    void ShowVisualProductMesh(AProductBase* Product);

    // 연출용 메시 끄기
    void HideVisualProductMesh();

    // 모든 클라이언트에게 잡은 상품을 보여주라고 알리는 함수
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_ShowVisualProductMesh(AProductBase* Product);

    // 현재 로봇손 시작위치 구하는 함수
    FVector GetGrabStartLocation() const;

    // Owner Actor에 로봇손 스플라인 메시를 확인하고 생성하는 함수
    void EnsureVisualComponents();

    // 조준선 나이아가라 확인하고 생성하는 함수
    void EnsureAimNiagara();

    // 조준선용 나이아가라 업데이트하는 함수
    void UpdateAimNiagaraVisual(const FVector& Start, const FVector& End);

    // 조준선 연출 설정하는 함수
    void SetAimVisual(bool bVisibility);

    // 마우스 위치에 데칼 확인하고 생성하는 함수
    void EnsureAimDecal();

    // 마우스 위치에 데칼 업데이트하는 함수
    void UpdateAimDecal(const FVector& Target);

private:
    /* ---------------- 로봇손 연출 관련 ---------------- */

    // 로봇손 팔에 사용할 에셋
    UPROPERTY(EditDefaultsOnly, Category = "Cart|Grab|Visual")
    TObjectPtr<UStaticMesh> ArmMeshAsset;

    // 로봇손 손에 사용할 에셋
    UPROPERTY(EditDefaultsOnly, Category = "Cart|Grab|Visual")
    TObjectPtr<UStaticMesh> HandMeshAsset;

    // 로봇손 팔 스플라인 메시 컴포넌트
    UPROPERTY()
    TObjectPtr<USplineMeshComponent> ArmSpline;

    // 로봇손 손부분 메시 컴포넌트
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> Hand;

    // 붙잡은 상품 관찰용 포인터
    UPROPERTY()
    TWeakObjectPtr<AProductBase> GrabbedProduct;

    // 연출용 상품 메시
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> VisualProductMesh;

    // 연출관련 변수들
    FVector VisualStartLocation;
    FVector VisualTargetLocation;
    float VisualReachDuration;
    float VisualReturnDuration;
    float VisualElapsedTime;

    // 현재 연출 상태
    EGrabVisualState VisualState;


    /* ---------------- 조준선 갱신 관련 변수들  ----------------   */

    // 저장한 조준 방향
    FVector CachedAimDirection;
    // 저장한 조준 위치
    FVector CachedAimTargetLocation;
    // 저장한 조준선 까지의 길이
    float CachedAimDistance;

    // 마우스 조준선 갱신 빈도
    UPROPERTY(EditAnywhere, Category = "Cart|Grab|Aim")
    float AimUpdateInterval;


    /* ------------------ 조준선 나이아가라 ------------------ */

    // 조준선용 나이아가라 컴포넌트
    UPROPERTY()
    TObjectPtr<UNiagaraComponent> AimNiagaraComponent;

    // 조준선용 나니아가라 시스템
    UPROPERTY(EditDefaultsOnly, Category = "Cart|Grab|Aim")
    TObjectPtr<UNiagaraSystem> AimNiagaraSystem;

    // 마우스 위치에 그릴 데칼
    UPROPERTY()
    TObjectPtr<UDecalComponent> AimDecal;

    // 데칼에 사용할 머티리얼
    UPROPERTY(EditDefaultsOnly, Category = "Cart|Grab|Aim")
    TObjectPtr<UMaterialInterface> AimDecalMaterial;

    // 나이아가라 점 간격
    UPROPERTY(EditAnywhere, Category = "Cart|Grab|Aim")
    float AimDotSpacing;

    // 나이아가라 점 크기
    UPROPERTY(EditAnywhere, Category = "Cart|Grab|Aim")
    float AimDotSize;

    // 나이아가라 표시 높이 오프셋
    UPROPERTY(EditAnywhere, Category = "Cart|Grab|Aim")
    float NiagaraHeightOffset;


    /* ---------------- 로봇손 밸런스 ---------------- */

    // 로봇손 속도
    // 최대 사거리가 몇초가 될건지를 기준으로 조정
    UPROPERTY(EditAnywhere, Category = "Cart|Grab")
    float GrabSpeed;

    // 로봇손 사거리
    UPROPERTY(EditAnywhere, Category = "Cart|Grab")
    float GrabRange;

    // 로봇손 트레이스 크기
    UPROPERTY(EditAnywhere, Category = "Cart|Grab")
    float GrabRadius;


    /* ---------------- 로봇손 확인용 ---------------- */

    // 현재 그랩 가능한지
    UPROPERTY(VisibleAnywhere, Category = "Cart|Grab")
    bool bCanGrab;

    // 아이템이 부착될 소켓 이름
    UPROPERTY(EditAnywhere, Category = "Cart|Grab")
    FName SocketName;


    /* ---------------- 타이머 ---------------- */

    // 그랩 종료 확인용 타이머
    FTimerHandle GrabFinishTimer;

    // 조준선 갱신 타이머
    FTimerHandle GrabAimUpdateTimer;

    // 상품과 손이 닿을 시간을 확인하는 타이머
    FTimerHandle TryGrabTimer;
};
