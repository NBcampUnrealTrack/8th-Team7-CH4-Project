// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Projectile/GloveProjectile.h"

AGloveProjectile::AGloveProjectile()
{
    Impulse = 300.f;
}

void AGloveProjectile::OnHitCart(ACartPawn* HitPlayer)
{
    // HitPlayer가 아이템 떨어뜨리도록

    UE_LOG(LogTemp, Warning, TEXT("글러브 아이템 충돌!"));
}
