//BumperCart - B(카트/플레이어 조작) 파트

#include "CartPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "TimerManager.h"
#include "BumperCart.h"
#include "Net/UnrealNetwork.h"

ACartPawn::ACartPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	//카트는 조향(A/D)으로 직접 회전하므로 컨트롤러 회전은 사용하지 않는다.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	//이동 컴포넌트 기본 세팅 (수치는 B2에서 본격 튜닝)
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = false; //Yaw는 우리가 직접 제어
		Move->RotationRate = FRotator::ZeroRotator;
		Move->bUseSeparateBrakingFriction = true;
		Move->GroundFriction = 3.0f; //낮을수록 미끄러짐(드리프트)
		Move->BrakingFriction = 1.5f;
		Move->MaxAcceleration = 1200.f;
		Move->MaxWalkSpeed = 900.f;
		Move->BrakingDecelerationWalking = 1400.f;
		Move->JumpZVelocity = 0.f; //카트는 점프 없음
		Move->AirControl = 0.f;
	}

	//카메라: 고정 쿼터뷰. 카트가 회전해도 각도는 고정되고 위치만 따라간다.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1100.f; //쿼터뷰라 멀리서
	CameraBoom->SetRelativeRotation(FRotator(-55.f, 0.f, 0.f)); //아래로 비스듬히 내려봄
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false; //카트 회전과 무관하게 각도 고정
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false; //탑다운이라 벽에 카메라가 당겨지지 않게
	CameraBoom->bEnableCameraLag = true; //위치만 부드럽게 따라감
	CameraBoom->CameraLagSpeed = 8.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	//적재 컴포넌트(C 상품 시스템) 부착. 적재율 연동은 BeginPlay에서 이벤트 바인딩
	LoadComponent = CreateDefaultSubobject<UCartLoadComponent>(TEXT("CartLoadComponent"));
}

void ACartPawn::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		DefaultMaxWalkSpeed = Move->MaxWalkSpeed;
		DefaultBrakingDeceleration = Move->BrakingDecelerationWalking;
	}

	//C의 적재 정보가 바뀔 때마다 적재율 갱신 (서버/클라 각자 자기 인스턴스에서 반영)
	if (LoadComponent)
	{
		LoadComponent->OnLoadInfoChanged.AddDynamic(this, &ACartPawn::HandleLoadInfoChanged);
	}
}

void ACartPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	//적재 무게 반영 배율
	const float LoadSpeedMul = FMath::Lerp(1.f, LoadMaxSpeedScale, LoadRatio);
	const float LoadTurnMul = FMath::Lerp(1.f, LoadTurnScale, LoadRatio);
	const float LoadBrakeMul = FMath::Lerp(1.f, LoadBrakeScale, LoadRatio);

	//--- 최고 속도: 부스터 > 후진 > 기본, 적재 무게 반영 ---
	float TargetMaxSpeed = DefaultMaxWalkSpeed * LoadSpeedMul;
	const float ForwardSpeed = FVector::DotProduct(Move->Velocity, GetActorForwardVector());
	if (ThrottleInput < 0.f && ForwardSpeed < -10.f) //실제로 뒤로 가는 중이면 후진 속도로 제한
	{
		TargetMaxSpeed *= MaxReverseSpeedRatio;
	}
	if (bIsBoosting) //부스터가 최우선
	{
		TargetMaxSpeed = DefaultMaxWalkSpeed * BoostSpeedMultiplier * LoadSpeedMul;
	}
	Move->MaxWalkSpeed = TargetMaxSpeed;

	//--- 브레이크 / 추력 ---
	if (bIsBraking)
	{
		//브레이크 중에는 추력을 넣지 않고 감속도를 크게 해서 급정지시킨다. (무거우면 덜 멈춤)
		Move->BrakingDecelerationWalking = BrakeDeceleration * LoadBrakeMul;
	}
	else
	{
		Move->BrakingDecelerationWalking = DefaultBrakingDeceleration;

		if (!FMath::IsNearlyZero(ThrottleInput))
		{
			//카트가 바라보는 방향으로 전/후진
			AddMovementInput(GetActorForwardVector(), ThrottleInput);
		}
	}

	//--- 조향 (A/D = Yaw 회전). 입력을 부드럽게 따라가 회전 지연감을 준다 ---
	CurrentSteer = FMath::FInterpTo(CurrentSteer, SteerInput, DeltaSeconds, SteerInterpSpeed);
	if (!FMath::IsNearlyZero(CurrentSteer))
	{
		//빠를수록 잘 돌고, 정지 시에는 최소 배율만 적용 (카트 특유의 둔한 조향)
		const float Speed = Move->Velocity.Size2D();
		const float SpeedAlpha = FMath::Clamp(Speed / FMath::Max(DefaultMaxWalkSpeed, 1.f), 0.f, 1.f);
		const float SpeedFactor = FMath::Lerp(MinSteerSpeedFactor, 1.f, SpeedAlpha);

		const float YawDelta = CurrentSteer * TurnRateDegPerSec * LoadTurnMul * SpeedFactor * DeltaSeconds;
		AddActorWorldRotation(FRotator(0.f, YawDelta, 0.f));
	}

	//--- B5: 부스터 오용 = 부스터 중 브레이크/급회전이면 내 물건을 쏟는다 ---
	//입력 기반이라 서버가 모름 => 내 카트(소유 클라)에서 판정, RequestSpill이 서버로 전달
	if (bIsBoosting && IsLocallyControlled())
	{
		//부스터 중 브레이크
		if (bIsBraking)
		{
			RequestSpill(BoostMisuseImpulse, EDropCollisionRole::Normal);
		}

		//부스터 중 누적 회전각이 임계를 넘으면 (살짝 보정은 OK, 확 꺾으면 쏟음)
		BoostTurnAccumDeg += FMath::Abs(CurrentSteer) * TurnRateDegPerSec * DeltaSeconds;
		if (BoostTurnAccumDeg >= BoostTurnSpillAngle)
		{
			RequestSpill(BoostMisuseImpulse, EDropCollisionRole::Normal);
			BoostTurnAccumDeg = 0.f;
		}
	}
}

void ACartPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogBumperCart, Error, TEXT("CartPawn: Enhanced Input Component를 찾지 못했습니다."));
		return;
	}

	if (ThrottleAction)
	{
		EIC->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ACartPawn::OnThrottle);
		EIC->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ACartPawn::OnThrottleReleased);
	}
	if (SteerAction)
	{
		EIC->BindAction(SteerAction, ETriggerEvent::Triggered, this, &ACartPawn::OnSteer);
		EIC->BindAction(SteerAction, ETriggerEvent::Completed, this, &ACartPawn::OnSteerReleased);
	}
	if (BrakeAction)
	{
		EIC->BindAction(BrakeAction, ETriggerEvent::Started, this, &ACartPawn::OnBrakeStart);
		EIC->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ACartPawn::OnBrakeStop);
	}
	if (BoostAction)
	{
		EIC->BindAction(BoostAction, ETriggerEvent::Started, this, &ACartPawn::OnBoost);
	}
}

void ACartPawn::OnThrottle(const FInputActionValue& Value)
{
	ThrottleInput = Value.Get<float>();
}

void ACartPawn::OnThrottleReleased(const FInputActionValue& Value)
{
	ThrottleInput = 0.f;
}

void ACartPawn::OnSteer(const FInputActionValue& Value)
{
	SteerInput = Value.Get<float>();
}

void ACartPawn::OnSteerReleased(const FInputActionValue& Value)
{
	SteerInput = 0.f;
}

void ACartPawn::OnBrakeStart(const FInputActionValue& Value)
{
	bIsBraking = true;
}

void ACartPawn::OnBrakeStop(const FInputActionValue& Value)
{
	bIsBraking = false;
}

void ACartPawn::OnBoost(const FInputActionValue& Value)
{
	if (bIsBoosting || bBoostOnCooldown)
	{
		return;
	}

	bIsBoosting = true;
	bBoostOnCooldown = true;
	BoostTurnAccumDeg = 0.f; //새 부스터마다 누적 회전 리셋
	ServerSetBoosting(true); //서버에도 부스터 상태 통지 (충돌 역할용)

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		//즉발 가속 느낌을 위해 전방으로 런치 (최고 속도 상승은 Tick에서 처리)
		const FVector LaunchVelocity = GetActorForwardVector() * DefaultMaxWalkSpeed * (BoostSpeedMultiplier - 1.f);
		LaunchCharacter(LaunchVelocity, false, false);
	}

	GetWorldTimerManager().SetTimer(BoostTimerHandle, this, &ACartPawn::EndBoost, BoostDuration, false);
}

