#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Bumpable.generated.h"

//카트가 부딪히면 상품을 떨구는 '충돌 대상' 마커 인터페이스.
//이 인터페이스를 구현한 액터에
//카트가 부딪히면 충돌 연출(쉐이크·사운드) + 상품 드롭이 발생한다.
//구현할 메서드 없음. C++은 `public IBumpable`만, BP는 Class Settings에서 인터페이스 추가만 하면 적용됨.
UINTERFACE(MinimalAPI, Blueprintable)
class UBumpable : public UInterface
{
	GENERATED_BODY()
};

class IBumpable
{
	GENERATED_BODY()
};
