#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Cart/Component/CartLoadComponent.h"
#include "Cart/SlideAffectable.h"
#include "Cart/Bumpable.h"
#include "CartPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UCameraShakeBase;
class USoundBase;
struct FInputActionValue;
class UCartGrabComponent;
class UNiagaraComponent;

UCLASS(abstract)
class ACartPawn : public ACharacter, public ISlideAffectable, public IBumpable
{
	GENERATED_BODY()

public:
	ACartPawn();

	virtual void Tick(float DeltaSeconds) override;

	//부스터 진행 중인지
	UFUNCTION(BlueprintCallable, Category = "Cart")
	bool IsBoosting() const { return bIsBoosting; }

	//현재 적재율(0~1). SetLoadRatio로 갱신
	UFUNCTION(BlueprintCallable, Category = "Cart")
	float GetLoadRatio() const { return LoadRatio; }

	//적재율 설정 (C 연동 진입점). 0~1로 clamp
	UFUNCTION(BlueprintCallable, Category = "Cart")
	void SetLoadRatio(float InLoadRatio);

	//[ISlideAffectable] 외부 기믹(물웅덩이 등)이 호출 => 멀티캐스트로 모든 인스턴스에 미끄럼 전파
	virtual void ApplySlip_Implementation(float Duration, float SpinAngleDeg) override;

    //외부에서 카트를 강제로 밀어내기
    void ApplyExternalKnockback(const FVector& Direction, float Strength);

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//B4 : 카트가 무언가에 부딪혔을 때 호출 (충돌 => 상품 드롭 판정)
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	//B5 : 부스터 상태를 서버에 통지 (서버가 충돌 역할 판정에 사용)
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetBoosting(bool bNewBoosting);

	//미끄럼 효과를 모든 인스턴스에 전파해 각자 로컬 적용 (소유 클라의 예측 이동에도 반영)
	UFUNCTION(NetMulticast, Reliable)
	void MulticastApplySlip(float Duration, float SpinAngleDeg);

	//충돌음을 소유 클라에서 재생 (서버 NotifyHit에서 호출 — 쉐이크와 동일 패턴, 멀티캐스트 중복재생 회피)
	UFUNCTION(Client, Unreliable)
	void ClientPlayBumpSound();

    //Yaw 서버에 전달
    UFUNCTION(Server, Unreliable)
    void ServerSetCartYaw(float Yaw);

	//---------- 카메라 ----------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	//---------- 입력 액션 (BP_CartPawn에서 지정) ----------
	//입력 매핑 컨텍스트 (BP_CartPawn에 IMC_Cart 지정). 빙의 시 카트가 직접 등록 => PlayerController 종류와 무관하게 동작
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	//IMC 우선순위
	UPROPERTY(EditAnywhere, Category = "Input")
	int32 MappingContextPriority = 0;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ThrottleAction; //W/S

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SteerAction; //A/D

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* BrakeAction; //Space

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* BoostAction; //Shift

	//---------- 전후진 ----------
	//후진 최고 속도 = 전진 최고 속도 * MaxReverseSpeedRatio (쇼핑카트는 후진이 느리다)
	UPROPERTY(EditAnywhere, Category = "Cart|Throttle", meta = (ClampMin = "0", ClampMax = "1"))
	float MaxReverseSpeedRatio = 0.5f;

	//---------- 조향 튜닝 ----------
	//초당 회전 각도(도)
	UPROPERTY(EditAnywhere, Category = "Cart|Steering", meta = (ClampMin = "0"))
	float TurnRateDegPerSec = 130.f;

	//정지 상태에서의 최소 조향 배율
	UPROPERTY(EditAnywhere, Category = "Cart|Steering", meta = (ClampMin = "0", ClampMax = "1"))
	float MinSteerSpeedFactor = 0.55f;

	//조향 입력을 따라가는 속도. 낮을수록 묵직하게 늦게 먹는다(회전 지연)
	UPROPERTY(EditAnywhere, Category = "Cart|Steering", meta = (ClampMin = "0.1"))
	float SteerInterpSpeed = 5.f;

	//---------- 브레이크 ----------
	//브레이크 시 감속도
	UPROPERTY(EditAnywhere, Category = "Cart|Brake", meta = (ClampMin = "0"))
	float BrakeDeceleration = 2000.f;

    //전진 중 후진키를 눌렀을 때의 감속도 (낮을수록 천천히)
    UPROPERTY(EditAnywhere, Category = "Cart|Brake", meta = (ClampMin = "0"))
    float ReverseSlideDeceleration = 600.f;

