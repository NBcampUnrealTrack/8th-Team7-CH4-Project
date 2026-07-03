#include "Item/Action/ItemAction.h"

#include "Cart/CartPawn.h"

bool UItemAction::CanExecute(ACartPawn* PlayerCharacter) const
{
    return IsValid(PlayerCharacter);
}

bool UItemAction::Execute(ACartPawn* PlayerCharacter)
{
    return false;
}
