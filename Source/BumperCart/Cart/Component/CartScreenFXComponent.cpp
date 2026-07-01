// BumperCart - B(카트/플레이어 조작) 파트
// 부스트 중 화면 가장자리 스피드라인 (로컬 플레이어 전용) 연출 컴포넌트

#include "CartScreenFXComponent.h"
#include "Cart/CartPawn.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

UCartScreenFXComponent::UCartScreenFXComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	//머티리얼은 BP_CartPawn의 CartScreenFXComponent에서 지정 (SpeedLineMaterial). 비어있으면 무동작
}

void UCartScreenFXComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCart = Cast<ACartPawn>(GetOwner());
}

void UCartScreenFXComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//내가 보는 화면(로컬 플레이어)만 처리 — 원격 카트의 카메라는 렌더되지 않으므로 무시
	if (!OwnerCart || !OwnerCart->IsLocallyControlled())
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
