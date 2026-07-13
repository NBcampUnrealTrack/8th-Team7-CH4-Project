// BumperCart - B(카트/플레이어 조작) 파트

#include "CartCameraComponent.h"
#include "Cart/CartPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

UCartCameraComponent::UCartCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCartCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCart = Cast<ACartPawn>(GetOwner());
	if (OwnerCart)
	{
		CameraBoom = OwnerCart->GetCameraBoom();
		FollowCamera = OwnerCart->GetFollowCamera();

		//카트 이동/회전 확정 후 카메라 갱신 (분리 전 pawn Tick 말미와 동일 순서)
		AddTickPrerequisiteActor(OwnerCart);
	}
}

void UCartCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//내 화면 연출이므로 소유 클라에서만
	if (!OwnerCart || !OwnerCart->IsLocallyControlled() || !CameraBoom || !FollowCamera)
	{
		return;
	}

	//속도 기반 알파 (0=저속, 1=고속)
	const float SpeedAlpha = FMath::Clamp(OwnerCart->GetVelocity().Size2D() / FMath::Max(SpeedZoomFullSpeed, 1.f), 0.f, 1.f);

	//속도로 FOV·암길이 보간
	float TargetArm = FMath::Lerp(CameraArmMin, CameraArmMax, SpeedAlpha);
	float TargetFov = FMath::Lerp(CameraFovMin, CameraFovMax, SpeedAlpha);
	if (OwnerCart->IsBoosting())
	{
		TargetArm += BoostExtraArm;
		TargetFov += BoostExtraFov;
	}

	//--- 커스텀 카메라 충돌: 카트 본체에서 원하는 카메라 위치까지 구 스윕 => 막히면 그 거리로 암 제한 ---
	float SafeArm = TargetArm;
	if (UWorld* World = GetWorld())
	{
		const FVector BoomOrigin = CameraBoom->GetComponentLocation();
		const FRotator BoomRot   = CameraBoom->GetComponentRotation();
		const FVector ArmDir      = BoomRot.RotateVector(FVector(-1.f, 0.f, 0.f)); //암이 뻗는 방향(뒤·아래), 단위벡터
		const FVector DesiredCamPos = BoomOrigin + ArmDir * TargetArm;

		//트레이스 시작은 카트 본체
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(CartCameraCollision), false, OwnerCart);
		if (World->SweepSingleByChannel(Hit, OwnerCart->GetActorLocation(), DesiredCamPos, FQuat::Identity, ECC_Camera,
				FCollisionShape::MakeSphere(CameraCollisionProbeSize), Params))
		{
			//막힌 지점을 붐 원점 기준 암 길이로 환산(암 방향에 투영) 후, 최소 암 이하로는 안 당김
			const float BlockedArm = FVector::DotProduct(Hit.Location - BoomOrigin, ArmDir);
			SafeArm = FMath::Clamp(BlockedArm, CameraCollisionMinArm, TargetArm);
		}
	}

	//충돌 결과를 향해 비대칭 보간(당길 땐 빠르게 관통 최소화, 풀 땐 천천히 출렁임 방지) + FOV는 기존 줌 속도
	const bool bPullingIn = SafeArm < CameraBoom->TargetArmLength;
	const float ArmInterpSpeed = bPullingIn ? CameraCollisionPullInSpeed : CameraCollisionPullOutSpeed;
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, SafeArm, DeltaTime, ArmInterpSpeed);
	FollowCamera->FieldOfView   = FMath::FInterpTo(FollowCamera->FieldOfView, TargetFov, DeltaTime, CameraZoomInterpSpeed);
}
