// BumperCart - B(카트/플레이어 조작) 파트
// 카트 연출(FX) 컴포넌트 — 화면 가장자리 스피드라인(PP, 로컬 전용) + 바퀴 월드 FX(바닥 리본·브레이크 스파크)

#include "CartScreenFXComponent.h"
#include "Cart/CartPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "BumperCart.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

UCartScreenFXComponent::UCartScreenFXComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	//대부분 에셋(PP 머티리얼·나이아가라)은 BP_CartPawn의 이 컴포넌트에서 지정. 비어있으면 해당 연출만 무동작
	//스키드 데칼 머티리얼만 C++에서 자동 로드 — BP 오버라이드는 main 병합 시 날아가기 쉬워 코드에 고정
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SkidDecalFinder(TEXT("/Game/Developers/dbals/FX/M_SkidDecal.M_SkidDecal"));
	if (SkidDecalFinder.Succeeded())
	{
		SkidDecalMaterial = SkidDecalFinder.Object;
	}
}

void UCartScreenFXComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCart = Cast<ACartPawn>(GetOwner());

	//데디케이티드 서버는 화면이 없으므로 FX 생성 안 함
	if (OwnerCart && GetNetMode() != NM_DedicatedServer)
	{
		SetupWheelFX();
	}
}

void UCartScreenFXComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerCart)
	{
		return;
	}

	//--- 바퀴 월드 FX: 모든 클라에서 구동 (속도는 RepMovement, 부스트·브레이크는 복제 플래그) ---
	const float ForwardSpeed = FVector::DotProduct(OwnerCart->GetVelocity(), OwnerCart->GetActorForwardVector());
	DriveWheelFX(ForwardSpeed, DeltaTime);

	//--- 화면 스피드라인: 내가 보는 화면(로컬 플레이어)만 ---
	if (!OwnerCart->IsLocallyControlled())
	{
		return;
	}

	if (!EnsureSpeedLineMID())
	{
		return;
	}

	//부스트 상태로 목표 세기 결정 후 부드럽게 보간 (급격한 팝핑 방지)
	const float TargetIntensity = OwnerCart->IsBoosting() ? 1.f : 0.f;
	CurrentIntensity = FMath::FInterpTo(CurrentIntensity, TargetIntensity, DeltaTime, IntensityInterpSpeed);
	SpeedLineMID->SetScalarParameterValue(FName("Intensity"), CurrentIntensity);
}

//로컬 카메라를 찾아 MID를 PostProcess 블렌더블로 얹는다 (최초 1회)
bool UCartScreenFXComponent::EnsureSpeedLineMID()
{
	if (SpeedLineMID)
	{
		return true;
	}
	if (!SpeedLineMaterial || !OwnerCart)
	{
		return false;
	}

	UCameraComponent* Camera = OwnerCart->FindComponentByClass<UCameraComponent>();
	if (!Camera)
	{
		return false;
	}

	SpeedLineMID = UMaterialInstanceDynamic::Create(SpeedLineMaterial, this);
	if (!SpeedLineMID)
	{
		return false;
	}

	//시작은 꺼진 상태(0)로. 세기 구동은 Tick에서
	SpeedLineMID->SetScalarParameterValue(FName("Intensity"), 0.f);
	Camera->PostProcessSettings.AddBlendable(SpeedLineMID, 1.f);
	return true;
}

