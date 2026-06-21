#pragma once

#include "CoreMinimal.h"
#include "CheckoutTypes.generated.h"


UENUM(BlueprintType)
enum class ECheckoutZoneState : uint8
{
    None,
    Open,           // 계산대 열림
    ClosingSoon,    // 계산대 마감 임박
    Closed          // 계산대 닫힘
};
