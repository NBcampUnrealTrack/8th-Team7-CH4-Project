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
class UCartScreenFXComponent;
class UCartItemInventoryComponent;
class UStaticMeshComponent;

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

	//브레이크 중인지 (FX 등 연출용. 타 클라에도 복제됨)
	UFUNCTION(BlueprintCallable, Category = "Cart")
	bool IsBraking() const { return bIsBraking; }

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

    //토마토에 맞은 소유 클라 화면에 가림 위젯 표시 (서버 => 소유 클라)
    UFUNCTION(Client, Reliable)
    void ClientApplyTomatoScreenBlock(float Duration);

    //카메라 셰이크를 이 카트 소유 클라 화면에 재생 (서버에서 호출 => 소유 클라 실행).
    //범프 충돌 외에도 거대 카트 충돌·아이템 드롭 등 다른 시스템이 공용으로 호출. ShakeClass가 비면 카트 기본(BumpCameraShakeClass) 사용
    UFUNCTION(Client, Reliable, BlueprintCallable, Category = "Cart|Camera")
    void ClientPlayCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale);

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//B4 : 카트가 무언가에 부딪혔을 때 호출 (충돌 => 상품 드롭 판정)
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	//B5 : 부스터 상태를 서버에 통지 (서버가 충돌 역할 판정에 사용)
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetBoosting(bool bNewBoosting);

	//브레이크 상태를 서버에 통지 (타 클라 스파크 연출용 — 부스터와 동일 패턴)
	UFUNCTION(Server, Reliable)
	void ServerSetBraking(bool bNewBraking);

	//미끄럼 효과를 모든 인스턴스에 전파해 각자 로컬 적용 (소유 클라의 예측 이동에도 반영)
	UFUNCTION(NetMulticast, Reliable)
	void MulticastApplySlip(float Duration, float SpinAngleDeg);

	//충돌음을 소유 클라에서 재생 (서버 NotifyHit에서 호출 — 쉐이크와 동일 패턴, 멀티캐스트 중복재생 회피)
	UFUNCTION(Client, Unreliable)
	void ClientPlayBumpSound();

	//충돌 리액션(몸통 들썩·기울임)을 모든 클라에 재생. 서버에서만 호출
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayBumpReaction(FVector WorldPushDir, float Intensity);

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
	//기본 전진 최고 속도(cm/s). 부스트 속도 = 이 값 * BoostSpeedMultiplier 로 자동 계산됨. (BeginPlay에서 이동 컴포넌트에 적용)
	UPROPERTY(EditAnywhere, Category = "Cart|Throttle", meta = (ClampMin = "0"))
	float BaseMaxWalkSpeed = 1050.f;

	//기본 가속도(cm/s^2). 최고 속도에 얼마나 빨리 도달하는지 (BeginPlay에서 이동 컴포넌트에 적용)
	UPROPERTY(EditAnywhere, Category = "Cart|Throttle", meta = (ClampMin = "0"))
	float BaseMaxAcceleration = 1800.f;

	//후진 최고 속도 = 전진 최고 속도 * MaxReverseSpeedRatio (쇼핑카트는 후진이 느리다)
	UPROPERTY(EditAnywhere, Category = "Cart|Throttle", meta = (ClampMin = "0", ClampMax = "1"))
	float MaxReverseSpeedRatio = 0.5f;

	//---------- 조향 튜닝 ----------
	//초당 회전 각도(도)
	UPROPERTY(EditAnywhere, Category = "Cart|Steering", meta = (ClampMin = "0"))
	float TurnRateDegPerSec = 145.f;

	//정지 상태에서의 최소 조향 배율
	UPROPERTY(EditAnywhere, Category = "Cart|Steering", meta = (ClampMin = "0", ClampMax = "1"))
	float MinSteerSpeedFactor = 0.55f;

	//조향 입력을 따라가는 속도. 낮을수록 묵직하게 늦게 먹는다(회전 지연)
	UPROPERTY(EditAnywhere, Category = "Cart|Steering", meta = (ClampMin = "0.1"))
	float SteerInterpSpeed = 5.f;

	//원격(타 클라·서버)에서 이 카트를 복제된 yaw로 따라잡는 보간 속도. 낮으면 회전이 느리게 돌고, 너무 높으면 각지게 튄다
	UPROPERTY(EditAnywhere, Category = "Cart|Steering", meta = (ClampMin = "1"))
	float RemoteYawInterpSpeed = 15.f;

	//---------- 브레이크 ----------
	//브레이크 시 감속도 (낮을수록 제동거리가 길다 — 일부러 무겁게: 확 끌려가다 멈추는 불편한 조작감 의도)
	UPROPERTY(EditAnywhere, Category = "Cart|Brake", meta = (ClampMin = "0"))
	float BrakeDeceleration = 150.f;

	//이 속도(cm/s) 이하로 느려지면 브레이크 키를 눌러도 브레이크가 비활성(정지 상태에선 브레이크 안 걸림)
	UPROPERTY(EditAnywhere, Category = "Cart|Brake", meta = (ClampMin = "0"))
	float BrakeStopSpeed = 20.f;

    //전진 중 후진키를 눌렀을 때의 감속도 (낮을수록 천천히)
    UPROPERTY(EditAnywhere, Category = "Cart|Brake", meta = (ClampMin = "0"))
    float ReverseSlideDeceleration = 600.f;

	//---------- 부스터 ----------
	UPROPERTY(EditAnywhere, Category = "Cart|Boost", meta = (ClampMin = "1"))
	float BoostSpeedMultiplier = 1.8f;

	//부스트 지속 시간(초). 아이템화로 사용 기회가 줄어드는 만큼 길게 (B13 밸런싱)
	UPROPERTY(EditAnywhere, Category = "Cart|Boost", meta = (ClampMin = "0"))
	float BoostDuration = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Cart|Boost", meta = (ClampMin = "0"))
	float BoostCooldown = 1.0f;


    //---------- 카메라 속도감 연출 ----------
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


    //---------- 연출 (FX) ----------
    //카트 연출 전담 컴포넌트 — 화면 스피드라인(부스트)·바닥 리본·브레이크 스파크 (에셋·소켓은 BP에서 지정)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cart|FX")
    UCartScreenFXComponent* ScreenFXComponent;

    //---------- 그랩 컴포넌트 ----------
    // 생성자에서 생성하고, SetupPlayerInputComponent에서 IMC 바인딩
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cart|Grab")
    UCartGrabComponent* GrabComponent;

    //---------- 아이템 인벤토리 컴포넌트 ----------
    // 생성자에서 생성, SetupPlayerInputComponent에서 아이템 사용 입력(Shift) 바인딩
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cart|Inventory")
    UCartItemInventoryComponent* ItemInventoryComponent;

	//---------- 적재 (C 상품 시스템 연동) ----------
	//C가 만든 적재 컴포넌트. 생성자에서 부착, BeginPlay에서 적재 변경 이벤트에 바인딩
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cart|Load", meta = (AllowPrivateAccess = "true"))
	UCartLoadComponent* LoadComponent;

	//---------- 적재 무게감 ----------
	//현재 적재율. 0~1 = 가벼움~무거움, 1 초과 = 과적(무게가 MaxWeight를 넘음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cart|Load", meta = (ClampMin = "0", ClampMax = "2"))
	float LoadRatio = 0.f;

	//무거움(적재율 1.0=무게 MaxWeight)일 때 최고 속도 배율 (낮을수록 무거우면 느림). 무거움의 '그나마 움직이는' 바닥 속도
	UPROPERTY(EditAnywhere, Category = "Cart|Load", meta = (ClampMin = "0", ClampMax = "1"))
	float LoadMaxSpeedScale = 0.6f;

	//과적(적재율 1.0 초과) 시 속도 배율 — 담는 양과 무관하게 고정. 무거움(0.6)보다 확 느려 답답하게(과유불급)
	UPROPERTY(EditAnywhere, Category = "Cart|Load", meta = (ClampMin = "0", ClampMax = "1"))
	float OverloadSpeedScale = 0.3f;

	//가득 실었을 때 회전 배율
	UPROPERTY(EditAnywhere, Category = "Cart|Load", meta = (ClampMin = "0", ClampMax = "1"))
	float LoadTurnScale = 0.6f;

	//가득 실었을 때 브레이크 배율 (낮을수록 무거우면 잘 안 멈춤)
	UPROPERTY(EditAnywhere, Category = "Cart|Load", meta = (ClampMin = "0", ClampMax = "1"))
	float LoadBrakeScale = 0.45f;

	//---------- 충돌/스필 드롭 (B4/B5) ----------
	//충격속도(cm/s)를 C 드롭 충격량으로 환산하는 배율 (충돌용)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpImpulseScale = 2.0f;

	//이 접근속도(cm/s) 미만의 약한 접촉은 충돌로 안 침
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float MinBumpSpeed = 600.f;

	//출발 그레이스(초) — 정지에서 움직이기 시작한 지 이 시간 안에 만든 충돌은 무효 (바로 앞 카트 밀기 오판정 방지). 부스트는 예외
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpStartGraceTime = 0.7f;

	//충돌 판정 후 무적 지속(초) — 이 동안 추가 충돌 완전 무시 + 몸통 깜빡 (연타/비비기 방지)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpInvincibleDuration = 1.f;

	//무적 중 몸통 깜빡임 간격(초)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0.01"))
	float BumpBlinkInterval = 0.08f;

	//한 번 쏟은 뒤 다음 드롭까지 최소 간격(초) — 모든 스필(충돌·부스터오용) 공통
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpDropCooldown = 0.5f;

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
	float BumpShakeFullSpeed = 900.f;

	//---------- 충돌 넉백 + 리액션(몸통 들썩·기울임) ----------
	//부스트로 상대 카트를 박았을 때 상대를 밀어내는 세기 (B: 부스트 비비기 방지). LaunchCharacter 속도
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BoostKnockbackStrength = 900.f;

	//일반 충돌(부스트 아님) 시 상대를 밀어내는 세기 = 접근속도 × 이 배율 (살짝만)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float NormalKnockbackScale = 0.4f;

	//일반 충돌 넉백 상한 (접근속도가 커도 이 이상은 안 밀림)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float NormalKnockbackMax = 500.f;

	//리액션 스프링 강성(높을수록 빨리 제자리로). 감쇠와 함께 '덜컹' 리듬 결정
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "1"))
	float BumpReactionStiffness = 220.f;

	//리액션 스프링 감쇠(낮을수록 여러 번 덜컹거림)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpReactionDamping = 14.f;

	//세기 1일 때 기울기 각충격량 — 부딪힌 쪽이 들리는 정도
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpTiltStrength = 220.f;

	//기울기 최대 각도(도) 클램프
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpTiltMaxAngle = 20.f;

	//세기 1일 때 수직 들썩 충격량 (0이면 들썩 없이 기울기만)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpHopStrength = 90.f;

	//수직 들썩 최대 높이(cm) 클램프
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpHopMaxHeight = 8.f;

	//외부 넉백(거대카트·체크아웃존 등)에서 리액션 세기 환산 기준 — 이 넉백 강도면 세기 1.0
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "1"))
	float BumpReactionKnockbackRef = 800.f;

	//---------- 미끄럼 (F 맵 기믹 연동: 물웅덩이 등) ----------
	//미끄럼 중 지면 마찰 (낮을수록 더 미끄러짐)
	UPROPERTY(EditAnywhere, Category = "Cart|Slip", meta = (ClampMin = "0"))
	float SlipGroundFriction = 0.f;

	//미끄럼 중 브레이크 감속도 (0이면 못 멈추고 미끄러짐)
	UPROPERTY(EditAnywhere, Category = "Cart|Slip", meta = (ClampMin = "0"))
	float SlipBrakingDeceleration = 0.f;

	//미끄럼 지속시간 클램프(초) — 기믹이 주는 값이 짧거나 길어도 이 범위로 보정 (조작 불능 시간)
	UPROPERTY(EditAnywhere, Category = "Cart|Slip", meta = (ClampMin = "0"))
	float SlipMinDuration = 1.5f;
	UPROPERTY(EditAnywhere, Category = "Cart|Slip", meta = (ClampMin = "0"))
	float SlipMaxDuration = 2.f;

	//미끄럼 동안 카트 메시가 도는 바퀴 수 — 정수 바퀴라 끝나면 원래 방향으로 복귀
	UPROPERTY(EditAnywhere, Category = "Cart|Slip", meta = (ClampMin = "1"))
	int32 SlipSpinTurns = 2;

	//빙글 돌릴 카트 메시 컴포넌트 이름 (BP_CartPawn의 카트 스태틱 메시)
	UPROPERTY(EditAnywhere, Category = "Cart|Slip")
	FName SlipSpinMeshName = TEXT("Cart");

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

	//미끄럼(물웅덩이) 시작 시 효과음 — 내 카트는 2D, 상대 카트는 3D로 재생 (StartSlip은 전 클라 실행)
	UPROPERTY(EditAnywhere, Category = "Cart|SFX")
	USoundBase* SlipSound;

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

	//몸통 메시(SlipSpinMeshName) 탐색 + 기준 상대회전/위치 캡처 (슬립·범프 리액션 공용, 최초 1회)
	void EnsureBodyMeshResolved();

	//충돌 후 무적 시작 (서버) — 지속시간 동안 추가 충돌 무시, 복제되어 전 클라 깜빡
	void StartBumpInvincibility();

	//C 적재 정보가 바뀔 때 호출되는 델리게이트 핸들러. AddDynamic용
	UFUNCTION()
	void HandleLoadInfoChanged(AActor* OwnerActor, const FLoadInfo& LoadInfo);

