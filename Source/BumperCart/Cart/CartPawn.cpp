//BumperCart - B(카트/플레이어 조작) 파트

#include "CartPawn.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "TimerManager.h"
#include "BumperCart.h"
#include "Net/UnrealNetwork.h"
#include "Component/CartGrabComponent.h"
#include "Component/CartScreenFXComponent.h"
#include "Component/CartItemInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ACartPawn::ACartPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	//카트는 조향(A/D)으로 직접 회전하므로 컨트롤러 회전은 사용하지 않는다.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	//이동 컴포넌트 기본 세팅
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = false; //Yaw는 우리가 직접 제어
		Move->RotationRate = FRotator::ZeroRotator;
		Move->bUseSeparateBrakingFriction = true;
		Move->GroundFriction = 3.0f; //낮을수록 미끄러짐(드리프트)
		Move->BrakingFriction = 1.5f;
		Move->MaxAcceleration = BaseMaxAcceleration;
		Move->MaxWalkSpeed = BaseMaxWalkSpeed;
		Move->BrakingDecelerationWalking = 1400.f;
		Move->JumpZVelocity = 0.f; //카트는 점프 없음
		Move->AirControl = 0.f;
	}

	//카메라: 3인칭. 카트 뒤를 따라가며(InheritYaw) 완만하게 내려다본다. 세부 위치는 BP_CartPawn에서 미세조정
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeLocation(FVector(371.72f, 0.f, 34.98f)); //앞으로 당겨 전방 시야 확보
	CameraBoom->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f)); //완만하게 내려봄(3인칭)
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = true; //카트 회전을 따라 뒤에서 쫓아감
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false; //벽에 카메라가 당겨지지 않게
	//카메라 랙(트레일): 위치·회전을 살짝 뒤늦게 따라오게 해 급가감속·회전 시 화면 출렁임/멀미 완화 (낮을수록 더 늘어짐)
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 8.f;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraRotationLagSpeed = 8.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	//적재 컴포넌트(C 상품 시스템) 부착. 적재율 연동은 BeginPlay에서 이벤트 바인딩
	LoadComponent = CreateDefaultSubobject<UCartLoadComponent>(TEXT("CartLoadComponent"));

    //임시 효과음 기본값 (정식 사운드 작업 때 교체)
    static ConstructorHelpers::FObjectFinder<USoundBase> BumpSoundFinder(TEXT("/Game/Developers/dbals/Audio/Bump.Bump"));
    if (BumpSoundFinder.Succeeded()) { BumpSound = BumpSoundFinder.Object; }

    static ConstructorHelpers::FObjectFinder<USoundBase> BoostSoundFinder(TEXT("/Game/Developers/dbals/Audio/Boost2.Boost2"));
    if (BoostSoundFinder.Succeeded()) { BoostSound = BoostSoundFinder.Object; }

    static ConstructorHelpers::FObjectFinder<USoundBase> BrakeSoundFinder(TEXT("/Game/Developers/dbals/Audio/Brake.Brake"));
    if (BrakeSoundFinder.Succeeded()) { BrakeSound = BrakeSoundFinder.Object; }

    // 그랩 컴포넌트 부착, SetupPlayerInputComponent에서 바인딩
    GrabComponent = CreateDefaultSubobject<UCartGrabComponent>(TEXT("CartGrabComponent"));

    // 아이템 인벤토리 컴포넌트 부착, SetupPlayerInputComponent에서 아이템 사용 입력 바인딩
    ItemInventoryComponent = CreateDefaultSubobject<UCartItemInventoryComponent>(TEXT("ItemInventoryComponent"));

    //연출(FX) 전담 컴포넌트 — 화면 스피드라인·바닥 리본·브레이크 스파크. 에셋·소켓은 BP의 이 컴포넌트에서 지정
    ScreenFXComponent = CreateDefaultSubobject<UCartScreenFXComponent>(TEXT("CartScreenFXComponent"));
}

