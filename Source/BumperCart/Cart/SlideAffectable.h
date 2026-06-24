//BumperCart - B(카트/플레이어 조작) 파트
//ISlideAffectable: 맵 기믹(물웅덩이 등)이 "미끄러져라"라고 알릴 때 쓰는 인터페이스.
//실제 미끄럼 처리는 이 인터페이스를 구현한 액터(카트)가 자기 이동모델 안에서 한다.
//=> 기믹이 카트 내부(CharacterMovement)를 직접 건드리지 않게 해서 충돌을 막는다.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SlideAffectable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class USlideAffectable : public UInterface
{
	GENERATED_BODY()
};

class ISlideAffectable
{
	GENERATED_BODY()

public:
	//Duration초 동안 미끄럼 + SpinAngleDeg만큼 강제 스핀. (권장: 서버에서 호출)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Slip")
	void ApplySlip(float Duration, float SpinAngleDeg);
};
