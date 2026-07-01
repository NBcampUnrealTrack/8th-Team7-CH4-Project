// BumperCart - B(카트/플레이어 조작) 파트
// 부스트 중 화면 가장자리 스피드라인 (로컬 플레이어 전용) 연출 컴포넌트

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CartScreenFXComponent.generated.h"

class ACartPawn;
class UCameraComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

//부스트 중 화면 가장자리 스피드라인(로컬 화면 전용).
//카메라 PostProcess에 머티리얼을 얹고, 부스트 상태에 따라 켜고 끈다.
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BUMPERCART_API UCartScreenFXComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCartScreenFXComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//화면 스피드라인 PP 머티리얼 (Material Domain = Post Process). 비어있으면 아무 동작도 하지 않음.
	UPROPERTY(EditAnywhere, Category = "Cart|ScreenFX")
	TObjectPtr<UMaterialInterface> SpeedLineMaterial;

	//세기 0<->1 보간 속도
	UPROPERTY(EditAnywhere, Category = "Cart|ScreenFX", meta = (ClampMin = "0.1"))
	float IntensityInterpSpeed = 8.f;

private:
	//로컬 카메라를 찾아 MID를 PostProcess 블렌더블로 얹는다 (최초 1회). 성공 시 true
	bool EnsureSpeedLineMID();

	//소유 카트 (부스트 상태·로컬 판정 참조용). BeginPlay에서 캐시
	UPROPERTY(Transient)
	TObjectPtr<ACartPawn> OwnerCart;

	//런타임 생성 머티리얼 인스턴스 (Intensity 파라미터를 매 프레임 구동)
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SpeedLineMID;

	//현재 화면 세기 (목표값을 향해 보간되는 실제 적용값)
	float CurrentIntensity = 0.f;
};
