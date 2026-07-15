// BumperCart - B(카트/플레이어 조작) 파트

#include "CartBumpComponent.h"
#include "Cart/CartPawn.h"
#include "Cart/Bumpable.h"
#include "Cart/Component/CartLoadComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "BumperCart.h"

UCartBumpComponent::UCartBumpComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true); //무적 상태 복제 + RPC용

	//임시 효과음 기본값 (정식 사운드 작업 때 교체)
	static ConstructorHelpers::FObjectFinder<USoundBase> BumpSoundFinder(TEXT("/Game/Developers/dbals/Audio/Bump.Bump"));
	if (BumpSoundFinder.Succeeded()) { BumpSound = BumpSoundFinder.Object; }
}

void UCartBumpComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCart = Cast<ACartPawn>(GetOwner());
	if (OwnerCart)
	{
		//카트 이동/슬립 갱신 후에 리액션·깜빡 처리 (분리 전 pawn Tick 내 순서 유지)
		AddTickPrerequisiteActor(OwnerCart);
	}
}

void UCartBumpComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//충돌 무적: 모든 클라가 몸통 깜빡 연출을 해야 하므로 소유자 포함 전체 복제
	DOREPLIFETIME(UCartBumpComponent, bBumpInvincible);
	//필살기 게이지: UI 표시용 => 소유 클라만 (임시 디버그 테스트 중엔 전체 복제 — 배포 시 COND_OwnerOnly 복귀)
	DOREPLIFETIME(UCartBumpComponent, UltimateStack);
}

//충돌 시 가해자 게이지 +1 (서버) — 발동 중이면 미획득, 상한 클램프
void UCartBumpComponent::AddUltimateStack()
{
	if (!OwnerCart || !OwnerCart->HasAuthority() || OwnerCart->IsUltimateActive())
	{
		return;
	}
	UltimateStack = FMath::Min(UltimateStack + 1, UltimateMaxStack);
}

//발동 시 게이지 소진 (서버)
void UCartBumpComponent::ResetUltimateStack()
{
	if (OwnerCart && OwnerCart->HasAuthority())
	{
		UltimateStack = 0;
	}
}

//게이지 복제 시 — UI 갱신 훅 (지금은 자리만, UI팀이 델리게이트 연결)
void UCartBumpComponent::OnRep_UltimateStack()
{
}

void UCartBumpComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerCart)
	{
		return;
	}

	UStaticMeshComponent* BodyMesh = OwnerCart->GetBodyMesh();

	//--- 충돌 리액션(몸통 들썩·기울임): 슬립 아닐 때만(몸통 회전은 슬립이 우선), 스프링으로 기준값 복귀 ---
	if (bBumpReactionActive && !OwnerCart->IsSlipping() && BodyMesh)
	{
		auto SpringStep = [&](float& X, float& V, float MaxAbs)
		{
		    //프레임 드랍 시 dt 폭주로 발산 방지 — 0.02초 고정 스텝으로 서브스텝
		    float Remaining = DeltaTime;
		    while (Remaining > 0.f)
		    {
		        const float Step = FMath::Min(Remaining, 0.02f);
		        Remaining -= Step;

		        const float Accel = -BumpReactionStiffness * X - BumpReactionDamping * V;
		        V += Accel * Step;
		        V = FMath::Clamp(V, -10000.f, 10000.f);
		        X += V * Step;
		    }
		    X = FMath::Clamp(X, -MaxAbs, MaxAbs);
		};
		SpringStep(BumpTiltPitch, BumpTiltPitchVel, BumpTiltMaxAngle);
		SpringStep(BumpTiltRoll, BumpTiltRollVel, BumpTiltMaxAngle);
		SpringStep(BumpHopOffsetZ, BumpHopVel, BumpHopMaxHeight);

		BodyMesh->SetRelativeRotation(OwnerCart->GetBodyMeshBaseRelRot() + FRotator(BumpTiltPitch, 0.f, BumpTiltRoll));
		BodyMesh->SetRelativeLocation(OwnerCart->GetBodyMeshBaseRelLoc() + FVector(0.f, 0.f, BumpHopOffsetZ));

		//충분히 잦아들면 기준값 스냅 후 비활성화
		if (FMath::Abs(BumpTiltPitch) < 0.05f && FMath::Abs(BumpTiltPitchVel) < 0.5f &&
			FMath::Abs(BumpTiltRoll) < 0.05f && FMath::Abs(BumpTiltRollVel) < 0.5f &&
			FMath::Abs(BumpHopOffsetZ) < 0.05f && FMath::Abs(BumpHopVel) < 0.5f)
		{
			ResetReaction();
			BodyMesh->SetRelativeRotation(OwnerCart->GetBodyMeshBaseRelRot());
			BodyMesh->SetRelativeLocation(OwnerCart->GetBodyMeshBaseRelLoc());
		}
	}

	//--- 충돌 후 무적: 서버는 시간 카운트다운, 모든 클라는 몸통 깜빡 ---
	if (OwnerCart->HasAuthority() && bBumpInvincible)
	{
		BumpInvincibleTimeRemaining -= DeltaTime;
		if (BumpInvincibleTimeRemaining <= 0.f)
		{
			bBumpInvincible = false;
		}
	}
	if (BodyMesh)
	{
		UCartLoadComponent* Load = OwnerCart->GetLoadComponent();
		//필살기 발동 중엔 깜빡 안 함 — 이미 상시 면역이라 깜빡 불필요 (발동 직전 걸린 무적이 남아도 커진 카트는 안 깜빡)
		if (bBumpInvincible && !OwnerCart->IsUltimateActive())
		{
			BumpBlinkAccum += DeltaTime;
			if (BumpBlinkAccum >= BumpBlinkInterval)
			{
				BumpBlinkAccum = 0.f;
				BodyMesh->SetVisibility(!BodyMesh->GetVisibleFlag(), false);

				if (IsValid(Load))
				{
					//토글 후 플래그는 이미 새 값 => 그대로 넘겨야 몸통과 같은 위상으로 깜빡임
					Load->SetLoadDummyBlinkVisible(BodyMesh->GetVisibleFlag());
				}
			}
		}
		else if (!BodyMesh->GetVisibleFlag())
		{
			BodyMesh->SetVisibility(true, false); //무적 끝 => 확실히 보이게
			if (IsValid(Load))
			{
				Load->UpdateLoadVisual();
			}
			BumpBlinkAccum = 0.f;
		}
	}
}

