#include "Checkout/CheckoutManager.h"

#include "Checkout/CheckoutZone.h"
#include "Checkout/CheckoutTypes.h"

ACheckoutManager::ACheckoutManager()
{
 	PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;

}

void ACheckoutManager::BeginPlay()
{
	Super::BeginPlay();

    // 서버에서만 수행
    if (!HasAuthority())
    {
        return;
    }

    // 계산대 초기 세팅
    if (!InitializeCheckoutZones())
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("계산대 수: %d"), CheckoutZones.Num());

    for (ACheckoutZone* CheckoutZone : CheckoutZones)
    {
        UE_LOG(LogTemp, Warning, TEXT("계산대 ID: %d"), CheckoutZone->GetCheckoutZoneID());
    }
}

// ------------------------------------------------------------
// 계산대 목록
// ------------------------------------------------------------

bool ACheckoutManager::InitializeCheckoutZones()
{
    // 월드에 계산대가 하나라도 존재하는지
    if (CheckoutZones.IsEmpty())
    {
        return false;
    }

    // 각 계산대가 모두 유효한지
    for (ACheckoutZone* CheckoutZone : CheckoutZones)
    {
        if (!IsValid(CheckoutZone))
        {
            return false;
        }
    }

    // 게임 시작 시, 모든 계산대 오픈
    for (ACheckoutZone* CheckoutZone : CheckoutZones)
    {
        CheckoutZone->SetCheckoutZoneState(ECheckoutZoneState::Open);
    }

    return true;
}


