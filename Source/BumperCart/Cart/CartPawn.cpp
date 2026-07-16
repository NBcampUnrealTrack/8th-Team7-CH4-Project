//BumperCart - B(카트/플레이어 조작) 파트

#include "CartPawn.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "TimerManager.h"
#include "BumperCart.h"
#include "GameState/MainGameState.h"
#include "GameInstance/MainGameInstance.h"
#include "DataAsset/CharacterSelectionConfig.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Component/CartGrabComponent.h"
#include "Component/CartScreenFXComponent.h"
#include "Component/CartItemInventoryComponent.h"
#include "Component/CartCameraComponent.h"
#include "Component/CartBumpComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "DrawDebugHelpers.h" //임시 디버그 - 필살기 게이지 출력 제거 시 함께 삭제

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
	CameraBoom->bDoCollisionTest = false; //기본 충돌은 즉시 스냅이라 시점이 튐 => Tick에서 커스텀 스무딩 충돌로 대체
	//카메라 랙(트레일): 위치/회전을 살짝 뒤늦게 따라오게 해 급가감속·회전 시 화면 출렁임/멀미 완화 (낮을수록 더 늘어짐)
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 8.f;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraRotationLagSpeed = 8.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	//적재 컴포넌트(C 상품 시스템) 부착. 적재율 연동은 BeginPlay에서 이벤트 바인딩
	LoadComponent = CreateDefaultSubobject<UCartLoadComponent>(TEXT("CartLoadComponent"));

    //임시 효과음 기본값 (정식 사운드 작업 때 교체. 충돌음은 CartBumpComponent)
    static ConstructorHelpers::FObjectFinder<USoundBase> BoostSoundFinder(TEXT("/Game/Developers/dbals/Audio/Boost2.Boost2"));
    if (BoostSoundFinder.Succeeded()) { BoostSound = BoostSoundFinder.Object; }

    static ConstructorHelpers::FObjectFinder<USoundBase> BrakeSoundFinder(TEXT("/Game/Developers/dbals/Audio/Brake.Brake"));
    if (BrakeSoundFinder.Succeeded()) { BrakeSound = BrakeSoundFinder.Object; }

    // 그랩 컴포넌트 부착, SetupPlayerInputComponent에서 바인딩
    // GrabComponent = CreateDefaultSubobject<UCartGrabComponent>(TEXT("CartGrabComponent"));

    // 아이템 인벤토리 컴포넌트 부착, SetupPlayerInputComponent에서 아이템 사용 입력 바인딩
    ItemInventoryComponent = CreateDefaultSubobject<UCartItemInventoryComponent>(TEXT("ItemInventoryComponent"));

    //연출(FX) 전담 컴포넌트 — 화면 스피드라인·바닥 리본·브레이크 스파크. 에셋·소켓은 BP의 이 컴포넌트에서 지정
    ScreenFXComponent = CreateDefaultSubobject<UCartScreenFXComponent>(TEXT("CartScreenFXComponent"));

    //카메라 연출 컴포넌트 — 속도 줌/FOV·카메라 충돌 (튜닝은 컴포넌트 프로퍼티)
    CameraComponent = CreateDefaultSubobject<UCartCameraComponent>(TEXT("CartCameraComponent"));

    //충돌 컴포넌트 — 판정·넉백·리액션·무적 전담 (NotifyHit에서 위임)
    BumpComponent = CreateDefaultSubobject<UCartBumpComponent>(TEXT("CartBumpComponent"));
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

	//몸통 메시 기준 회전/위치 1회 캡처 (FX가 건드리기 전에)
	EnsureBodyMeshResolved();

	//필살기 크기 원복용 원본 캐시 (캡슐 + 몸통 메시)
	if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		DefaultCapsuleRadius = Cap->GetUnscaledCapsuleRadius();
		DefaultCapsuleHalfHeight = Cap->GetUnscaledCapsuleHalfHeight();
	}
	if (SlipSpinMesh)
	{
		DefaultBodyMeshScale = SlipSpinMesh->GetRelativeScale3D();
	}
}