//카트가 IBumpable 대상(다른 카트·차단벽·장애물)에 부딪히면 충격 세기만큼 상품을 쏟는다
void UCartBumpComponent::HandleHit(AActor* Other, const FVector& HitNormal)
{
	//충돌은 서버가 판정 (NotifyHit은 서버·소유클라 양쪽에서 떠서, 안 막으면 더블 드롭)
	if (!OwnerCart || !OwnerCart->HasAuthority())
	{
		return;
	}

	if (!Other || Other == OwnerCart)
	{
		return;
	}

	const FVector PreviousVelocity = OwnerCart->GetPreviousVelocity();

	//IBumpable(퍼블릭 상속/BP 인터페이스 추가)을 구현한 대상에만 충돌 연출+드롭. 그 외(일반 벽 등)는 그냥 막힘
	if (!Other->GetClass()->ImplementsInterface(UBumpable::StaticClass()))
	{
		//일반 벽 등 판정 밖 대상: 정면으로 박았으면 부스트만 끊김 (스침은 유지)
		if (FVector::DotProduct(PreviousVelocity, -HitNormal) > MinBumpSpeed)
		{
			OwnerCart->CancelBoost();
		}
		return;
	}

	//상대가 카트면 그 카트의 '충돌 직전' 속도를, 아니면(차단벽·장애물 등 정적) 0으로
	ACartPawn* OtherCart = Cast<ACartPawn>(Other);
	UCartBumpComponent* OtherBump = OtherCart ? OtherCart->GetBumpComponent() : nullptr;

	//거대 카트끼리는 충돌 판정 없음 — 둘 다 안 밀리는데 판정(셰이크·사운드)만 반복되는 스팸 방지. 물리로만 서로 막힘
	if (OwnerCart->IsUltimateActive() && OtherCart && OtherCart->IsUltimateActive())
	{
		return;
	}

	//무적 중이면(나 또는 상대) 이 충돌은 완전 무시 — 넉백·틸트·효과음·드롭 전부 skip
	//단 내가 필살기면 상대의 깜빡 무적을 뚫고 밀어버린다 (거대 카트가 무적 상대에 막혀 못 지나가는 문제 방지)
	if (!OwnerCart->IsUltimateActive() && (bBumpInvincible || (OtherBump && OtherBump->bBumpInvincible)))
	{
		//판정은 무시해도 정면으로 박은 부스트는 끊김 (무적 상대 밀어붙이기 방지)
		if (FVector::DotProduct(PreviousVelocity, -HitNormal) > MinBumpSpeed)
		{
			OwnerCart->CancelBoost();
		}
		return;
	}

	const FVector OtherVel = OtherCart ? OtherCart->GetPreviousVelocity() : FVector::ZeroVector;
	const FVector RelativeVelocity = PreviousVelocity - OtherVel;

	//두 액터 중심을 잇는 선 방향(수평면) — 접근 중(충돌)인지 판정에만 사용
	const FVector ToOther = (Other->GetActorLocation() - OwnerCart->GetActorLocation()).GetSafeNormal2D();
	const float Approach = FVector::DotProduct(RelativeVelocity, ToOther);
	if (Approach <= 0.f)
	{
		return; //서로 멀어지는 중이면 충돌로 치지 않음
	}

	//충돌 세기 = 상대속도 크기(2D). 부딪힌 각도와 무관하게 일관됨
	const float ClosingSpeed = RelativeVelocity.Size2D();
	if (ClosingSpeed < MinBumpSpeed && !OwnerCart->IsUltimateActive())
	{
		return; //약하게 스치는 접촉은 무시 (단 필살기는 붙어서 살짝 밀어도 발동 — 거대화로 가속 거리가 없어 못 미는 문제 방지)
	}

	//충돌 시 정산 취소 (차단벽 정산 중 충돌 시 잠깐 정산 불가)
	if (IsValid(OtherCart))
	{
		OtherCart->CancelCheckout(0.5f);
	}

	//출발 그레이스: 막 움직이기 시작한(부스트 아님) 카트가 단독으로 만든 충돌은 무효 — 바로 앞 카트 밀기 오판정 방지
	//'정당한 돌진자'(상대 쪽으로 실제 이동 중 + 그레이스 지남 or 부스트)가 한 명도 없으면 충돌로 안 침
	//필살기는 그레이스 무시 + 상대 쪽으로 조금만 움직여도 정당한 돌진자 (거대 카트가 앞 상대를 못 미는 문제 방지)
	const float MyToward = FVector::DotProduct(PreviousVelocity, ToOther);
	const float OtherToward = OtherCart ? FVector::DotProduct(OtherVel, -ToOther) : 0.f;
	const float MyTowardThreshold = OwnerCart->IsUltimateActive() ? 10.f : 50.f;
	const bool bMyChargeLegit = MyToward > MyTowardThreshold && (OwnerCart->IsBoosting() || OwnerCart->IsUltimateActive() || OwnerCart->GetTimeSinceMoveStart() >= BumpStartGraceTime);
	const float OtherTowardThreshold = (OtherCart && OtherCart->IsUltimateActive()) ? 10.f : 50.f;
	const bool bOtherChargeLegit = OtherCart && OtherToward > OtherTowardThreshold && (OtherCart->IsBoosting() || OtherCart->IsUltimateActive() || OtherCart->GetTimeSinceMoveStart() >= BumpStartGraceTime);
	if (!bMyChargeLegit && !bOtherChargeLegit)
	{
		return;
	}

	//--- 충돌 확정: 첫 NotifyHit이 나+상대를 전부 해결 ---
	//아래서 양쪽에 무적이 걸리면 상대 쪽 NotifyHit은 위 무적 체크에서 무효화되므로,
	//사운드·셰이크·넉백·리액션·드롭을 여기서 양쪽 다 처리해야 피해자 쪽이 누락되지 않는다

	//셰이크·충돌음 — 나 + 상대 (세기는 접근속도 비례, 동일 적용. nullptr => 카트 기본 셰이크)
	const float ShakeScale = FMath::GetMappedRangeValueClamped(
		FVector2D(MinBumpSpeed, BumpShakeFullSpeed),
		FVector2D(BumpShakeScale, BumpShakeMaxScale),
		ClosingSpeed);
	OwnerCart->ClientPlayCameraShake(nullptr, ShakeScale);
	ClientPlayBumpSound();
	if (OtherCart && OtherBump)
	{
		OtherCart->ClientPlayCameraShake(nullptr, ShakeScale);
		OtherBump->ClientPlayBumpSound();
	}

	//드롭 역할 + 넉백 세기. 우선순위: 필살기 > 부스터 상쇄 > 부스터 > 일반
	const bool bIUlt = OwnerCart->IsUltimateActive();
	const bool bOtherUlt = OtherCart && OtherCart->IsUltimateActive();
	const bool bIBoost = OwnerCart->IsBoosting();
	const bool bOtherBoost = OtherCart && OtherCart->IsBoosting();
	EDropCollisionRole DropRole = EDropCollisionRole::Normal;
	EDropCollisionRole OtherDropRole = EDropCollisionRole::Normal;
	const float NormalKnock = FMath::Min(ClosingSpeed * NormalKnockbackScale, NormalKnockbackMax);
	float OtherKnockStrength = NormalKnock;
	float MyKnockStrength = OtherCart ? NormalKnock : 0.f; //벽·장애물은 나를 안 밀어냄
	bool bBoostClash = false; //부스터끼리 상쇄 충돌 (드롭 스킵용)
	if (bIUlt) //필살기는 상대가 부스터여도 풀 파워 (필살기끼리는 위에서 이미 판정 무시)
	{
		DropRole = EDropCollisionRole::BoosterInstigator;
		OtherDropRole = EDropCollisionRole::BoostedTarget;
		OtherKnockStrength = BoostKnockbackStrength;
		MyKnockStrength = 0.f;
	}
	else if (bOtherUlt)
	{
		DropRole = EDropCollisionRole::BoostedTarget;
		OtherDropRole = EDropCollisionRole::BoosterInstigator;
		OtherKnockStrength = 0.f;
		MyKnockStrength = BoostKnockbackStrength;
	}
	else if (bIBoost && bOtherBoost)
	{
		//부스터끼리는 방향 무관 서로 상쇄: 가벼운 고정 넉백으로 대칭 튕김 + 드롭 없음
		//(레이스로 한쪽만 가해자 되던 비대칭 수정. 부스트는 아래서 양쪽 다 끊김)
		OtherKnockStrength = BoostClashKnockback;
		MyKnockStrength = BoostClashKnockback;
		bBoostClash = true;
	}
	else if (bIBoost)
	{
		DropRole = EDropCollisionRole::BoosterInstigator; //내가 부스터로 박음 => 덜 흘림
		OtherDropRole = EDropCollisionRole::BoostedTarget;
		OtherKnockStrength = BoostKnockbackStrength; //부스터에 박힌 쪽은 더 멀리 (부스트 비비기 방지)
		MyKnockStrength = 0.f;                       //부스터 본인은 안 밀림
	}
	else if (bOtherBoost)
	{
		DropRole = EDropCollisionRole::BoostedTarget; //부스터한테 박힘 => 더 흘림
		OtherDropRole = EDropCollisionRole::BoosterInstigator;
		OtherKnockStrength = 0.f;
		MyKnockStrength = BoostKnockbackStrength;
	}
	//필살기 완전 면역 — 가해자든 피해자든 절대 안 밀림
	if (OwnerCart->IsUltimateActive())
	{
		MyKnockStrength = 0.f;
	}
	if (OtherCart && OtherCart->IsUltimateActive())
	{
		OtherKnockStrength = 0.f;
	}

	//넉백 — ApplyKnockback이 넉백+몸통 리액션 공용 처리. 안 밀리는 쪽도 리액션은 재생
	const float ReactionIntensity = FMath::GetMappedRangeValueClamped(
		FVector2D(MinBumpSpeed, BumpShakeFullSpeed), FVector2D(0.25f, 1.f), ClosingSpeed);
	if (OtherBump)
	{
		if (OtherKnockStrength > 0.f)
		{
			OtherBump->ApplyKnockback(ToOther, OtherKnockStrength);
		}
		else
		{
			OtherBump->MulticastPlayBumpReaction(ToOther, ReactionIntensity); //부스터 본인(안 밀림)
		}
	}
	if (MyKnockStrength > 0.f)
	{
		ApplyKnockback(-ToOther, MyKnockStrength);
	}
	else
	{
		MulticastPlayBumpReaction(-ToOther, ReactionIntensity); //벽 충돌·부스터 본인(안 밀림)
	}

	//드롭 — 나 + 상대 (필살기 발동자는 자기 상품 안 흘림, 부스터 상쇄는 양쪽 다 안 흘림)
	if (!OwnerCart->IsUltimateActive() && !bBoostClash)
	{
		RequestSpill(ClosingSpeed * BumpImpulseScale, DropRole);
	}
	if (OtherBump && !bBoostClash && !(OtherCart && OtherCart->IsUltimateActive()))
	{
		OtherBump->RequestSpill(ClosingSpeed * OtherBump->BumpImpulseScale, OtherDropRole);
	}

	//부딪히면 부스트 끊김 — 충돌한 양쪽 모두 (역할·넉백 판정엔 위에서 이미 반영됨. 필살기는 CancelBoost 무관)
	OwnerCart->CancelBoost();
	if (OtherCart)
	{
		OtherCart->CancelBoost();
	}

	//필살기 게이지 — 정당한 돌진자(가해자)만 +1. 정면충돌은 양쪽 legit이라 둘 다 획득. 발동 중인 카트는 AddUltimateStack에서 자동 제외
	if (bMyChargeLegit)
	{
		AddUltimateStack();
	}
	if (OtherBump && bOtherChargeLegit)
	{
		OtherBump->AddUltimateStack();
	}

	//충돌 성립 => 잠깐 무적 (연타/비비기 방지, 그동안 깜빡).
	//필살기 발동자는 이미 상시 면역이라 깜빡 무적 안 걸음. 맞은 상대(일반)는 기존대로 깜빡 무적 부여
	if (!OwnerCart->IsUltimateActive())
	{
		StartInvincibility();
	}
	if (OtherBump && !(OtherCart && OtherCart->IsUltimateActive()))
	{
		OtherBump->StartInvincibility();
	}
}

