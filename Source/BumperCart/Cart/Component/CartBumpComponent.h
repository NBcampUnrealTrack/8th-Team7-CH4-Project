// BumperCart - B(카트/플레이어 조작) 파트
// 카트 충돌(범프) 컴포넌트 — 충돌 판정(그레이스·무적·페어 해결)·넉백·리액션 스프링·무적 깜빡·스필 전담

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Cart/CartLoadTypes.h"
#include "CartBumpComponent.generated.h"

class ACartPawn;
class USoundBase;

//CartPawn::NotifyHit(서버)에서 위임받아 충돌 전체를 처리. 복제 컴포넌트(무적 상태 + RPC)
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BUMPERCART_API UCartBumpComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCartBumpComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//서버: CartPawn::NotifyHit 위임 — 충돌 판정 + 페어 해결(양쪽 사운드·셰이크·넉백·스필·무적) 전체
	void HandleHit(AActor* Other, const FVector& HitNormal);

	//서버: 외부 넉백 (CartPawn::ApplyExternalKnockback 위임 — 거대카트·체크아웃존 등)
	void ApplyKnockback(const FVector& Direction, float Strength);

	//충돌 후 무적 중인지
	bool IsInvincible() const { return bBumpInvincible; }

	//리액션 스프링 상태 초기화 — 슬립이 몸통 회전을 점유할 때 호출
	void ResetReaction();

	//---------- 필살기 게이지 ----------
	//현재 게이지 스택 (UI·발동 판정용)
	int32 GetUltimateStack() const { return UltimateStack; }

	//게이지 +1 (서버) — 충돌 시 가해자만 호출. 발동 중이면 미획득, 상한 클램프
	void AddUltimateStack();

	//게이지 소진 (서버) — 발동 시 CartPawn이 호출
	void ResetUltimateStack();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//충돌 후 무적 시작 (서버) — 지속시간 동안 추가 충돌 무시, 복제되어 전 클라 깜빡
	void StartInvincibility();

	//스필(드롭) 공통 진입점 — 쿨다운 적용 후 C에 낙하 요청
	void RequestSpill(float Impulse, EDropCollisionRole DropRole);

	//충돌음을 소유 클라에서 재생 (서버 HandleHit에서 호출 — 셰이크와 동일 패턴, 멀티캐스트 중복재생 회피)
	UFUNCTION(Client, Unreliable)
	void ClientPlayBumpSound();

	//충돌 리액션(몸통 들썩·기울임)을 모든 클라에 재생. 서버에서만 호출
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayBumpReaction(FVector WorldPushDir, float Intensity);

	//---------- 충돌/스필 드롭 판정 ----------
	//충격속도(cm/s)를 C 드롭 충격량으로 환산하는 배율 (충돌용)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpImpulseScale = 2.0f;

	//이 접근속도(cm/s) 미만의 약한 접촉은 충돌로 안 침
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float MinBumpSpeed = 600.f;

	//출발 그레이스(초) — 정지에서 움직이기 시작한 지 이 시간 안에 만든 충돌은 무효 (바로 앞 카트 밀기 오판정 방지). 부스트는 예외
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpStartGraceTime = 0.5f;

	//충돌 판정 후 무적 지속(초) — 이 동안 추가 충돌 완전 무시 + 몸통 깜빡 (연타/비비기 방지)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpInvincibleDuration = 1.f;

	//무적 중 몸통 깜빡임 간격(초)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0.01"))
	float BumpBlinkInterval = 0.08f;

	//한 번 쏟은 뒤 다음 드롭까지 최소 간격(초) — 모든 스필(충돌·부스터오용) 공통
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpDropCooldown = 0.5f;

	//충돌 카메라 쉐이크 세기: 약한 충돌(MinBumpSpeed 부근)일 때의 배율 (쉐이크 클래스는 CartPawn.BumpCameraShakeClass)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpShakeScale = 0.4f;

	//충돌 카메라 쉐이크 세기: 강한 충돌(BumpShakeFullSpeed 이상)일 때의 배율 — 셀수록 더 세게
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpShakeMaxScale = 0.8f;

	//이 접근속도(cm/s)에서 쉐이크가 최대 세기에 도달 (MinBumpSpeed~이 값 사이를 보간)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BumpShakeFullSpeed = 900.f;

	//---------- 충돌 넉백 + 리액션(몸통 들썩·기울임) ----------
	//부스트로 상대 카트를 박았을 때 상대를 밀어내는 세기 (부스트 비비기 방지). LaunchCharacter 속도
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float BoostKnockbackStrength = 2500.f;

	//일반 충돌(부스트 아님) 시 상대를 밀어내는 세기 = 접근속도 × 이 배율 (박힌 쪽이 확실히 밀리게 — 3주차 피드백 2배 강화)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float NormalKnockbackScale = 1.1f;

	//일반 충돌 넉백 상한 (접근속도가 커도 이 이상은 안 밀림) — 수직 띄움이 체공으로 수평 거리를 늘려서 상한은 보수적으로
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float NormalKnockbackMax = 1000.f;

	//넉백 시 위로 띄우는 비율 (수직속도 = 넉백 세기 × 이 값) — 박힌 쪽이 붕 뜨는 맛
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float KnockbackUpRatio = 0.25f;

	//넉백 수직 속도 상한 (너무 높이 뜨는 것 방지)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "0"))
	float KnockbackUpMax = 450.f;

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

	//충돌(범프) 시 효과음 (비어있으면 무음)
	UPROPERTY(EditAnywhere, Category = "Cart|SFX")
	TObjectPtr<USoundBase> BumpSound;

private:
	//소유 카트 캐시 (BeginPlay 1회)
	UPROPERTY(Transient)
	TObjectPtr<ACartPawn> OwnerCart;

	//마지막으로 스필(드롭)을 요청한 시각 — BumpDropCooldown 공통 적용
	float LastBumpDropTime = -1000.f;

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

	//---------- 필살기 게이지 ----------
	//충돌마다 가해자에게 +1, 상한 도달 시 발동 가능. 소유 클라 UI용 복제(OwnerOnly)
	UPROPERTY(ReplicatedUsing = OnRep_UltimateStack)
	int32 UltimateStack = 0;

	//게이지 상한 = 발동 필요량 (CartPawn.UltimateRequiredStack과 일치시킬 것)
	UPROPERTY(EditAnywhere, Category = "Cart|Bump", meta = (ClampMin = "1"))
	int32 UltimateMaxStack = 5;

	UFUNCTION()
	void OnRep_UltimateStack();
};