//서버: 로비에서 확정한 캐릭터 색을 카트에 적용 (GameInstance 조회 => CartColor 복제)
void ACartPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	const APlayerState* PS = GetPlayerState();
	const UMainGameInstance* GI = GetGameInstance<UMainGameInstance>();
	if (!PS || !GI || !GI->CharacterSelectionConfig)
	{
		return;
	}

	const int32 CharacterIndex = GI->GetPlayerCharacter(PS->GetUniqueId());
	if (CharacterIndex == INDEX_NONE)
	{
		return; //로비 선택 기록 없음(테스트 직행 등) => 기본 머티리얼 색 유지
	}

	CartColor = GI->CharacterSelectionConfig->GetColor(CharacterIndex);
	ApplyCartColor(); //리슨 호스트 본인 화면 (타 클라는 OnRep)
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
	//속도: 무거움까지는 무게 비례(1.0=>0.6), 과적(적재율 1.0 초과)이면 담는 양 무관하게 고정 배율(0.3)로 뚝 떨어져 답답하게
	//회전·브레이크: 과적 페널티 없음 => 적재율을 1.0로 클램프해 무거움 수준 유지
	const float ClampedLoad = FMath::Min(LoadRatio, 1.f);
	//지수 커브(<1) => 초반 몇 개부터 감속 체감, 뒤로 갈수록 완만 (갯수당 속도 차이는 전 구간 유지)
	const float LoadSpeedAlpha = FMath::Pow(ClampedLoad, LoadSpeedCurveExp);
	const float LoadSpeedMul = (LoadRatio > 1.f) ? OverloadSpeedScale : FMath::Lerp(1.f, LoadMaxSpeedScale, LoadSpeedAlpha);
	const float LoadTurnMul = FMath::Lerp(1.f, LoadTurnScale, ClampedLoad);
	const float LoadBrakeMul = FMath::Lerp(1.f, LoadBrakeScale, ClampedLoad);

	//--- 최고 속도: 부스터 > 후진 > 기본, 적재 무게 반영 ---
	float TargetMaxSpeed = DefaultMaxWalkSpeed * LoadSpeedMul;
	const float ForwardSpeed = FVector::DotProduct(Move->Velocity, GetActorForwardVector());
	if (ThrottleInput < 0.f && ForwardSpeed < -10.f) //실제로 뒤로 가는 중이면 후진 속도로 제한
	{
		TargetMaxSpeed *= MaxReverseSpeedRatio;
	}
	if (bUltimateActive) //필살기: 적재 무게 무시, 일반보다 살짝 빠르게 (부스터가 있으면 아래서 덮어씀)
	{
		TargetMaxSpeed = DefaultMaxWalkSpeed * UltimateSpeedMultiplier;
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
		//브레이크로 부스트 취소 — 부스트 중 브레이크 누르면 부스트를 끊고 감속으로 전환 (즉시 정지 X: 감속도가 낮아 끌려가다 멈춤)
		if (bBrakeHeld && bIsBoosting)
		{
			EndBoost();
		}

		//정지 상태(BrakeStopSpeed 이하)·미끄럼 중·후진 중(ForwardSpeed<=0)엔 브레이크 무효
		const bool bBrakeEffective = bBrakeHeld && !bIsSlipping && ForwardSpeed > 0.f && Move->Velocity.Size2D() > BrakeStopSpeed;
		if (bBrakeEffective != bIsBraking)
		{
			bIsBraking = bBrakeEffective;
			if (!HasAuthority())
			{
				ServerSetBraking(bIsBraking); //타 클라 스파크 연출용
			}
		}
	}

	//--- 미끄럼(슬립) 진행: 시간 경과 + 카트 메시 제자리 스핀 (액터 yaw 불변 => 카메라 정면 유지) ---
	if (bIsSlipping)
	{
		SlipTimeRemaining -= DeltaSeconds;
		if (SlipTimeRemaining <= 0.f)
		{
			EndSlip();
		}
		else if (SlipSpinMesh)
		{
			//진행도(0=>1)에 이즈아웃 — 처음엔 빠르게 돌고 끝엔 감속. 정수 바퀴라 종료 시 원래 방향
			const float Progress = 1.f - (SlipTimeRemaining / FMath::Max(SlipDurationTotal, KINDA_SMALL_NUMBER));
			const float Eased = 1.f - FMath::Square(1.f - Progress);
			const float SpinYaw = SlipSpinDir * SlipSpinTurns * 360.f * Eased;
			SlipSpinMesh->SetRelativeRotation(SlipSpinMeshBaseRelRot + FRotator(0.f, SpinYaw, 0.f));
		}
	}

	//필살기 거대화/복귀 크기 트윈 — 게임플레이 상태는 즉시, 시각·판정 크기만 (오버슈트/단계 선택은 CVar)
	DriveUltimateScale(DeltaSeconds);

	//(충돌 리액션 스프링·무적 깜빡 => CartBumpComponent로 분리)

	//게임 시작 전(대기 페이즈) 동안엔 조작 잠금 (스로틀·조향·부스트 차단)
	const bool bCanMove = CanPlayerMove();

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

	    //게임 시작 전 대기 중엔 이동 불가
	    if (!bCanMove)
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
	if (!bIsSlipping && bCanMove && !FMath::IsNearlyZero(CurrentSteer))
	{
		//빠를수록 잘 돌고, 정지 시에는 최소 배율만 적용
		const float Speed = Move->Velocity.Size2D();
		const float SpeedAlpha = FMath::Clamp(Speed / FMath::Max(DefaultMaxWalkSpeed, 1.f), 0.f, 1.f);
		const float SpeedFactor = FMath::Lerp(MinSteerSpeedFactor, 1.f, SpeedAlpha);

		//후진 중(실제로 뒤로 갈 때)에는 조향을 반전 => 플레이어 기준 방향 유지(A=왼쪽)
		const float SteerSign = (ForwardSpeed < -10.f) ? -1.f : 1.f;
		YawDeltaTotal += SteerSign * CurrentSteer * TurnRateDegPerSec * LoadTurnMul * SpeedFactor * DeltaSeconds;
	}

	//--- 회전 적용 + 네트워크 동기화 (회전은 '소유 클라 전담') ---
	if (IsLocallyControlled())
	{
		//소유자: 절대 yaw를 매 프레임 재확정 => 서버 보정 롤백에 면역 (상대 회전은 롤백 시 각도를 까먹어 클라만 느려짐)
		ControlledYaw = FRotator::NormalizeAxis(ControlledYaw + YawDeltaTotal);
		SetActorRotation(FRotator(0.f, ControlledYaw, 0.f));

		if (HasAuthority())
		{
			ReplicatedYaw = ControlledYaw; //리슨 서버 호스트: 바로 복제값 갱신
		}
		else if (!FMath::IsNearlyEqual(ControlledYaw, LastSentYaw, 0.1f))
		{
			LastSentYaw = ControlledYaw;
			ServerSetCartYaw(ControlledYaw); //클라: 서버에 통지 => 서버가 ReplicatedYaw 갱신 => 타 클라 복제
		}
	}
	else
	{
		//비소유(타 클라·서버): 복제 yaw로 부드럽게 보간해 따라간다 (회전은 충돌 판정에 안 쓰여 보간 지연 무해)
		const FRotator Target(0.f, ReplicatedYaw, 0.f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), Target, DeltaSeconds, RemoteYawInterpSpeed));
	}

	//(부스터 오용 스필 제거됨 — 부스터 아이템화 밸런싱: 급브레이크/급회전 패널티 없음)
	//(카메라 속도감 연출/충돌 => CartCameraComponent로 분리)

	//연속 이동 시간 추적 (출발 그레이스 판정용) — 거의 정지면 리셋
	if (Move->Velocity.Size2D() > 50.f)
	{
		TimeSinceMoveStart += DeltaSeconds;
	}
	else
	{
		TimeSinceMoveStart = 0.f;
	}

	//충돌 세기 계산용: 이번 프레임 속도를 저장 (다음 프레임 NotifyHit에서 '충돌 직전' 속도로 사용)
	PreviousVelocity = GetVelocity();

	//임시 디버그 — 각 카트 머리 위에 게이지 표시. 테스트 빌드에서만 (UI 정식 반영 후 제거: 이 블록 + include + UltimateStack 전체복제 되돌리기)
