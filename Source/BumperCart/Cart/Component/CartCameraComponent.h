// BumperCart - B(카트/플레이어 조작) 파트
// 카트 카메라 연출 컴포넌트 — 속도 줌/FOV + 커스텀 카메라 충돌(벽/진열대 관통 방지). 소유 클라 로컬 전용

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CartCameraComponent.generated.h"

class ACartPawn;
class USpringArmComponent;
class UCameraComponent;

//카트 카메라의 속도감 연출(줌·FOV)과 관통 방지 충돌을 전담. 카메라 셰이크 RPC는 CartPawn에 유지
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BUMPERCART_API UCartCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCartCameraComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//---------- 속도감 연출 ----------
	//이 속도(cm/s)에서 줌이 최대에 도달 (0~이 값 사이를 보간). 기본 최고 속도(BaseMaxWalkSpeed)에 맞춤
	UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "1"))
	float SpeedZoomFullSpeed = 1050.f;

	//저속/고속 카메라 암길이
	UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "0"))
	float CameraArmMin = 800.f;
	UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "0"))
	float CameraArmMax = 850.f;

	//저속/고속 FOV (폭을 작게 — 가감속 반복 시 FOV 출렁임/멀미 완화)
	UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "1", ClampMax = "170"))
	float CameraFovMin = 90.f;
	UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "1", ClampMax = "170"))
	float CameraFovMax = 105.f;

	//부스트 시 추가 빼기/넓히기
	UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "0"))
	float BoostExtraArm = 20.f;
	UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "0"))
	float BoostExtraFov = 10.f;

	//FOV/줌 보간 속도 (낮을수록 부드럽게 — 가감속 시 FOV가 천천히 반응해 멀미 완화)
	UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "0.1"))
	float CameraZoomInterpSpeed = 3.f;

	//---------- 카메라 충돌(벽/진열대 관통 방지) ----------
	//직접 스윕으로 카메라 암을 당긴다 — 스프링암 기본 충돌은 즉시 스냅이라 가판대 스칠 때 시점이 튀어서 커스텀 스무딩 사용
	//충돌 탐지 구 반경 (클수록 벽에서 더 여유 두고 당김)
	UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "0"))
	float CameraCollisionProbeSize = 12.f;

	//당겨올 때(장애물 감지) 보간 속도 — 빠르게(관통 최소화). 스냅은 아니라 어지럼 방지
	UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "0.1"))
	float CameraCollisionPullInSpeed = 12.f;

	//카메라가 당겨질 수 있는 최소 암 길이 — 이 이하로는 안 당기고 차라리 살짝 걸치게 둠 (극단적 클로즈업으로 시점 망침 방지)
	UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "0"))
	float CameraCollisionMinArm = 400.f;

	//풀어줄 때(장애물 벗어남) 보간 속도 — 천천히(가판대 사이 스칠 때 출렁임 방지)
	UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "0.1"))
	float CameraCollisionPullOutSpeed = 4.f;

private:
	//소유 카트 + 스프링암/카메라 캐시 (BeginPlay 1회)
	UPROPERTY(Transient)
	TObjectPtr<ACartPawn> OwnerCart;
	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> FollowCamera;
};
