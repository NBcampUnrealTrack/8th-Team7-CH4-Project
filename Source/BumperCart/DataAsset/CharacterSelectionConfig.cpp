#include "CharacterSelectionConfig.h"

TSubclassOf<APawn> UCharacterSelectionConfig::GetPawnClass(int32 Index) const
{
    if (CharacterDatas.IsValidIndex(Index))
    {
        return CharacterDatas[Index].PawnClass;
    }
    return nullptr;
}

FLinearColor UCharacterSelectionConfig::GetColor(int32 Index) const
{
    if (CharacterDatas.IsValidIndex(Index))
    {
        return CharacterDatas[Index].Color;
    }
    return FLinearColor::White;
}
