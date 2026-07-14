#include "Item/Projectile/TomatoProjectile.h"

#include "Cart/CartPawn.h"
#include "Cart/CartLoadTypes.h"
#include "Cart/Component/CartLoadComponent.h"


void ATomatoProjectile::OnHitCart(ACartPawn* HitPlayer)
{
    // 토마토 화면 가림 적용
    HitPlayer->ClientApplyTomatoScreenBlock(ScreenBlockDuration);

    //UCartLoadComponent* LoadComponent = HitPlayer->GetLoadComponent();
    //if (IsValid(LoadComponent))
    //{
    //    LoadComponent->DropProducts(ProductDropStrength, EDropCollisionRole::Normal);
    //}
}