void ACartPawn::EndBoost()
{
	bIsBoosting = false;
	ServerSetBoosting(false); //부스터 종료 서버에 통지
	GetWorldTimerManager().SetTimer(BoostCooldownTimerHandle, this, &ACartPawn::ResetBoostCooldown, BoostCooldown, false);
}

void ACartPawn::ResetBoostCooldown()
{
	bBoostOnCooldown = false;
}

void ACartPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//소유 클라는 로컬값 유지, 서버/타 클라만 복제 (충돌 역할 판정·연출용)
	DOREPLIFETIME_CONDITION(ACartPawn, bIsBoosting, COND_SkipOwner);
}

//[B5-②] 부스터 상태를 서버에 통지 (클라 입력은 서버가 모르므로 RPC로 전달)
bool ACartPawn::ServerSetBoosting_Validate(bool bNewBoosting)
{
	return true;
}

void ACartPawn::ServerSetBoosting_Implementation(bool bNewBoosting)
{
	bIsBoosting = bNewBoosting;
}

void ACartPawn::SetLoadRatio(float InLoadRatio)
{
	LoadRatio = FMath::Clamp(InLoadRatio, 0.f, 1.f);
}

//적재 변경 델리게이트 핸들러
void ACartPawn::HandleLoadInfoChanged(AActor* OwnerActor, const FLoadInfo& LoadInfo)
{
	//계산한 적재율(현재무게/최대무게)을 그대로 카트에 반영
	if (LoadComponent)
	{
		SetLoadRatio(LoadComponent->GetLoadRatio());
	}
}

//B4: 카트끼리 부딪히면 충격 세기만큼 상품을 쏟는다 (비물리라 속도로 의사 충격량 계산)
void ACartPawn::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	//충돌은 서버가 판정 (NotifyHit은 서버·소유클라 양쪽에서 떠서, 안 막으면 더블 드롭)
	if (!HasAuthority())
	{
		return;
	}

	//카트끼리만: 상대가 다른 카트일 때만 (벽/장애물은 무시)
	ACartPawn* OtherCart = Cast<ACartPawn>(Other);
	if (!OtherCart || OtherCart == this)
	{
		return;
	}

	//충돌 노멀 반대방향으로의 상대 접근 속도 = 정면으로 부딪힐수록 큼
	const FVector RelativeVelocity = GetVelocity() - OtherCart->GetVelocity();
	const float ClosingSpeed = FVector::DotProduct(RelativeVelocity, -HitNormal);
	if (ClosingSpeed < MinBumpSpeed)
	{
		return; //약하게 스치는 접촉은 무시
	}

	//[B5-②] 충돌 역할 판정 (서버 기준 부스터 상태로)
	EDropCollisionRole DropRole = EDropCollisionRole::Normal;
	if (bIsBoosting)
	{
		DropRole = EDropCollisionRole::BoosterInstigator;   //내가 부스터로 박음 => 덜 흘림
	}
	else if (OtherCart->bIsBoosting)
	{
		DropRole = EDropCollisionRole::BoostedTarget;        //부스터한테 박힘 => 더 흘림
	}
	RequestSpill(ClosingSpeed * BumpImpulseScale, DropRole);
}

//스필(드롭) 공통 진입점 — 쿨다운 적용 후 C에 낙하 요청 (충돌·부스터오용 공용)
void ACartPawn::RequestSpill(float Impulse, EDropCollisionRole DropRole)
{
	if (!LoadComponent)
	{
		return;
	}

	//모든 스필 공통 쿨다운
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (Now - LastBumpDropTime < BumpDropCooldown)
	{
		return;
	}
	LastBumpDropTime = Now;

	//[임시] B5-② 역할 확인용 - 검증 후 제거
	UE_LOG(LogBumperCart, Log, TEXT("Spill role=%d impulse=%.0f"), (int32)DropRole, Impulse);

	//C가 개수 판정 + 실제 드롭 (서버=즉시, 클라=서버 RPC)
	LoadComponent->RequestDropProduct(Impulse, DropRole);
}
