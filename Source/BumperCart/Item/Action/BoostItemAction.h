#pragma once

#include "CoreMinimal.h"
#include "Item/Action/ItemAction.h"
#include "BoostItemAction.generated.h"

UCLASS()
class BUMPERCART_API UBoostItemAction : public UItemAction
{
	GENERATED_BODY()

public:
    // 아이템 사용 가능한지 검사
    virtual bool CanExecute(ACartPawn* PlayerCharacter) const override;
    // 아이템 사용
    virtual bool Execute(ACartPawn* PlayerCharacter) override;
};
