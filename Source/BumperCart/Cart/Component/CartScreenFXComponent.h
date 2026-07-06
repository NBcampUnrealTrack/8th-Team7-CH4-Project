// BumperCart - B(카트/플레이어 조작) 파트
// 카트 연출(FX) 컴포넌트 — 화면 가장자리 스피드라인(PP, 로컬 전용) + 바퀴 월드 FX(바닥 리본·브레이크 스파크)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CartScreenFXComponent.generated.h"

class ACartPawn;
class UCameraComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UNiagaraSystem;
class UStaticMeshComponent;
class UUserWidget;

//카트의 시각 연출을 전담하는 컴포넌트.
//화면: 부스트 중 가장자리 스피드라인(PostProcess MID, 내 화면만)
//월드: 뒷바퀴 소켓에 부착한 바닥 리본(속도감)·브레이크 스파크 (모든 클라 표시)
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BUMPERCART_API UCartScreenFXComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCartScreenFXComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	//---------- 토마토 화면 가림 ----------
	//토마토에 맞으면 화면을 가리는 위젯을 Duration초간 표시 (로컬 플레이어만)
	void ApplyTomatoScreenBlock(float Duration);
	//토마토 화면 가림 위젯 제거
	void RemoveTomatoScreenBlock();

protected:

	//---------- 화면 스피드라인 (부스트, 로컬 화면 전용) ----------
	//화면 스피드라인 PP 머티리얼 (Material Domain = Post Process). 비어있으면 아무 동작도 하지 않음.
	UPROPERTY(EditAnywhere, Category = "Cart|ScreenFX")
	TObjectPtr<UMaterialInterface> SpeedLineMaterial;

	//세기 0<->1 보간 속도
	UPROPERTY(EditAnywhere, Category = "Cart|ScreenFX", meta = (ClampMin = "0.1"))
	float IntensityInterpSpeed = 8.f;

	//---------- 바퀴 월드 FX (뒷바퀴 소켓 부착) ----------
	//바닥 리본 나이아가라 (BP에서 NS_CartSpeedLines 지정. 비어있으면 무동작)
	UPROPERTY(EditAnywhere, Category = "Cart|WheelFX")
	TObjectPtr<UNiagaraSystem> GroundRibbonSystem;

	//브레이크 스파크 나이아가라 (BP에서 NS_BrakeSparks 지정. 비어있으면 무동작)
	UPROPERTY(EditAnywhere, Category = "Cart|WheelFX")
	TObjectPtr<UNiagaraSystem> BrakeSparkSystem;

	//카트 메시(SM_Shopping_Cart)에 추가한 뒷바퀴 소켓 이름 — 리본·스파크가 공유
	UPROPERTY(EditAnywhere, Category = "Cart|WheelFX")
	FName RearWheelSocketLeft = TEXT("RearWheel_L");
	UPROPERTY(EditAnywhere, Category = "Cart|WheelFX")
	FName RearWheelSocketRight = TEXT("RearWheel_R");

	//이 전진 속도(cm/s) 넘으면 리본 ON, 미만이면 OFF (계단식 게이트)
	UPROPERTY(EditAnywhere, Category = "Cart|WheelFX", meta = (ClampMin = "0"))
	float SpeedLineMinSpeed = 500.f;

	//브레이크 중 이 전진 속도(cm/s) 이상일 때만 스파크 (저속 브레이크는 불꽃 없음)
	UPROPERTY(EditAnywhere, Category = "Cart|WheelFX", meta = (ClampMin = "0"))
	float BrakeSparkMinSpeed = 250.f;

	//브레이크를 이 시간(초) 유지하면 히트 최대 — Niagara User.BrakeHeat(0~1)로 전달되어 스파크 색이 주황->하양으로 달아오름
	UPROPERTY(EditAnywhere, Category = "Cart|WheelFX", meta = (ClampMin = "0.1"))
	float BrakeHeatRampTime = 1.5f;

private:
	//로컬 카메라를 찾아 MID를 PostProcess 블렌더블로 얹는다 (최초 1회). 성공 시 true
	bool EnsureSpeedLineMID();

	//뒷바퀴 소켓을 가진 카트 메시를 찾아 리본·스파크 컴포넌트를 생성/부착 (BeginPlay)
	void SetupWheelFX();

	//나이아가라 컴포넌트 하나를 생성해 부착. 소켓이 없을 땐 FallbackOffset 위치로
	UNiagaraComponent* SpawnWheelFX(UNiagaraSystem* System, USceneComponent* AttachTo, FName Socket, const FVector& FallbackOffset, bool bStartActive);

	//리본 파라미터 갱신 + 브레이크 스파크 on/off·히트 누적 (매 프레임)
	void DriveWheelFX(float ForwardSpeed, float DeltaTime);

	//소유 카트 (부스트·브레이크·적재율 참조용). BeginPlay에서 캐시
	UPROPERTY(Transient)
	TObjectPtr<ACartPawn> OwnerCart;

	//런타임 생성 머티리얼 인스턴스 (Intensity 파라미터를 매 프레임 구동)
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SpeedLineMID;

	//현재 화면 세기 (목표값을 향해 보간되는 실제 적용값)
	float CurrentIntensity = 0.f;

	//런타임 생성 바퀴 FX (BeginPlay에서 소켓에 부착)
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> RibbonFXLeft;
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> RibbonFXRight;
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> SparkFXLeft;
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> SparkFXRight;

	//스파크 현재 활성 상태 (상태 바뀔 때만 Activate/Deactivate)
	bool bSparksActive = false;

	//브레이크 히트 누적(0~1). 밟는 동안 차오르고, 떼면 2배 속도로 식음
	float BrakeHeat = 0.f;

	//---------- 토마토 화면 가림 위젯 ----------
	//토마토 위젯 클래스 (BP에서 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Cart|ScreenFX|Tomato")
	TSubclassOf<UUserWidget> TomatoScreenBlockWidgetClass;

	//토마토 얼룩 위에서 보이도록 Z오더
	UPROPERTY(EditDefaultsOnly, Category = "Cart|ScreenFX|Tomato")
	int32 TomatoScreenBlockZOrder = 100;

	//현재 화면에 떠 있는 토마토 위젯
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> TomatoScreenBlockWidget;

	//토마토 위젯 제거 타이머
	FTimerHandle TomatoScreenBlockTimerHandle;
};