	//---------- 부스터 ----------
	UPROPERTY(EditAnywhere, Category = "Cart|Boost", meta = (ClampMin = "1"))
	float BoostSpeedMultiplier = 1.8f;

	UPROPERTY(EditAnywhere, Category = "Cart|Boost", meta = (ClampMin = "0"))
	float BoostDuration = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Cart|Boost", meta = (ClampMin = "0"))
	float BoostCooldown = 1.0f;


    //---------- 카메라 속도감 연출 ----------
    //이 속도(cm/s)에서 줌이 최대에 도달 (0~이 값 사이를 보간)
    UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "1"))
    float SpeedZoomFullSpeed = 700.f;

    //저속/고속 카메라 암길이
    UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "0"))
    float CameraArmMin = 800.f;
    UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "0"))
    float CameraArmMax = 870.f;

    //저속/고속 FOV
    UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "1", ClampMax = "170"))
    float CameraFovMin = 90.f;
    UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "1", ClampMax = "170"))
    float CameraFovMax = 97.f;

    //부스트 시 추가 빼기/넓히기
    UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "0"))
    float BoostExtraArm = 20.f;
    UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "0"))
    float BoostExtraFov = 3.f;

    //FOV/줌 보간 속도 (낮을수록 부드럽게)
    UPROPERTY(EditAnywhere, Category = "Cart|Camera", meta = (ClampMin = "0.1"))
    float CameraZoomInterpSpeed = 7.f;


    //---------- 속도감 FX (바닥 스피드라인) ----------
    //기존 1컴포넌트
    //UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cart|FX")
    //UNiagaraComponent* SpeedLineFX;

    //양쪽 뒷바퀴 속도선 (바퀴 위치는 BP의 RelativeLocation에서 조정)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cart|FX")
    UNiagaraComponent* SpeedLineFXLeft;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cart|FX")
    UNiagaraComponent* SpeedLineFXRight;

    //이 속도 넘으면 속도선 ON 미만이면 OFF
    UPROPERTY(EditAnywhere, Category = "Cart|FX", meta = (ClampMin = "0"))
    float SpeedLineMinSpeed = 500.f;

    //---------- 그랩 컴포넌트 ----------
    // 생성자에서 생성하고, SetupPlayerInputComponent에서 IMC 바인딩
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cart|Grab")
    UCartGrabComponent* GrabComponent;

	//---------- 적재 (C 상품 시스템 연동) ----------
	//C가 만든 적재 컴포넌트. 생성자에서 부착, BeginPlay에서 적재 변경 이벤트에 바인딩
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cart|Load", meta = (AllowPrivateAccess = "true"))
	UCartLoadComponent* LoadComponent;

	//---------- 적재 무게감 ----------
	//현재 적재율 0~1
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cart|Load", meta = (ClampMin = "0", ClampMax = "1"))
	float LoadRatio = 0.f;

	//가득 실었을 때 최고 속도 배율 (낮을수록 무거우면 느림)
	UPROPERTY(EditAnywhere, Category = "Cart|Load", meta = (ClampMin = "0", ClampMax = "1"))
	float LoadMaxSpeedScale = 0.6f;

	//가득 실었을 때 회전 배율
	UPROPERTY(EditAnywhere, Category = "Cart|Load", meta = (ClampMin = "0", ClampMax = "1"))
	float LoadTurnScale = 0.6f;

	//가득 실었을 때 브레이크 배율 (낮을수록 무거우면 잘 안 멈춤)
	UPROPERTY(EditAnywhere, Category = "Cart|Load", meta = (ClampMin = "0", ClampMax = "1"))
	float LoadBrakeScale = 0.45f;

	//---------- 충돌/스필 드롭 (B4/B5) ----------
	//충격속도(cm/s)를 C 드롭 충격량으로 환산하는 배율 (충돌용)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpImpulseScale = 1.0f;

	//이 충격속도(cm/s) 미만의 약한 접촉은 무시 (충돌용)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float MinBumpSpeed = 200.f;

	//한 번 쏟은 뒤 다음 드롭까지 최소 간격(초) — 모든 스필(충돌·부스터오용) 공통
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpDropCooldown = 0.5f;

	//B5 : 부스터 오용(브레이크/급회전)으로 쏟을 때의 충격량
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BoostMisuseImpulse = 800.f;

	//B5 : 부스터 중 누적 회전각(도)이 이 값을 넘으면 쏟음
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BoostTurnSpillAngle = 60.f;

	//충돌 시 재생할 카메라 쉐이크 (BP_CartPawn에 BP_CartBumpShake 지정). 비어있으면 흔들지 않음
	UPROPERTY(EditAnywhere, Category = "Cart|Bump")
	TSubclassOf<UCameraShakeBase> BumpCameraShakeClass;

	//충돌 카메라 쉐이크 세기: 약한 충돌(MinBumpSpeed 부근)일 때의 배율
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpShakeScale = 0.4f;

	//충돌 카메라 쉐이크 세기: 강한 충돌(BumpShakeFullSpeed 이상)일 때의 배율 — 셀수록 더 세게
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpShakeMaxScale = 0.8f;

	//이 접근속도(cm/s)에서 쉐이크가 최대 세기에 도달 (MinBumpSpeed~이 값 사이를 보간)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpShakeFullSpeed = 1200.f;

	//---------- 미끄럼 (F 맵 기믹 연동: 물웅덩이 등) ----------
	//미끄럼 중 지면 마찰 (낮을수록 더 미끄러짐)
	UPROPERTY(EditAnywhere, Category = "Cart|Slip", meta = (ClampMin = "0"))
	float SlipGroundFriction = 0.f;

	//미끄럼 중 브레이크 감속도 (0이면 못 멈추고 미끄러짐)
	UPROPERTY(EditAnywhere, Category = "Cart|Slip", meta = (ClampMin = "0"))
	float SlipBrakingDeceleration = 0.f;

	//미끄럼 시작 시 강제 스핀이 풀리는 속도(도/초)
	UPROPERTY(EditAnywhere, Category = "Cart|Slip", meta = (ClampMin = "0"))
	float SlipSpinSpeedDeg = 240.f;

	//---------- 효과음 (SFX) — BP_CartPawn에서 사운드 지정. 비어있으면 무음 ----------
	//충돌(범프) 시 효과음
	UPROPERTY(EditAnywhere, Category = "Cart|SFX")
	USoundBase* BumpSound;

	//부스터 발동 시 효과음
	UPROPERTY(EditAnywhere, Category = "Cart|SFX")
	USoundBase* BoostSound;

	//브레이크 시 효과음
	UPROPERTY(EditAnywhere, Category = "Cart|SFX")
	USoundBase* BrakeSound;

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

	//스필(드롭) 공통 진입점 — 쿨다운 적용 후 C에 낙하 요청
	void RequestSpill(float Impulse, EDropCollisionRole DropRole);

	//미끄럼 시작/종료 (로컬 적용) — MulticastApplySlip에서 호출
	void StartSlip(float Duration, float SpinAngleDeg);
	void EndSlip();

	//C 적재 정보가 바뀔 때 호출되는 델리게이트 핸들러. AddDynamic용
	UFUNCTION()
	void HandleLoadInfoChanged(AActor* OwnerActor, const FLoadInfo& LoadInfo);

