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
class UCartCameraComponent;
class UCartBumpComponent;
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

	//미끄럼(물웅덩이 등) 중인지 — 조작 잠금 상태. 외부 시스템(예: 로봇손)이 발동 스킵 판정에 사용
	UFUNCTION(BlueprintCallable, Category = "Cart")
	bool IsSlipping() const { return bIsSlipping; }

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

    //카메라 연출 컴포넌트(CartCameraComponent)용 접근자
    USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
    UCameraComponent* GetFollowCamera() const { return FollowCamera; }

    //---------- 충돌 컴포넌트(CartBumpComponent) 연동 접근자 ----------
    UCartBumpComponent* GetBumpComponent() const { return BumpComponent; }
    UCartLoadComponent* GetLoadComponent() const { return LoadComponent; }

    //충돌 직전 프레임 속도 (NotifyHit 시점 속도는 깎여서 신뢰 불가 => Tick 캐시값)
    FVector GetPreviousVelocity() const { return PreviousVelocity; }

    //연속 이동 시간(초) — 출발 그레이스 판정용
    float GetTimeSinceMoveStart() const { return TimeSinceMoveStart; }

    //몸통 메시 + 기준 상대회전/위치 (슬립·범프·색상 공용) — 필요 시 1회 탐색
    UStaticMeshComponent* GetBodyMesh() { EnsureBodyMeshResolved(); return SlipSpinMesh; }
    FRotator GetBodyMeshBaseRelRot() const { return SlipSpinMeshBaseRelRot; }
    FVector GetBodyMeshBaseRelLoc() const { return SlipSpinMeshBaseRelLoc; }

    //부스트 강제 종료 (서버) — 충돌·기믹 넉백 시 호출. 소유 클라에도 전파
    void CancelBoost();

    //---------- 충돌 시 정산 취소 상태 반영 ----------
    //정산 취소 상태인지
    bool IsCancelCheckoutState() const;

    //정산 취소 시 Duration 동안 정산 못함
    void CancelCheckout(float Duration = 0.2f);

    void ClearCancelCheckoutState();

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override; //서버: 로비 선택 색을 카트에 적용
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

	//부스트 지속 시간(초)
	UPROPERTY(EditAnywhere, Category = "Cart|Boost", meta = (ClampMin = "0"))
	float BoostDuration = 1.5f;

	//부스트 쿨타임(초)
	UPROPERTY(EditAnywhere, Category = "Cart|Boost", meta = (ClampMin = "0"))
	float BoostCooldown = 5.0f;


    //---------- 카메라 연출 컴포넌트 ----------
    //속도 줌/FOV·카메라 충돌 전담 (튜닝 프로퍼티는 컴포넌트로 이동)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cart|Camera")
    UCartCameraComponent* CameraComponent;

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

	//---------- 충돌 ----------
	//충돌 판정·넉백·리액션·무적 전담 컴포넌트 (판정/튜닝 프로퍼티는 컴포넌트로 이동)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cart|Bump")
	UCartBumpComponent* BumpComponent;

	//충돌 시 재생할 카메라 쉐이크 (BP_CartPawn에 BP_CartBumpShake 지정). 비어있으면 흔들지 않음. ClientPlayCameraShake의 기본값
	UPROPERTY(EditAnywhere, Category = "Cart|Bump")
	TSubclassOf<UCameraShakeBase> BumpCameraShakeClass;

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

	//---------- 효과음 (SFX) — BP_CartPawn에서 사운드 지정. 비어있으면 무음 (충돌음은 CartBumpComponent) ----------
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

	//부스트 시작 핵심 (상태+런치+지속 타이머). 사운드 없음
	void StartBoost();
	void EndBoost();
	void ResetBoostCooldown();

	//부스트 강제 종료를 소유 클라에 적용 (타이머 정리 + 쿨다운 시작)
	UFUNCTION(Client, Reliable)
	void ClientCancelBoost();

	//미끄럼 시작/종료 (로컬 적용) — MulticastApplySlip에서 호출
	void StartSlip(float Duration, float SpinAngleDeg);
	void EndSlip();

	//몸통 메시(SlipSpinMeshName) 탐색 + 기준 상대회전/위치 캡처 (슬립·범프 리액션 공용, 최초 1회)
	void EnsureBodyMeshResolved();

	//게임 시작 전 대기 페이즈 동안 조작 가능 여부 (MainGameState 질의). GameState 없으면 true
	bool CanPlayerMove() const;

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

	//정산 취소 상태인지
	bool bIsCancelCheckoutState = false;
	//정산 취소 시간
	FTimerHandle CancelCheckoutTimerHandle;

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

	//---------- 카트 색상 (로비 캐릭터 선택 연동) ----------
	//서버가 빙의 시 로비 선택(GameInstance)으로 결정 => 전 클라 복제
	UPROPERTY(ReplicatedUsing = OnRep_CartColor)
	FLinearColor CartColor = FLinearColor::White;

	UFUNCTION()
	void OnRep_CartColor();

	//몸통 메시 전 슬롯에 MID를 만들어 CartColor 파라미터 적용
	void ApplyCartColor();
};
