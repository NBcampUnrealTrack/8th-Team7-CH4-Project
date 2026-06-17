#pragma once

#include "CoreMinimal.h"
#include "CheckoutTypes.generated.h"


UENUM(BlueprintType)
enum class ECounterState : uint8
{
    OPEN,           // 계산대 열림
    CLOSING_SOON,   // 계산대 마감 임박
    CLOSED          // 계산대 닫힘
};
