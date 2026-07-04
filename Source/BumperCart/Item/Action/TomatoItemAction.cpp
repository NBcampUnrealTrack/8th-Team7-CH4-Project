#include "Item/Action/TomatoItemAction.h"

#include "Cart/CartPawn.h"

bool UTomatoItemAction::CanExecute(ACartPawn* PlayerCharacter) const
{
    return IsValid(PlayerCharacter);
}

bool UTomatoItemAction::Execute(ACartPawn* PlayerCharacter)
{
    if (!IsValid(PlayerCharacter))
    {
        return false;
    }

    if (!PlayerCharacter->HasAuthority())
    {
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("%s: 토마토 아이템 사용"), *GetNameSafe(PlayerCharacter));

    return true;
}
