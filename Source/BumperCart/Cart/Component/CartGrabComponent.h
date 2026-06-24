// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CartGrabComponent.generated.h"

class AProductBase;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BUMPERCART_API UCartGrabComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCartGrabComponent();

    // IMC, IA를 설정하는 함수, Pawn의 SetupPlayerInputComponent에서 호출함
    // 테스트용으로 UFUNCTION 열어둠
    UFUNCTION(BlueprintCallable)
    void SetupInput(UInputComponent* PlayerInputComponent);

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // 게임 시작 시 마우스 조준선 타이머 시작하는 함수
    void StartAimTimer();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cart|Input")
    TObjectPtr<class UInputMappingContext> GrabMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cart|Input")
    TObjectPtr<class UInputAction> GrabAction;

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

    // 마우스 에임을 시각화 하는 함수
    void ShowMouseAim(const FVector& Start);

    // Grab Action에 바인딩되어 호출하는 함수
    // 그랩 방향을 구해 서버로 요청 준비
    void RequestGrab();

    // 서버 RPC, 조준된 방향으로 상품을 잡는 함수
    UFUNCTION(Server, Reliable)
    void Server_GrabProduct(FVector_NetQuantizeNormal AimDirection);

    // 멀티캐스트 RPC, 로봇손 뻗는 연출 실행하라고 요청하는 함수
    // Duration을 받아서 해당 시간동안 손을 뻗고, 회수하면 됨
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_StretchGrab(FVector_NetQuantize StartLocation, FVector_NetQuantize EndLocation, float Duration);

    bool PerformGrabTrace(FVector_NetQuantizeNormal AimDirection, FHitResult& Hit);

    // 게임 종료 시 마우스 조준선 타이머 끄는 함수
    void StopAimTimer();

    void SetGrabCooldown(float Duration);

private:
    // 붙잡은 상품 관찰용 포인터
    UPROPERTY()
    TWeakObjectPtr<AProductBase> GrabbedProduct;

    // 저장한 조준 방향
    FVector CachedAimDirection;
    // 저장한 조준 위치
    FVector CachedAimTargetLocation;

    // 마우스 조준선 갱신 빈도
    UPROPERTY(EditAnywhere, Category = "Cart|Grab")
    float AimUpdateInterval;

    // 로봇손 속도
    // 최대 사거리 X2 가 몇초가 될건지를 기준으로 조정
    UPROPERTY(EditAnywhere, Category = "Cart|Grab")
    float GrabSpeed;

    // 로봇손 사거리
    UPROPERTY(EditAnywhere, Category = "Cart|Grab")
    float GrabRange;

    // 로봇손 트레이스 크기
    UPROPERTY(EditAnywhere, Category = "Cart|Grab")
    float GrabRadius;

    // 현재 그랩 가능한지
    UPROPERTY(VisibleAnywhere, Category = "Cart|Grab")
    bool bCanGrab;

    // 아이템이 부착될 소켓 이름
    UPROPERTY(EditAnywhere, Category = "Cart|Grab")
    FName SocketName;

    // 그랩 종료 확인용 타이머
    FTimerHandle GrabFinishTimer;

    // 조준선 갱신 타이머
    FTimerHandle GrabAimUpdateTimer;
};