void ACartPawn::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		//BP에서 튜닝한 기본 이속·가속을 이동 컴포넌트에 반영 (생성자 이후 BP 오버라이드까지 최종 반영)
		Move->MaxWalkSpeed = BaseMaxWalkSpeed;
		Move->MaxAcceleration = BaseMaxAcceleration;

		DefaultMaxWalkSpeed = Move->MaxWalkSpeed;
		DefaultBrakingDeceleration = Move->BrakingDecelerationWalking;
		DefaultGroundFriction = Move->GroundFriction;
	}

	//C의 적재 정보가 바뀔 때마다 적재율 갱신 (서버/클라 각자 자기 인스턴스에서 반영)
	if (LoadComponent)
	{
		LoadComponent->OnLoadInfoChanged.AddDynamic(this, &ACartPawn::HandleLoadInfoChanged);
	}

	//회전 동기화 초기값 — 스폰 방향으로 세팅 (첫 복제 전 비소유 인스턴스가 yaw=0으로 튀는 것 방지)
	ReplicatedYaw = GetActorRotation().Yaw;
	LastSentYaw = ReplicatedYaw;
	ControlledYaw = ReplicatedYaw;
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
	//속도: 무거움까지는 무게 비례(1.0→0.6), 과적(적재율 1.0 초과)이면 담는 양 무관하게 고정 배율(0.3)로 뚝 떨어져 답답하게
	//회전·브레이크: 과적 페널티 없음 → 적재율을 1.0로 클램프해 무거움 수준 유지
	const float ClampedLoad = FMath::Min(LoadRatio, 1.f);
	const float LoadSpeedMul = (LoadRatio > 1.f) ? OverloadSpeedScale : FMath::Lerp(1.f, LoadMaxSpeedScale, LoadRatio);
	const float LoadTurnMul = FMath::Lerp(1.f, LoadTurnScale, ClampedLoad);
	const float LoadBrakeMul = FMath::Lerp(1.f, LoadBrakeScale, ClampedLoad);

	//--- 최고 속도: 부스터 > 후진 > 기본, 적재 무게 반영 ---
	float TargetMaxSpeed = DefaultMaxWalkSpeed * LoadSpeedMul;
	const float ForwardSpeed = FVector::DotProduct(Move->Velocity, GetActorForwardVector());
	if (ThrottleInput < 0.f && ForwardSpeed < -10.f) //실제로 뒤로 가는 중이면 후진 속도로 제한
	{
		TargetMaxSpeed *= MaxReverseSpeedRatio;
	}
	if (bIsBoosting) //부스터가 최우선 — 무게 감속 무시(아이템, 굼뜬 카트의 풀스피드 탈출/공격)
	{
		TargetMaxSpeed = DefaultMaxWalkSpeed * BoostSpeedMultiplier;
	}
	Move->MaxWalkSpeed = TargetMaxSpeed;

	//--- 브레이크 상태 갱신: 키를 눌러도 정지 상태(BrakeStopSpeed 이하)면 브레이크 비활성 ---
	//입력(bBrakeHeld)과 실제 상태(bIsBraking)를 분리. 소유자가 판정 후 서버/타 클라에 복제 (부스터와 동일 패턴)
	if (IsLocallyControlled())
	{
		//부스트 중엔 브레이크 무시(부스트는 커밋된 돌진), 미끄럼 중엔 조작 불능 → 감속·스파크 연출도 차단
		const bool bBrakeEffective = bBrakeHeld && !bIsBoosting && !bIsSlipping && Move->Velocity.Size2D() > BrakeStopSpeed;
		if (bBrakeEffective != bIsBraking)
		{
			bIsBraking = bBrakeEffective;
			if (!HasAuthority())
			{
				ServerSetBraking(bIsBraking); //타 클라 스파크 연출용
			}
		}
	}

	//--- 미끄럼(슬립) 진행: 시간 경과 + 카트 메시 제자리 스핀 (액터 yaw 불변 → 카메라 정면 유지) ---
	if (bIsSlipping)
	{
		SlipTimeRemaining -= DeltaSeconds;
		if (SlipTimeRemaining <= 0.f)
		{
			EndSlip();
		}
		else if (SlipSpinMesh)
		{
			//진행도(0→1)에 이즈아웃 — 처음엔 빠르게 돌고 끝엔 감속. 정수 바퀴라 종료 시 원래 방향
			const float Progress = 1.f - (SlipTimeRemaining / FMath::Max(SlipDurationTotal, KINDA_SMALL_NUMBER));
			const float Eased = 1.f - FMath::Square(1.f - Progress);
			const float SpinYaw = SlipSpinDir * SlipSpinTurns * 360.f * Eased;
			SlipSpinMesh->SetRelativeRotation(SlipSpinMeshBaseRelRot + FRotator(0.f, SpinYaw, 0.f));
		}
	}

	//--- 브레이크 / 추력 ---
	if (bIsBraking && !bIsSlipping)
	{
		//브레이크 중에는 추력을 넣지 않고 감속도를 크게 해서 급정지시킨다. (무거우면 덜 멈춤)
		Move->BrakingDecelerationWalking = BrakeDeceleration * LoadBrakeMul;
	}
	else
	{
		float EffectiveThrottle = ThrottleInput;

	    //부스터 중 후진 무시
	    if (bIsBoosting)
	    {
	        EffectiveThrottle = 1.f;
	    }

	    //미끄럼 중 조작 불능 — 추력 차단, 관성으로만 미끄러짐
	    if (bIsSlipping)
	    {
	        EffectiveThrottle = 0.f;
	    }

	    //전진 중 후진 => 약한 감속
 	    const bool bReverseWhileForward = (EffectiveThrottle < 0.f && ForwardSpeed > 10.f);

	    //미끄럼 > 전진중후진(약한 감속, 무게 반영) > 평소 기본값
	    if (bIsSlipping)
	        Move->BrakingDecelerationWalking = SlipBrakingDeceleration;
	    else if (bReverseWhileForward)
	        Move->BrakingDecelerationWalking = ReverseSlideDeceleration * LoadBrakeMul;
	    else
	        Move->BrakingDecelerationWalking = DefaultBrakingDeceleration;

	    if (!bReverseWhileForward && !FMath::IsNearlyZero(EffectiveThrottle))
	    {
	        AddMovementInput(GetActorForwardVector(), EffectiveThrottle);
	    }
	}

	//--- 조향(A/D) yaw 델타 계산 (미끄럼 중엔 조향 불능, 회전은 아래에서 한 번에 적용) ---
	CurrentSteer = FMath::FInterpTo(CurrentSteer, SteerInput, DeltaSeconds, SteerInterpSpeed);
	float YawDeltaTotal = 0.f;
	if (!bIsSlipping && !FMath::IsNearlyZero(CurrentSteer))
	{
		//빠를수록 잘 돌고, 정지 시에는 최소 배율만 적용
		const float Speed = Move->Velocity.Size2D();
		const float SpeedAlpha = FMath::Clamp(Speed / FMath::Max(DefaultMaxWalkSpeed, 1.f), 0.f, 1.f);
		const float SpeedFactor = FMath::Lerp(MinSteerSpeedFactor, 1.f, SpeedAlpha);

		//후진 중(실제로 뒤로 갈 때)에는 조향을 반전 → 플레이어 기준 방향 유지(A=왼쪽)
		const float SteerSign = (ForwardSpeed < -10.f) ? -1.f : 1.f;
		YawDeltaTotal += SteerSign * CurrentSteer * TurnRateDegPerSec * LoadTurnMul * SpeedFactor * DeltaSeconds;
	}

	//--- 회전 적용 + 네트워크 동기화 (회전은 '소유 클라 전담') ---
	if (IsLocallyControlled())
	{
		//소유자: 절대 yaw를 매 프레임 재확정 → 서버 보정 롤백에 면역 (상대 회전은 롤백 시 각도를 까먹어 클라만 느려짐)
		ControlledYaw = FRotator::NormalizeAxis(ControlledYaw + YawDeltaTotal);
		SetActorRotation(FRotator(0.f, ControlledYaw, 0.f));

		if (HasAuthority())
		{
			ReplicatedYaw = ControlledYaw; //리슨 서버 호스트: 바로 복제값 갱신
		}
		else if (!FMath::IsNearlyEqual(ControlledYaw, LastSentYaw, 0.1f))
		{
			LastSentYaw = ControlledYaw;
			ServerSetCartYaw(ControlledYaw); //클라: 서버에 통지 → 서버가 ReplicatedYaw 갱신 → 타 클라 복제
		}
	}
	else
	{
		//비소유(타 클라·서버): 복제 yaw로 부드럽게 보간해 따라간다 (회전은 충돌 판정에 안 쓰여 보간 지연 무해)
		const FRotator Target(0.f, ReplicatedYaw, 0.f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), Target, DeltaSeconds, RemoteYawInterpSpeed));
	}

	//(부스터 오용 스필 제거됨 — 부스터 아이템화 밸런싱: 급브레이크/급회전 패널티 없음)

    //--- 카메라 속도감 연출 ---
    if (IsLocallyControlled() && CameraBoom && FollowCamera)
    {
        //속도 기반 알파 (0=저속, 1=고속)
        const float SpeedAlpha = FMath::Clamp(Move->Velocity.Size2D() / FMath::Max(SpeedZoomFullSpeed, 1.f), 0.f, 1.f);

        //속도로 FOV·암길이 보간
        float TargetArm = FMath::Lerp(CameraArmMin, CameraArmMax, SpeedAlpha);
        float TargetFov = FMath::Lerp(CameraFovMin, CameraFovMax, SpeedAlpha);
        if (bIsBoosting)
        {
            TargetArm += BoostExtraArm;
            TargetFov += BoostExtraFov;
        }

        //튐 방지로 부드럽게 보간
        CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArm, DeltaSeconds, CameraZoomInterpSpeed);
        FollowCamera->FieldOfView   = FMath::FInterpTo(FollowCamera->FieldOfView, TargetFov, DeltaSeconds, CameraZoomInterpSpeed);

    }

	//충돌 세기 계산용: 이번 프레임 속도를 저장 (다음 프레임 NotifyHit에서 '충돌 직전' 속도로 사용)
	PreviousVelocity = GetVelocity();
}

void ACartPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	//카트가 자기 입력매핑(IMC)을 직접 등록 — 어떤 PlayerController가 빙의하든 동작 (PC 의존 제거)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, MappingContextPriority);
			}
		}
	}

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

    // 그랩 전용 IMC, IA 연결
    if (IsValid(GrabComponent))
    {
        GrabComponent->SetupInput();
    }

    // 아이템 사용 입력(Shift) 연결
    if (IsValid(ItemInventoryComponent))
    {
        ItemInventoryComponent->SetupInput();
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
	bBrakeHeld = true; //실제 브레이크 상태는 Tick에서 이동 중일 때만 true로 갱신

	//브레이크 효과음 (소유 클라 로컬) — 실제로 브레이크가 걸릴 때만.
	//정지 상태(BrakeStopSpeed 이하)·부스트 중·미끄럼 중엔 브레이크가 무효라 사운드도 재생 안 함 (Tick의 bBrakeEffective와 동일 조건)
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	if (BrakeSound && Move && !bIsBoosting && !bIsSlipping && Move->Velocity.Size2D() > BrakeStopSpeed)
	{
		UGameplayStatics::PlaySound2D(this, BrakeSound);
	}
}

void ACartPawn::OnBrakeStop(const FInputActionValue& Value)
{
	bBrakeHeld = false;
}

