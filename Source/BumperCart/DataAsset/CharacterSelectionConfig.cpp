#include "CharacterSelectionConfig.h"

FLinearColor UCharacterSelectionConfig::GetColor(int32 Index) const
{
    if (CharacterDatas.IsValidIndex(Index))
    {
        return CharacterDatas[Index].Color;
    }
    return FLinearColor::White;
}