#if !UE_BUILD_SHIPPING
	{
		const FColor Col = bUltimateActive ? FColor::Yellow : (GetUltimateStack() >= UltimateRequiredStack ? FColor::Green : FColor::White);
		DrawDebugString(GetWorld(), FVector(0.f, 0.f, 140.f),
			FString::Printf(TEXT("ULT %d/%d%s"), GetUltimateStack(), UltimateRequiredStack, bUltimateActive ? TEXT(" ACT") : TEXT("")),
			this, Col, 0.f, true);
	}
#endif
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
	if (UltimateAction)
	{
		EIC->BindAction(UltimateAction, ETriggerEvent::Started, this, &ACartPawn::OnUltimate);
	}

    //// 그랩 전용 IMC, IA 연결
    //if (IsValid(GrabComponent))
    //{
    //    GrabComponent->SetupInput();
    //}

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
	if (BrakeSound && Move && !bIsSlipping && Move->Velocity.Size2D() > BrakeStopSpeed)
	{
		//후진 중(ForwardSpeed<=0)엔 브레이크 무효라 사운드도 재생 안 함
		const float FwdSpeed = FVector::DotProduct(Move->Velocity, GetActorForwardVector());
		if (FwdSpeed > 0.f)
		{
			UGameplayStatics::PlaySound2D(this, BrakeSound);
		}
	}
}

