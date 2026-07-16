// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUDScoreboardRowWidget.h"

int32 UHUDScoreboardRowWidget::GetBaseIndex() const
{
    return BaseIndex;
}

void UHUDScoreboardRowWidget::SetBaseIndex(int32 NewIndex)
{
    BaseIndex = NewIndex;
}

bool UHUDScoreboardRowWidget::PrepareMove(float NewTargetY)
{
    MoveStartY = GetRenderTransform().Translation.Y;
    MoveTargetY = NewTargetY;
    return !FMath::IsNearlyEqual(MoveStartY, MoveTargetY, 0.1f);
}

void UHUDScoreboardRowWidget::ApplyMove(float Alpha)
{
    FWidgetTransform Transform = GetRenderTransform();
    Transform.Translation.Y = FMath::Lerp(MoveStartY, MoveTargetY, Alpha);
    SetRenderTransform(Transform);
}

void UHUDScoreboardRowWidget::SnapToTarget()
{
    ApplyMove(1.f);
}
