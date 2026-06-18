//BumperCart - B(카트/플레이어 조작) 파트
//CartPawn: 쇼핑카트 플레이어 폰. ACharacter 기반 직접 제어(완전 물리 X).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CartPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

UCLASS(abstract)
class ACartPawn : public ACharacter
{
	GENERATED_BODY()

public:
	ACartPawn();

	virtual void Tick(float DeltaSeconds) override;

	//부스터 진행 중인지
	UFUNCTION(BlueprintCallable, Category = "Cart")
	bool IsBoosting() const { return bIsBoosting; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//---------- 카메라 ----------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	//---------- 입력 액션 (BP_CartPawn에서 지정) ----------
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ThrottleAction; //W/S (Axis1D)

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SteerAction; //A/D (Axis1D)

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* BrakeAction; //Space (Bool)

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* BoostAction; //Shift (Bool)

	//---------- 전후진 ----------
	// 후진 최고 속도 = 전진 최고 속도 * MaxReverseSpeedRatio (쇼핑카트는 후진이 느리다)
	UPROPERTY(EditAnywhere, Category = "Cart|Throttle", meta = (ClampMin = "0", ClampMax = "1"))
	float MaxReverseSpeedRatio = 0.5f;

	//---------- 조향 튜닝 ----------
	//초당 회전 각도(도)
	UPROPERTY(EditAnywhere, Category = "Cart|Steering", meta = (ClampMin = "0"))
	float TurnRateDegPerSec = 130.f;

	//정지 상태에서의 최소 조향 배율
	UPROPERTY(EditAnywhere, Category = "Cart|Steering", meta = (ClampMin = "0", ClampMax = "1"))
	float MinSteerSpeedFactor = 0.35f;

	//조향 입력을 따라가는 속도. 낮을수록 묵직하게 늦게 먹는다(회전 지연)
	UPROPERTY(EditAnywhere, Category = "Cart|Steering", meta = (ClampMin = "0.1"))
	float SteerInterpSpeed = 3.f;

	//---------- 브레이크 ----------
	//브레이크 시 감속도
	UPROPERTY(EditAnywhere, Category = "Cart|Brake", meta = (ClampMin = "0"))
	float BrakeDeceleration = 2000.f;

	//---------- 부스터 ----------
	UPROPERTY(EditAnywhere, Category = "Cart|Boost", meta = (ClampMin = "1"))
	float BoostSpeedMultiplier = 1.8f;

	UPROPERTY(EditAnywhere, Category = "Cart|Boost", meta = (ClampMin = "0"))
	float BoostDuration = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Cart|Boost", meta = (ClampMin = "0"))
	float BoostCooldown = 2.5f;

protected:
	//---------- 입력 핸들러 ----------
	void OnThrottle(const FInputActionValue& Value);
	void OnThrottleReleased(const FInputActionValue& Value);
	void OnSteer(const FInputActionValue& Value);
	void OnSteerReleased(const FInputActionValue& Value);
	void OnBrakeStart(const FInputActionValue& Value);
	void OnBrakeStop(const FInputActionValue& Value);
	void OnBoost(const FInputActionValue& Value);

	void EndBoost();
	void ResetBoostCooldown();

private:
	//현재 프레임 입력값
	float ThrottleInput = 0.f;
	float SteerInput = 0.f;
	float CurrentSteer = 0.f; //부드럽게 보간된 조향값
	bool bIsBraking = false;

	//부스터 상태
	bool bIsBoosting = false;
	bool bBoostOnCooldown = false;

	//기본값 백업(부스터/브레이크 후 복구용)
	float DefaultMaxWalkSpeed = 0.f;
	float DefaultBrakingDeceleration = 0.f;

	FTimerHandle BoostTimerHandle;
	FTimerHandle BoostCooldownTimerHandle;
};