void ACartPawn::OnBrakeStop(const FInputActionValue& Value)
{
	bBrakeHeld = false;
}

//부스트 발동 (Shift 입력, 소유 클라) — 예측 시작 + 사운드 + 서버 통지
void ACartPawn::OnBoost(const FInputActionValue& Value)
{
	if (bIsBoosting || bBoostOnCooldown || bIsSlipping || bUltimateActive || !CanPlayerMove()) //쿨타임·미끄럼·필살기·게임 시작 전 대기 중엔 발동 불가
	{
		return;
	}

	StartBoost();

	//부스터 효과음 (소유 클라 로컬)
	if (BoostSound)
	{
		UGameplayStatics::PlaySound2D(this, BoostSound);
	}

	ServerSetBoosting(true); //서버에 부스터 상태 통지 (충돌 역할용)
}

//부스트 시작 핵심 — 상태 + 전방 런치 + 지속 타이머 (사운드 없음)
void ACartPawn::StartBoost()
{
	bIsBoosting = true;

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
	GetWorldTimerManager().ClearTimer(BoostTimerHandle); //조기 종료(브레이크·충돌) 시 남은 지속 타이머 정리
	bIsBoosting = false;
	bBoostOnCooldown = true;
	ServerSetBoosting(false); //부스터 종료 서버에 통지
	GetWorldTimerManager().SetTimer(BoostCooldownTimerHandle, this, &ACartPawn::ResetBoostCooldown, BoostCooldown, false);
}

void ACartPawn::ResetBoostCooldown()
{
	bBoostOnCooldown = false;
}

//부스터 남은 쿨다운(초) — 소유 클라 타이머 기준
float ACartPawn::GetBoostCooldownRemaining() const
{
	if (!bBoostOnCooldown)
	{
		return 0.f;
	}
	//GetTimerRemaining은 무효 핸들에 -1 => 0 클램프
	return FMath::Max(GetWorldTimerManager().GetTimerRemaining(BoostCooldownTimerHandle), 0.f);
}

