#pragma once

#include "CoreMinimal.h"
#include "ProductShelfTypes.generated.h" 

UENUM(BlueprintType)
enum class EShelfType : uint8
{
    Normal,
    Sale,
    Limited
};