//뒷바퀴 소켓을 가진 카트 메시를 찾아 리본·스파크 컴포넌트를 생성/부착
void UCartScreenFXComponent::SetupWheelFX()
{
	//BP에서 붙인 카트 메시(SM_Shopping_Cart) 중 뒷바퀴 소켓 2개를 가진 것을 찾는다
	USceneComponent* AttachTarget = nullptr;
	TArray<UStaticMeshComponent*> MeshComps;
	GetOwner()->GetComponents(MeshComps);
	for (UStaticMeshComponent* Mesh : MeshComps)
	{
		if (Mesh && Mesh->DoesSocketExist(RearWheelSocketLeft) && Mesh->DoesSocketExist(RearWheelSocketRight))
		{
			AttachTarget = Mesh;
			break;
		}
	}

	//소켓을 못 찾으면 루트(캡슐) 기준 기존 오프셋으로 폴백 — 연출이 끊기지 않게 하고 경고만 남긴다
	FVector FallbackLeft = FVector::ZeroVector;
	FVector FallbackRight = FVector::ZeroVector;
	FName LeftSocket = RearWheelSocketLeft;
	FName RightSocket = RearWheelSocketRight;
	if (!AttachTarget)
	{
		UE_LOG(LogBumperCart, Warning, TEXT("CartFX: 뒷바퀴 소켓(%s/%s)을 가진 메시를 찾지 못해 루트 오프셋으로 부착합니다. SM_Shopping_Cart 소켓을 확인하세요."),
			*RearWheelSocketLeft.ToString(), *RearWheelSocketRight.ToString());
		AttachTarget = GetOwner()->GetRootComponent();
		LeftSocket = NAME_None;
		RightSocket = NAME_None;
		FallbackLeft = FVector(-10.f, -20.f, -88.f);  //구버전 하드코딩 위치 (캡슐 바닥 뒷바퀴 부근)
		FallbackRight = FVector(-10.f, 20.f, -88.f);
	}

	//바닥 리본: 항상 재생(파라미터로 강도 제어) / 스파크: 브레이크 때만 Activate
	RibbonFXLeft = SpawnWheelFX(GroundRibbonSystem, AttachTarget, LeftSocket, FallbackLeft, true);
	RibbonFXRight = SpawnWheelFX(GroundRibbonSystem, AttachTarget, RightSocket, FallbackRight, true);
	SparkFXLeft = SpawnWheelFX(BrakeSparkSystem, AttachTarget, LeftSocket, FallbackLeft, false);
	SparkFXRight = SpawnWheelFX(BrakeSparkSystem, AttachTarget, RightSocket, FallbackRight, false);

	//스키드 데칼용 뒷바퀴 위치 조회 정보 캐시 (소켓 or 폴백 오프셋)
	WheelAttach = AttachTarget;
	WheelSocketLeftResolved = LeftSocket;
	WheelSocketRightResolved = RightSocket;
	WheelFallbackLeft = FallbackLeft;
	WheelFallbackRight = FallbackRight;
}

//나이아가라 컴포넌트 하나를 생성해 부착. 소켓이 NAME_None이면 FallbackOffset 위치 사용
UNiagaraComponent* UCartScreenFXComponent::SpawnWheelFX(UNiagaraSystem* System, USceneComponent* AttachTo, FName Socket, const FVector& FallbackOffset, bool bStartActive)
{
	if (!System || !AttachTo)
	{
		return nullptr;
	}

	UNiagaraComponent* Fx = NewObject<UNiagaraComponent>(GetOwner());
	Fx->SetAsset(System);
	Fx->SetAutoActivate(bStartActive);
	Fx->SetupAttachment(AttachTo, Socket);
	Fx->SetRelativeLocation(FallbackOffset);
	Fx->RegisterComponent();
	return Fx;
}