//쿨다운 진행률 0(방금 사용)=>1(사용 가능). 부스트 지속 중엔 0 (끝나야 쿨다운 시작)
float ACartPawn::GetBoostCooldownProgress() const
{
	if (bIsBoosting)
	{
		return 0.f;
	}
	if (!bBoostOnCooldown)
	{
		return 1.f;
	}
	return 1.f - FMath::Clamp(GetBoostCooldownRemaining() / FMath::Max(BoostCooldown, KINDA_SMALL_NUMBER), 0.f, 1.f);
}

//부스트 강제 종료 (서버) — 충돌·기믹 넉백 시 호출. 부스트 중이 아니면 무동작
void ACartPawn::CancelBoost()
{
	if (!HasAuthority() || !bIsBoosting)
	{
		return;
	}

	if (IsLocallyControlled())
	{
		EndBoost(); //리슨 호스트 본인
		return;
	}

	bIsBoosting = false;      //서버 즉시 반영 (타 클라 복제)
	ClientCancelBoost();      //소유 클라: 타이머 정리 + 쿨다운 시작
}

//부스트 강제 종료를 소유 클라에 적용
void ACartPawn::ClientCancelBoost_Implementation()
{
	if (bIsBoosting)
	{
		EndBoost();
	}
}

//필살기 발동 (Q 입력, 소유 클라) — 게이지 확인은 서버 권한이므로 서버에 요청만
void ACartPawn::OnUltimate(const FInputActionValue& Value)
{
	if (bUltimateActive || bIsSlipping || !CanPlayerMove())
	{
		return;
	}
	ServerStartUltimate();
}

//필살기 발동 처리 (서버) — 게이지 충전 확인 후 상태·크기 세팅 + 지속 타이머
void ACartPawn::ServerStartUltimate_Implementation()
{
	if (bUltimateActive || bIsSlipping || !CanPlayerMove())
	{
		return;
	}
	if (!BumpComponent || BumpComponent->GetUltimateStack() < UltimateRequiredStack)
	{
		return; //게이지 부족
	}

	bUltimateActive = true;
	BumpComponent->ResetUltimateStack(); //발동하면 게이지 소진
	SetUltimateVisual(true);             //목표 크기·사운드 (서버 로컬, 타 클라는 OnRep). 크기 트윈은 Tick

	GetWorldTimerManager().SetTimer(UltimateTimerHandle, this, &ACartPawn::EndUltimate, UltimateDuration, false);
}

//현재 게이지 스택 (UI용) — 게이지는 BumpComponent가 보유
int32 ACartPawn::GetUltimateStack() const
{
	return BumpComponent ? BumpComponent->GetUltimateStack() : 0;
}

//발동 가능 여부 (충전 완료 && 미발동)
bool ACartPawn::IsUltimateReady() const
{
	return !bUltimateActive && GetUltimateStack() >= UltimateRequiredStack;
}

//필살기 종료 (서버 타이머) — 상태·크기 원복. 게이지는 여기서 다시 모으기 시작
void ACartPawn::EndUltimate()
{
	bUltimateActive = false;
	SetUltimateVisual(false); //목표 크기·사운드 (서버 로컬, 타 클라는 OnRep). 크기 트윈은 Tick
}

//필살기 상태 복제 => 각 클라가 목표 크기·사운드 세팅 (실제 크기 변화는 Tick 보간)
void ACartPawn::OnRep_UltimateActive()
{
	SetUltimateVisual(bUltimateActive);
}