void ACartPawn::OnBoost(const FInputActionValue& Value)
{
	if (bIsBoosting || bBoostOnCooldown || bIsSlipping) //미끄럼 중엔 부스트 발동 불가
	{
		return;
	}

	bIsBoosting = true;
	bBoostOnCooldown = true;
	ServerSetBoosting(true); //서버에도 부스터 상태 통지 (충돌 역할용)

	//부스터 효과음 (소유 클라 로컬)
	if (BoostSound)
	{
		UGameplayStatics::PlaySound2D(this, BoostSound);
	}

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
	DOREPLIFETIME_CONDITION(ACartPawn, bIsBraking, COND_SkipOwner);
	//회전 yaw: 소유자는 로컬 회전을 쓰므로 제외, 비소유(타 클라)만 복제받아 보간
	DOREPLIFETIME_CONDITION(ACartPawn, ReplicatedYaw, COND_SkipOwner);
}

//B5 : 부스터 상태를 서버에 통지 (클라 입력은 서버가 모르므로 RPC로 전달)
bool ACartPawn::ServerSetBoosting_Validate(bool bNewBoosting)
{
	return true;
}

//회전 갱신 — 클라가 보낸 yaw를 복제 프로퍼티에만 반영 (서버 권한 회전은 Tick의 else 분기가 ReplicatedYaw로 세팅)
void ACartPawn::ServerSetCartYaw_Implementation(float Yaw)
{
    ReplicatedYaw = Yaw;
}

void ACartPawn::ServerSetBoosting_Implementation(bool bNewBoosting)
{
	bIsBoosting = bNewBoosting;
}

