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

    return PlayerCharacter->ActivateBoostFromItem();
}