//리본 파라미터 갱신 + 브레이크 스파크 on/off·히트 누적
void UCartScreenFXComponent::DriveWheelFX(float ForwardSpeed, float DeltaTime)
{
	//--- 바닥 리본 ---
	//계단식 게이트(짧은 10 램프): MinSpeed 넘으면 꽉 참, 미만이면 0. 후진(음수)도 0
	//무거우면 최고속도가 MinSpeed 아래라 자동 OFF, 부스트 땐 빨라져서 ON
	const float SpeedLineAlpha = FMath::GetMappedRangeValueClamped(
		FVector2D(SpeedLineMinSpeed, SpeedLineMinSpeed + 10.f), FVector2D(0.f, 1.f), ForwardSpeed);
	//적재무게 => Niagara에서 굵기/길이 커브에 사용 (가벼움 0 ~ 무거움 1)
	const float SpeedLineLoad = FMath::Clamp(OwnerCart->GetLoadRatio(), 0.f, 1.f);
	const float BoostValue = OwnerCart->IsBoosting() ? 1.f : 0.f;

	auto DriveRibbon = [&](UNiagaraComponent* Fx)
	{
		if (!Fx) { return; }
		Fx->SetVariableFloat(FName("SpeedAlpha"), SpeedLineAlpha);
		Fx->SetVariableFloat(FName("LoadRatio"), SpeedLineLoad);
		Fx->SetVariableFloat(FName("Boost"), BoostValue);
	};
	DriveRibbon(RibbonFXLeft);
	DriveRibbon(RibbonFXRight);

	//--- 브레이크 스파크: 브레이크 중 + 충분히 빠를 때만. 상태가 바뀔 때만 토글 ---
	const bool bWantSparks = OwnerCart->IsBraking() && ForwardSpeed > BrakeSparkMinSpeed;
	if (bWantSparks != bSparksActive)
	{
		bSparksActive = bWantSparks;
		auto ToggleSpark = [&](UNiagaraComponent* Fx)
		{
			if (!Fx) { return; }
			if (bWantSparks) { Fx->Activate(true); }
			else { Fx->Deactivate(); }
		};
		ToggleSpark(SparkFXLeft);
		ToggleSpark(SparkFXRight);
	}

	//--- 스키드 마크(바닥 데칼): 브레이크 중 뒷바퀴가 일정 거리 이동할 때마다 자국을 찍는다 ---
	DriveSkidDecals(ForwardSpeed);

	//--- 브레이크 히트: 밟는 동안 차오르고(램프 시간), 떼면 2배 속도로 식는다 ---
	//Niagara에서 User.BrakeHeat로 스파크 색 보간 (0=주황 -> 1=백황, 달아오르는 연출)
	if (bWantSparks)
	{
		BrakeHeat = FMath::Min(1.f, BrakeHeat + DeltaTime / BrakeHeatRampTime);
	}
	else
	{
		BrakeHeat = FMath::Max(0.f, BrakeHeat - DeltaTime * 2.f / BrakeHeatRampTime);
	}
	if (SparkFXLeft) { SparkFXLeft->SetVariableFloat(FName("BrakeHeat"), BrakeHeat); }
	if (SparkFXRight) { SparkFXRight->SetVariableFloat(FName("BrakeHeat"), BrakeHeat); }
}

//브레이크 스키드 데칼 갱신 — 브레이크 중 + 충분히 빠를 때, 양쪽 뒷바퀴 위치에 거리 기반으로 자국을 찍는다
void UCartScreenFXComponent::DriveSkidDecals(float ForwardSpeed)
{
	const bool bWantSkid = OwnerCart && OwnerCart->IsBraking() && ForwardSpeed > SkidDecalMinSpeed;
	if (!bWantSkid || !SkidDecalMaterial)
	{
		//브레이크를 뗐거나 조건 미달 => 다음 자국은 새 라인으로 시작(연속선 끊기)
		bHasLastSkidLeft = false;
		bHasLastSkidRight = false;
		return;
	}

	TrySpawnSkidDecal(GetRearWheelWorldPos(true), LastSkidPosLeft, bHasLastSkidLeft);
	TrySpawnSkidDecal(GetRearWheelWorldPos(false), LastSkidPosRight, bHasLastSkidRight);
}

//한쪽 뒷바퀴: 이전 자국에서 SkidDecalSpacing 이상 벌어졌을 때만 새 데칼 (자국 간격 일정 유지)
void UCartScreenFXComponent::TrySpawnSkidDecal(const FVector& WheelPos, FVector& LastPos, bool& bHasLast)
{
	if (!bHasLast)
	{
		LastPos = WheelPos;
		bHasLast = true;
		SpawnSkidDecalAt(WheelPos);
		return;
	}

	//고속(부스트 직후 등)에선 한 프레임에 간격의 몇 배를 이동해 자국 사이가 점점이 비므로,
	//마지막 자국에서 현재 위치까지 간격 단위로 걸어가며 빈 구간을 전부 채운다
	float Dist = FVector::Dist2D(WheelPos, LastPos);
	if (Dist < SkidDecalSpacing)
	{
		return; //아직 충분히 안 움직임 — 자국 안 찍음
	}

	const FVector Dir = (WheelPos - LastPos).GetSafeNormal2D();
	int32 Guard = 0; //순간이동 등 비정상 거리 폭주 방지
	while (Dist >= SkidDecalSpacing && Guard++ < 32)
	{
		LastPos += Dir * SkidDecalSpacing;
		Dist -= SkidDecalSpacing;
		SpawnSkidDecalAt(LastPos);
	}
}