//브레이크 상태를 서버에 통지 (타 클라 스파크 연출용)
void ACartPawn::ServerSetBraking_Implementation(bool bNewBraking)
{
	bIsBraking = bNewBraking;
}

//[ISlideAffectable] 외부 기믹(물웅덩이 등)이 서버에서 호출 => 모든 인스턴스에 전파
void ACartPawn::ApplySlip_Implementation(float Duration, float SpinAngleDeg)
{
	//서버 기준으로만 시작 => 멀티캐스트로 소유 클라(예측 이동) 포함 전체에 적용
	if (HasAuthority())
	{
		MulticastApplySlip(Duration, SpinAngleDeg);
	}
}

void ACartPawn::MulticastApplySlip_Implementation(float Duration, float SpinAngleDeg)
{
	StartSlip(Duration, SpinAngleDeg);
}

//미끄럼 시작 — 조작을 잠그고 마찰을 낮춰 관성으로만 미끄러뜨린다. 연출은 카트 메시 제자리 스핀(Tick)
void ACartPawn::StartSlip(float Duration, float SpinAngleDeg)
{
	if (bIsSlipping)
	{
		return; //이미 미끄러지는 중이면 무시 (메시 원래 회전 백업 보호)
	}

	bIsSlipping = true;
	SlipDurationTotal = FMath::Clamp(Duration, SlipMinDuration, SlipMaxDuration);
	SlipTimeRemaining = SlipDurationTotal;
	SlipSpinDir = (SpinAngleDeg < 0.f) ? -1.f : 1.f; //기믹이 준 각도는 부호만 방향으로 사용

	//빙글 돌릴 카트 메시 탐색(최초 1회 캐시) — BP_CartPawn의 카트 스태틱 메시
	if (!SlipSpinMesh)
	{
		TArray<UStaticMeshComponent*> MeshComps;
		GetComponents(MeshComps);
		for (UStaticMeshComponent* MeshComp : MeshComps)
		{
			if (MeshComp && MeshComp->GetFName() == SlipSpinMeshName)
			{
				SlipSpinMesh = MeshComp;
				break;
			}
		}
		if (!SlipSpinMesh)
		{
			UE_LOG(LogBumperCart, Warning, TEXT("CartSlip: 카트 메시(%s)를 찾지 못해 스핀 연출 없이 조작 잠금만 적용합니다."), *SlipSpinMeshName.ToString());
		}
	}

	//원래 상대 회전 백업 (종료 시 복구)
	if (SlipSpinMesh)
	{
		SlipSpinMeshBaseRelRot = SlipSpinMesh->GetRelativeRotation();
	}

	//미끄럼 효과음 — 내 카트는 2D(귀에 크게), 상대 카트는 위치 기반 3D (StartSlip은 멀티캐스트로 전 클라 실행)
	if (SlipSound)
	{
		if (IsLocallyControlled())
		{
			UGameplayStatics::PlaySound2D(this, SlipSound);
		}
		else
		{
			UGameplayStatics::PlaySoundAtLocation(this, SlipSound, GetActorLocation());
		}
	}

	//지면 마찰만 시작 시 한 번 낮춘다 (Tick에서 GroundFriction은 안 건드리므로)
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->GroundFriction = SlipGroundFriction;
	}   
}

//미끄럼 종료 — 마찰·메시 회전 원복
void ACartPawn::EndSlip()
{
	bIsSlipping = false;
	SlipTimeRemaining = 0.f;

	if (SlipSpinMesh)
	{
		SlipSpinMesh->SetRelativeRotation(SlipSpinMeshBaseRelRot);
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->GroundFriction = DefaultGroundFriction;
	}
}

