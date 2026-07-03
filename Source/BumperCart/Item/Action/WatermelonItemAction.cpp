#include "Item/Action/WatermelonItemAction.h"

#include "Cart/CartPawn.h"

bool UWatermelonItemAction::CanExecute(ACartPawn* PlayerCharacter) const
{
    return IsValid(PlayerCharacter);
}

bool UWatermelonItemAction::Execute(ACartPawn* PlayerCharacter)
{
    if (!IsValid(PlayerCharacter))
    {
        return false;
    }

    if (!PlayerCharacter->HasAuthority())
    {
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("%s: 수박 아이템 사용"), *GetNameSafe(PlayerCharacter));

    return true;
}