//발동/종료 시 목표 크기 지정 + 사운드. 실제 크기 변화는 Tick의 보간이 담당 (서버 로컬 + OnRep 공용 경로)
void ACartPawn::SetUltimateVisual(bool bOn)
{
	UltimateTargetScale = bOn ? UltimateScale : 1.f;

	//발동/종료음 — 내 카트는 2D(귀에 크게), 상대 카트는 위치 기반 3D
	USoundBase* Sfx = bOn ? UltimateStartSound : UltimateEndSound;
	if (Sfx)
	{
		if (IsLocallyControlled())
		{
			UGameplayStatics::PlaySound2D(this, Sfx);
		}
		else
		{
			UGameplayStatics::PlaySoundAtLocation(this, Sfx, GetActorLocation());
		}
	}
}

//몸통 메시 + 캡슐(판정)을 지정 배율 S로 세팅. 캡슐 높이 변화만큼 발 위치를 유지하도록 액터 Z 보정 => 부양/파묻힘 방지
//Tick이 매 프레임 보간된 S로 호출 => 커지고/작아지는 연출. DeltaHalf가 현재 높이 기준이라 트윈에도 누적이 맞물림
void ACartPawn::ApplyUltimateScale(float S)
{
	CurrentUltimateScale = S; //GetBodyMeshBaseRelLoc이 이 값을 써서 오프셋도 같은 배율 (바퀴 부양 방지)

	//시각 — 몸통 메시 (전 클라). 오프셋도 배율 => 바퀴가 새 캡슐 바닥에 붙음 (부양 방지)
	if (UStaticMeshComponent* Body = GetBodyMesh())
	{
		Body->SetRelativeScale3D(DefaultBodyMeshScale * S);
		Body->SetRelativeLocation(GetBodyMeshBaseRelLoc());
	}

	//판정 — 캡슐 반경·높이 (전 클라, 로컬 물리 일치). 높이 변화분만큼 발 위치 유지
	UCapsuleComponent* Cap = GetCapsuleComponent();
	if (!Cap)
	{
		return;
	}
	const float NewHalfHeight = DefaultCapsuleHalfHeight * S;
	const float DeltaHalf = NewHalfHeight - Cap->GetUnscaledCapsuleHalfHeight(); //커지면 +, 작아지면 -
	Cap->SetCapsuleSize(DefaultCapsuleRadius * S, NewHalfHeight);

	//발 위치(중심-높이) 유지 — 중심을 높이 증가분만큼 이동. 위치는 서버 권한만(RepMovement로 클라 복제)
	if (HasAuthority() && !FMath::IsNearlyZero(DeltaHalf))
	{
		AddActorWorldOffset(FVector(0.f, 0.f, DeltaHalf), false, nullptr, ETeleportType::TeleportPhysics);
	}
}

//거대화/복귀 크기 트윈 구동 (Tick, 전 클라) — 감쇠 스프링, 목표를 살짝 넘었다 되돌아오는 오버슈트 팝
void ACartPawn::DriveUltimateScale(float DeltaTime)
{
	//크기+속도 모두 잦아들면 정확히 스냅 후 정지 (오버슈트가 목표를 지나는 순간 조기 종료 방지 위해 속도도 확인)
	if (FMath::IsNearlyEqual(CurrentUltimateScale, UltimateTargetScale, 0.002f) && FMath::Abs(UltimateScaleVel) < 0.01f)
	{
		if (!FMath::IsNearlyEqual(CurrentUltimateScale, UltimateTargetScale, KINDA_SMALL_NUMBER))
		{
			UltimateScaleVel = 0.f;
			ApplyUltimateScale(UltimateTargetScale); //잔여 오차 스냅
		}
		return;
	}

	//감쇠 스프링 1스텝: a = -k(x-target) - c·v. 감쇠비<1이면 목표를 넘어 오버슈트
	//커질 때/작아질 때 진동수를 다르게 — 성장은 천천히 보이고, 복귀는 빠르게
	const float Freq = (UltimateTargetScale > 1.f) ? UltimateGrowSpeed : UltimateShrinkSpeed;
	const float K = Freq * Freq;                              //강성
	const float C = 2.f * UltimateOvershootRatio * Freq;      //감쇠 (ratio<1 => 오버슈트)
	const float Accel = -K * (CurrentUltimateScale - UltimateTargetScale) - C * UltimateScaleVel;
	UltimateScaleVel += Accel * DeltaTime;

	//프레임 히치 시 스프링 발산 방지 — 원본~오버슈트 여유 범위로 클램프
	const float NextScale = FMath::Clamp(CurrentUltimateScale + UltimateScaleVel * DeltaTime, 1.f, UltimateScale * 1.4f);
	ApplyUltimateScale(NextScale);
}

