#include "Item/Action/BoostItemAction.h"

#include "Cart/CartPawn.h"

bool UBoostItemAction::CanExecute(ACartPawn* PlayerCharacter) const
{
    return IsValid(PlayerCharacter);
}

bool UBoostItemAction::Execute(ACartPawn* PlayerCharacter)
{
    if (!IsValid(PlayerCharacter))
    {
        return false;
    }

    if (!PlayerCharacter->HasAuthority())
    {
        return false;
    }

    //부스터 상시화로 아이템 사용 없음 — 클래스/DA_Boost 정리 예정
    return false;
}
