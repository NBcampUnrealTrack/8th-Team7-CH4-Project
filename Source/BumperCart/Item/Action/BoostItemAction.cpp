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

    UE_LOG(LogTemp, Warning, TEXT("%s: 부스트 아이템 사용"), *GetNameSafe(PlayerCharacter));

    return true;
}