//지정 위치 바닥에 자국 데칼 하나 스폰 (수명 후 자동 소멸 + 마지막에 페이드아웃)
void UCartScreenFXComponent::SpawnSkidDecalAt(const FVector& WorldPos)
{
	UWorld* World = GetWorld();
	if (!World || !SkidDecalMaterial)
	{
		return;
	}

	//데칼은 로컬 X축으로 바닥에 투영된다 => X를 아래로, 길이축(DecalSize.Z)을 '실제 이동 방향'에 정렬
	//미끄러질 땐 카트가 바라보는 방향(yaw)과 실제 진행 방향(속도)이 달라서, 속도 방향을 써야 자국이 경로를 따라 눕는다
	FVector MoveDir = OwnerCart ? OwnerCart->GetVelocity() : FVector::ZeroVector;
	MoveDir.Z = 0.f;
	MoveDir = MoveDir.GetSafeNormal();
	if (MoveDir.IsNearlyZero())
	{
		MoveDir = OwnerCart ? OwnerCart->GetActorForwardVector() : FVector::ForwardVector;
	}
	//X=아래(-Z) 투영, Z=이동 방향(길이축), Y=좌우 폭
	const FRotator DecalRot = FRotationMatrix::MakeFromXZ(FVector(0.f, 0.f, -1.f), MoveDir).Rotator();

	UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(World, SkidDecalMaterial, SkidDecalSize, WorldPos, DecalRot, SkidDecalLifetime);
	if (Decal)
	{
		//수명 끝나기 직전부터 페이드아웃 시작
		const float FadeStart = FMath::Max(0.f, SkidDecalLifetime - SkidDecalFadeDuration);
		Decal->SetFadeOut(FadeStart, SkidDecalFadeDuration, false);
	}
}

//뒷바퀴 월드 위치 — 소켓이 있으면 소켓 위치, 없으면 WheelAttach 기준 fallback 오프셋
FVector UCartScreenFXComponent::GetRearWheelWorldPos(bool bLeft) const
{
	if (!WheelAttach)
	{
		return OwnerCart ? OwnerCart->GetActorLocation() : FVector::ZeroVector;
	}
	const FName Socket = bLeft ? WheelSocketLeftResolved : WheelSocketRightResolved;
	if (Socket != NAME_None)
	{
		return WheelAttach->GetSocketLocation(Socket);
	}
	return WheelAttach->GetComponentTransform().TransformPosition(bLeft ? WheelFallbackLeft : WheelFallbackRight);
}

//컴포넌트 종료 시 남아있는 토마토 위젯 정리
void UCartScreenFXComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveTomatoScreenBlock();

	Super::EndPlay(EndPlayReason);
}

//토마토에 맞으면 화면 가림 위젯을 Duration초간 표시 (로컬 플레이어만)
void UCartScreenFXComponent::ApplyTomatoScreenBlock(float Duration)
{
	if (!OwnerCart)
	{
		return;
	}

	//로컬 플레이어만 가림
	if (!OwnerCart->IsLocallyControlled())
	{
		return;
	}

	if (!TomatoScreenBlockWidgetClass)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(OwnerCart->GetController());
	if (!PlayerController)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	//이미 토마토가 화면을 가릴 경우, 제거 후 다시 화면 가림
	RemoveTomatoScreenBlock();

	TomatoScreenBlockWidget = CreateWidget<UUserWidget>(PlayerController, TomatoScreenBlockWidgetClass);
	if (!TomatoScreenBlockWidget)
	{
		return;
	}

	TomatoScreenBlockWidget->AddToViewport(TomatoScreenBlockZOrder);

	if (Duration <= 0.f)
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		TomatoScreenBlockTimerHandle,
		this,
		&ThisClass::RemoveTomatoScreenBlock,
		Duration,
		false
	);
}

//토마토 화면 가림 위젯 제거
void UCartScreenFXComponent::RemoveTomatoScreenBlock()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(TomatoScreenBlockTimerHandle);
	}

	if (TomatoScreenBlockWidget)
	{
		TomatoScreenBlockWidget->RemoveFromParent();
		TomatoScreenBlockWidget = nullptr;
	}
}
