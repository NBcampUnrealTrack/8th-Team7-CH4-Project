#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemAction.generated.h"

class ACartPawn;

UCLASS(Abstract, Blueprintable)
class BUMPERCART_API UItemAction : public UObject
{
	GENERATED_BODY()

public:
    // 아이템 사용 가능한지 검사
    virtual bool CanExecute(ACartPawn* PlayerCharacter) const;
    // 아이템 사용
    virtual bool Execute(ACartPawn* PlayerCharacter);
};