private:
	//현재 프레임 입력값
	float ThrottleInput = 0.f;
	float SteerInput = 0.f;
	float CurrentSteer = 0.f; //부드럽게 보간된 조향값
	bool bIsBraking = false;

	//부스터 상태 (bIsBoosting: 충돌 역할 판정 위해 서버 통지 + 타 클라 복제)
	UPROPERTY(Replicated)
	bool bIsBoosting = false;
	bool bBoostOnCooldown = false;

	//기본값 백업(부스터/브레이크 후 복구용)
	float DefaultMaxWalkSpeed = 0.f;
	float DefaultBrakingDeceleration = 0.f;

	FTimerHandle BoostTimerHandle;
	FTimerHandle BoostCooldownTimerHandle;

	//마지막으로 스필(드롭)을 요청한 시각 — BumpDropCooldown 공통 적용
	float LastBumpDropTime = -1000.f;

	//충돌 직전 프레임의 속도 (NotifyHit 시점엔 비물리라 속도가 0으로 깎여 신뢰 불가 => Tick에서 매 프레임 캐시)
	FVector PreviousVelocity = FVector::ZeroVector;

    //서버에 마지막으로 보낸 Yaw
    float LastSentYaw = 0.f;

	//부스터 중 누적 회전각(도). 부스터 시작 시 0으로 리셋
	float BoostTurnAccumDeg = 0.f;

	//미끄럼(슬립) 상태
	bool bIsSlipping = false;
	float SlipTimeRemaining = 0.f;     //남은 미끄럼 시간(초)
	float SlipSpinRemainingDeg = 0.f;  //남은 강제 스핀 각(도)
	float DefaultGroundFriction = 0.f; //미끄럼 후 마찰 복구용 백업
};