bool ACartPawn::IsCancelCheckoutState() const
{
	return bIsCancelCheckoutState;
}

//정산 취소 시 Duration 동안 정산 못함
void ACartPawn::CancelCheckout(float Duration)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsCancelCheckoutState = true;

	GetWorldTimerManager().ClearTimer(CancelCheckoutTimerHandle);
	GetWorldTimerManager().SetTimer(
		CancelCheckoutTimerHandle,
		this,
		&ACartPawn::ClearCancelCheckoutState,
		Duration,
		false
	);
}

void ACartPawn::ClearCancelCheckoutState()
{
	bIsCancelCheckoutState = false;
}

void ACartPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//소유 클라는 로컬값 유지, 서버/타 클라만 복제 (충돌 역할 판정·연출용)
	DOREPLIFETIME_CONDITION(ACartPawn, bIsBoosting, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ACartPawn, bIsBraking, COND_SkipOwner);
	//회전 yaw: 소유자는 로컬 회전을 쓰므로 제외, 비소유(타 클라)만 복제받아 보간
	DOREPLIFETIME_CONDITION(ACartPawn, ReplicatedYaw, COND_SkipOwner);
	//카트 색상: 모든 클라가 몸통 색을 그려야 하므로 전체 복제 (충돌 무적은 CartBumpComponent가 복제)
	DOREPLIFETIME(ACartPawn, CartColor);
	//필살기 발동: 모든 클라가 거대화·무적 연출을 해야 하므로 전체 복제
	DOREPLIFETIME(ACartPawn, bUltimateActive);
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

	//몸통 메시 탐색 (base는 BeginPlay에서 이미 캡처됨)
	EnsureBodyMeshResolved();
	if (!SlipSpinMesh)
	{
		UE_LOG(LogBumperCart, Warning, TEXT("CartSlip: 카트 메시(%s)를 찾지 못해 스핀 연출 없이 조작 잠금만 적용합니다."), *SlipSpinMeshName.ToString());
	}
	else
	{
		//슬립이 몸통 회전 점유 => 범프 리액션 초기화 후 기준 자세로 (거대화 중엔 배율 오프셋 기준)
		if (BumpComponent)
		{
			BumpComponent->ResetReaction();
		}
		SlipSpinMesh->SetRelativeRotation(SlipSpinMeshBaseRelRot);
		SlipSpinMesh->SetRelativeLocation(GetBodyMeshBaseRelLoc());
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

    //if (IsValid(GrabComponent))
    //{
    //    GrabComponent->SetGrabDisabledBySlip(true);
    //}
}

//미끄럼 종료 — 마찰·메시 회전 원복
void ACartPawn::EndSlip()
{
	bIsSlipping = false;
	SlipTimeRemaining = 0.f;

	if (SlipSpinMesh)
	{
		SlipSpinMesh->SetRelativeRotation(SlipSpinMeshBaseRelRot);
		SlipSpinMesh->SetRelativeLocation(GetBodyMeshBaseRelLoc()); //거대화 중엔 배율 오프셋 기준
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->GroundFriction = DefaultGroundFriction;
	}

    //if (IsValid(GrabComponent))
    //{
    //    GrabComponent->SetGrabDisabledBySlip(false);
    //}
}

void ACartPawn::SetLoadRatio(float InLoadRatio)
{
	//상한을 1이 아니라 2로 — 1 초과(과적)를 살려서 속도 페널티에 사용 (회전·브레이크는 Tick에서 1로 클램프)
	LoadRatio = FMath::Clamp(InLoadRatio, 0.f, 2.f);
}