//외부에서 카트를 강제로 밀어내기 (서버) — 넉백 + 몸통 리액션 공용 처리
void UCartBumpComponent::ApplyKnockback(const FVector& Direction, float Strength)
{
	if (!OwnerCart || !OwnerCart->HasAuthority())
	{
		return;
	}

	if (OwnerCart->IsUltimateActive())
	{
		return; //필살기 발동 중엔 외부 넉백(거대카트·체크아웃존 등)도 면역
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

	//수평 넉백 + 세기 비례로 위로 살짝 띄움
	FVector LaunchVelocity = KnockbackDirection * Strength;
	LaunchVelocity.Z = FMath::Min(Strength * KnockbackUpRatio, KnockbackUpMax);
	OwnerCart->LaunchCharacter(LaunchVelocity, true, true);

	//충돌 판정 나는 기믹(거대카트 등)에 밀리면 부스트 끊김
	OwnerCart->CancelBoost();

	//넉백과 함께 몸통 리액션도 재생 (외부 시스템 공용). 세기는 Strength/기준값
	const float ReactionIntensity = FMath::Clamp(Strength / BumpReactionKnockbackRef, 0.25f, 1.f);
	MulticastPlayBumpReaction(KnockbackDirection, ReactionIntensity);
}

//충돌 리액션(몸통 들썩·기울임)을 로컬에서 재생
void UCartBumpComponent::MulticastPlayBumpReaction_Implementation(FVector WorldPushDir, float Intensity)
{
	//슬립 중엔 몸통 스핀이 우선 => 범프 틸트 생략
	if (Intensity <= 0.f || !OwnerCart || OwnerCart->IsSlipping())
	{
		return;
	}

	UStaticMeshComponent* BodyMesh = OwnerCart->GetBodyMesh();
	if (!BodyMesh)
	{
		return; //몸통 메시 없으면 연출만 생략
	}

	//밀리는 방향을 로컬로 변환 => 부딪힌 쪽(반대쪽)이 들리게 pitch/roll 충격
	const FVector LocalPush = OwnerCart->GetActorTransform().InverseTransformVectorNoScale(WorldPushDir.GetSafeNormal2D());
	const float StruckFwd = -LocalPush.X;   //+면 앞에서 맞음 => 앞이 들림(nose up)
	const float StruckRight = -LocalPush.Y; //+면 오른쪽에서 맞음 => 오른쪽이 들림
	const float Amt = FMath::Clamp(Intensity, 0.f, 1.f);

	//스프링 속도에 충격 누적 (부호 반대로 보이면 아래 두 줄 뒤집기)
	BumpTiltPitchVel += StruckFwd * BumpTiltStrength * Amt;
	BumpTiltRollVel += StruckRight * BumpTiltStrength * Amt;
	BumpHopVel += BumpHopStrength * Amt; //위로 들썩
	bBumpReactionActive = true;
}

//충돌음을 소유 클라에서 재생 (BumpSound 비어있으면 무음)
void UCartBumpComponent::ClientPlayBumpSound_Implementation()
{
	if (BumpSound)
	{
		UGameplayStatics::PlaySound2D(this, BumpSound);
	}
}

//스필(드롭) 공통 진입점 — 쿨다운 적용 후 C에 낙하 요청 (충돌·부스터오용 공용)
void UCartBumpComponent::RequestSpill(float Impulse, EDropCollisionRole DropRole)
{
	UCartLoadComponent* Load = OwnerCart ? OwnerCart->GetLoadComponent() : nullptr;
	if (!Load)
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
	Load->RequestDropProduct(Impulse, DropRole);
}

//충돌 후 무적 시작 (서버) — 지속시간 동안 추가 충돌 무시. bBumpInvincible 복제로 전 클라가 몸통 깜빡
void UCartBumpComponent::StartInvincibility()
{
	if (!OwnerCart || !OwnerCart->HasAuthority())
	{
		return;
	}
	bBumpInvincible = true;
	BumpInvincibleTimeRemaining = BumpInvincibleDuration;
}

//리액션 스프링 상태 초기화 — 슬립 시작 등 몸통 회전을 넘겨줄 때
void UCartBumpComponent::ResetReaction()
{
	BumpTiltPitch = BumpTiltRoll = BumpHopOffsetZ = 0.f;
	BumpTiltPitchVel = BumpTiltRollVel = BumpHopVel = 0.f;
	bBumpReactionActive = false;
}