void ACartPawn::SetLoadRatio(float InLoadRatio)
{
	//상한을 1이 아니라 2로 — 1 초과(과적)를 살려서 속도 페널티에 사용 (회전·브레이크는 Tick에서 1로 클램프)
	LoadRatio = FMath::Clamp(InLoadRatio, 0.f, 2.f);
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

//카트끼리 부딪히면 충격 세기만큼 상품을 쏟는다
//카트가 IBumpable 대상(다른 카트·차단벽·장애물)에 부딪히면 충격 세기만큼 상품을 쏟는다
void ACartPawn::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	//충돌은 서버가 판정 (NotifyHit은 서버·소유클라 양쪽에서 떠서, 안 막으면 더블 드롭)
	if (!HasAuthority())
	{
		return;
	}

	if (!Other || Other == this)
	{
		return;
	}

	//IBumpable(퍼블릭 상속/BP 인터페이스 추가)을 구현한 대상에만 충돌 연출+드롭. 그 외(일반 벽 등)는 그냥 막힘
	if (!Other->GetClass()->ImplementsInterface(UBumpable::StaticClass()))
	{
		return;
	}

	//상대가 카트면 그 카트의 '충돌 직전' 속도를, 아니면(차단벽·장애물 등 정적) 0으로
	ACartPawn* OtherCart = Cast<ACartPawn>(Other);
	const FVector OtherVel = OtherCart ? OtherCart->PreviousVelocity : FVector::ZeroVector;
	const FVector RelativeVelocity = PreviousVelocity - OtherVel;

	//두 액터 중심을 잇는 선 방향(수평면) — 접근 중(충돌)인지 판정에만 사용
	const FVector ToOther = (Other->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const float Approach = FVector::DotProduct(RelativeVelocity, ToOther);
	if (Approach <= 0.f)
	{
		return; //서로 멀어지는 중이면 충돌로 치지 않음
	}

	//충돌 세기 = 상대속도 크기(2D). 부딪힌 각도와 무관하게 일관됨
	const float ClosingSpeed = RelativeVelocity.Size2D();
	if (ClosingSpeed < MinBumpSpeed)
	{
		return; //약하게 스치는 접촉은 무시
	}

	//충돌 연출: 이 카트를 조종하는 클라 화면만 흔든다 (서버 → 소유 클라 Client RPC)
	if (BumpCameraShakeClass)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			const float ShakeScale = FMath::GetMappedRangeValueClamped(
				FVector2D(MinBumpSpeed, BumpShakeFullSpeed),
				FVector2D(BumpShakeScale, BumpShakeMaxScale),
				ClosingSpeed);
			PC->ClientStartCameraShake(BumpCameraShakeClass, ShakeScale);
		}
	}

	//충돌음: 소유 클라에서 재생
	ClientPlayBumpSound();

	//드롭 역할 판정 — 카트끼리만 부스터 역할, 벽/장애물은 Normal
	EDropCollisionRole DropRole = EDropCollisionRole::Normal;
	if (bIsBoosting)
	{
		DropRole = EDropCollisionRole::BoosterInstigator; //내가 부스터로 박음 => 덜 흘림
	}
	else if (OtherCart && OtherCart->bIsBoosting)
	{
		DropRole = EDropCollisionRole::BoostedTarget; //부스터한테 박힘 => 더 흘림
	}

	RequestSpill(ClosingSpeed * BumpImpulseScale, DropRole);
}

//충돌음을 소유 클라에서 재생 (BumpSound 비어있으면 무음)
void ACartPawn::ClientPlayBumpSound_Implementation()
{
	if (BumpSound)
	{
		UGameplayStatics::PlaySound2D(this, BumpSound);
	}
}

//토마토 피격 시 서버가 소유 클라에 호출 → 화면 가림 위젯 표시
void ACartPawn::ClientApplyTomatoScreenBlock_Implementation(float Duration)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	if (!IsValid(ScreenFXComponent))
	{
		return;
	}

	ScreenFXComponent->ApplyTomatoScreenBlock(Duration);
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

	//B5확인용 - 검증 후 제거
	UE_LOG(LogBumperCart, Log, TEXT("Spill role=%d impulse=%.0f"), (int32)DropRole, Impulse);

	//C가 개수 판정 + 실제 드롭 (서버=즉시, 클라=서버 RPC)
	LoadComponent->RequestDropProduct(Impulse, DropRole);
}

//외부에서 카트를 강제로 밀어내기
void ACartPawn::ApplyExternalKnockback(const FVector& Direction, float Strength)
{
    if (!HasAuthority())
    {
        return;
    }

    if (Strength <= 0.0f)
    {
        return;
    }

    const FVector KnockbackDirection = Direction.GetSafeNormal2D();
    if (KnockbackDirection.IsNearlyZero())
    {
        return;
    }

    const FVector LaunchVelocity = KnockbackDirection * Strength;
    LaunchCharacter(LaunchVelocity, true, false);
}