//적재 변경 델리게이트 핸들러
void ACartPawn::HandleLoadInfoChanged(AActor* OwnerActor, const FLoadInfo& LoadInfo)
{
	//갯수/기준갯수 => 연속 적재율 (무게 미사용 — 무게 폐지 대비, 갯수 1개 차이도 속도에 반영)
	SetLoadRatio((float)LoadInfo.CurrentLoadedCount / FMath::Max(FullLoadCount, 1));
}

//카트가 무언가에 부딪히면 충돌 컴포넌트에 위임 (판정·페어 해결·서버 필터 전부 컴포넌트 담당)
void ACartPawn::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	if (BumpComponent)
	{
		BumpComponent->HandleHit(Other, HitNormal);
	}
}

//지정한 셰이크(없으면 기본 범프 셰이크)를 소유 클라 카메라에 재생 — 여러 충돌 시스템이 공용으로 호출하는 진입점
void ACartPawn::ClientPlayCameraShake_Implementation(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale)
{
	const TSubclassOf<UCameraShakeBase> ShakeToPlay = ShakeClass ? ShakeClass : BumpCameraShakeClass;
	if (!ShakeToPlay)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}

	PC->PlayerCameraManager->StartCameraShake(ShakeToPlay, Scale);
}

//토마토 피격 시 서버가 소유 클라에 호출 => 화면 가림 위젯 표시
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

//정산 완료 시 서버가 소유 클라에 호출 => 점수 팝업 UI 표시. 로컬·계산완료만
void ACartPawn::ClientShowCheckoutScore_Implementation(const FCheckoutScoreResult& ScoreResult)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	if (!ScoreResult.bIsCalculationCompleted)
	{
		return;
	}

	BP_ShowCheckoutScore(ScoreResult);
}

//외부에서 카트를 강제로 밀어내기 (거대카트·체크아웃존 등 공용 진입점) — 컴포넌트에 위임
void ACartPawn::ApplyExternalKnockback(const FVector& Direction, float Strength)
{
    if (BumpComponent)
    {
        BumpComponent->ApplyKnockback(Direction, Strength);
    }
}

//몸통 메시 탐색 + 기준 상대회전/위치 1회 캡처 (슬립·범프 공용)
void ACartPawn::EnsureBodyMeshResolved()
{
    if (bBodyMeshResolved)
    {
        return;
    }

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
    }

    if (SlipSpinMesh)
    {
        SlipSpinMeshBaseRelRot = SlipSpinMesh->GetRelativeRotation();
        SlipSpinMeshBaseRelLoc = SlipSpinMesh->GetRelativeLocation();
        bBodyMeshResolved = true;
    }
}

void ACartPawn::OnRep_CartColor()
{
	ApplyCartColor();
}

//몸통 메시 전 슬롯에 MID를 만들어 CartColor 파라미터 적용 (파라미터 없는 슬롯은 무동작)
void ACartPawn::ApplyCartColor()
{
	EnsureBodyMeshResolved();
	if (!SlipSpinMesh)
	{
		return;
	}

	const int32 NumMaterials = SlipSpinMesh->GetNumMaterials();
	for (int32 i = 0; i < NumMaterials; ++i)
	{
		if (UMaterialInstanceDynamic* MID = SlipSpinMesh->CreateAndSetMaterialInstanceDynamic(i))
		{
			MID->SetVectorParameterValue(FName("CartColor"), CartColor);
		}
	}
}

//게임 시작 전 대기 페이즈 동안 조작 가능 여부 — MainGameState가 없으면(테스트 레벨 등) 허용
bool ACartPawn::CanPlayerMove() const
{
    const AMainGameState* GS = GetWorld() ? GetWorld()->GetGameState<AMainGameState>() : nullptr;
    return !GS || GS->bCanPlayerMove();
}