private:
	//현재 프레임 입력값
	float ThrottleInput = 0.f;
	float SteerInput = 0.f;
	float CurrentSteer = 0.f; //부드럽게 보간된 조향값

	//브레이크 키 눌림(원입력). 실제 브레이크 상태(bIsBraking)는 이 값 + 이동 중일 때만 true
	bool bBrakeHeld = false;

	//브레이크 상태(실제 감속·연출 적용). 정지 상태에선 키를 눌러도 false. (타 클라 스파크 연출용 복제 — 부스터와 동일 패턴)
	UPROPERTY(Replicated)
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

	//연속 이동 시간(초) — 정지하면 0으로 리셋. 출발 그레이스 판정용(서버)
	float TimeSinceMoveStart = 0.f;

    //카트 회전(yaw) 네트워크 동기화 — 소유자가 갱신, 비소유(타 클라·서버)는 이 값으로 보간해 따라간다
    //RepMovement 회전이 클라 조종 카트에서 느리게/스텝으로 전파되는 문제를 우회. COND_SkipOwner(소유자는 로컬 유지)
    UPROPERTY(Replicated)
    float ReplicatedYaw = 0.f;

    //서버에 마지막으로 보낸 Yaw
    float LastSentYaw = 0.f;

    //소유 클라가 전담하는 절대 회전값(도). 매 프레임 SetActorRotation으로 재확정해 서버 보정 롤백에 면역
    float ControlledYaw = 0.f;

	//미끄럼(슬립) 상태 — 액터 yaw는 건드리지 않고(카메라 정면 유지) 카트 메시만 제자리 스핀
	bool bIsSlipping = false;
	float SlipTimeRemaining = 0.f;     //남은 미끄럼 시간(초)
	float SlipDurationTotal = 0.f;     //이번 미끄럼 총 시간(초) — 스핀 진행도 계산용
	float SlipSpinDir = 1.f;           //스핀 방향(+1/-1) — 기믹이 준 SpinAngleDeg의 부호
	float DefaultGroundFriction = 0.f; //미끄럼 후 마찰 복구용 백업

	//빙글 돌릴 카트 메시 (SlipSpinMeshName으로 최초 1회 탐색·캐시) + 원래 상대 회전/위치(복구용). 범프 리액션도 공유
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> SlipSpinMesh;
	FRotator SlipSpinMeshBaseRelRot = FRotator::ZeroRotator;
	FVector SlipSpinMeshBaseRelLoc = FVector::ZeroVector; //몸통 메시 원래 상대 위치(들썩 복구용)
	bool bBodyMeshResolved = false;                       //몸통 메시 탐색·기준값 캡처 완료

	//---------- 충돌 리액션 스프링 상태 (로컬 연출, 복제 안 함) ----------
	//몸통 메시 상대 pitch/roll(도) 오프셋 + 각 속도, 수직 들썩(cm) 오프셋 + 속도. 스프링으로 0(기준)에 복귀
	float BumpTiltPitch = 0.f;    float BumpTiltPitchVel = 0.f;
	float BumpTiltRoll = 0.f;     float BumpTiltRollVel = 0.f;
	float BumpHopOffsetZ = 0.f;   float BumpHopVel = 0.f;
	bool bBumpReactionActive = false; //스프링이 움직이는 중일 때만 Tick에서 메시 갱신

	//---------- 충돌 후 무적 상태 ----------
	//무적 여부 (복제: 모든 클라가 몸통 깜빡). 서버가 시간 관리
	UPROPERTY(Replicated)
	bool bBumpInvincible = false;
	float BumpInvincibleTimeRemaining = 0.f; //서버 카운트다운
	float BumpBlinkAccum = 0.f;              //로컬 깜빡 타이머
};
