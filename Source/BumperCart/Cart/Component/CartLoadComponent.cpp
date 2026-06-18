// Fill out your copyright notice in the Description page of Project Settings.


#include "CartLoadComponent.h"
#include "Product/ProductBase.h"
#include "Net/UnrealNetwork.h"


UCartLoadComponent::UCartLoadComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    SetIsReplicatedByDefault(true);

    Initialize();
}

void UCartLoadComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UCartLoadComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, LoadInfo);
}

void UCartLoadComponent::Initialize()
{
    MaxLoadedCount = 10;
    MaxWeight = 20;

    LoadInfo.CurrentLoadedCount = 0;
    LoadInfo.CurrentWeight = 0;

    LoadedProducts.Empty();
}

bool UCartLoadComponent::TryAddProduct(AProductBase* Product)
{
    if (!IsValid(Product)) return false;

    // Owner 확인, Owner 권한 확인
    if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority()) return false;

    // 상품이 진열 상태인지 확인, 무게 확인, 적재 개수 확인
    if (Product->GetProductState() != EProductState::Display) return false;
    if (LoadInfo.CurrentWeight + Product->GetWeight() > MaxWeight) return false;
    if (LoadInfo.CurrentLoadedCount >= MaxLoadedCount) return false;

    // Loaded 상태로 변경가능한지 확인하면서 변경
    if (!Product->TrySetLoaded()) return false;

    // 배열에 추가
    LoadedProducts.Add(Product);

    // 적재 정보 갱신
    UpdateLoadInfo();

    Product->AttachToActor(GetOwner(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);

    return true;
}

void UCartLoadComponent::DropProducts(float Impulse)
{
    if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority()) return;
    if (LoadedProducts.IsEmpty()) return;


    // 충격량만큼 떨어뜨릴 개수 판정
    int32 DropCount = CalculateDropCount(Impulse);
    if (DropCount <= 0) return;

    // 현재 적재중인 상품에서 랜덤 인덱스 선택
    // 적재 상품 배열에서 제거, Falling 상태로 전환
    int32 ActualDropCount = FMath::Min(DropCount, LoadedProducts.Num());
    for (int32 i = 0; i < ActualDropCount; ++i)
    {
        int32 RandomIndex = FMath::RandRange(0, LoadedProducts.Num() - 1);
        AProductBase* DroppedProduct = LoadedProducts[RandomIndex];

        LoadedProducts.RemoveAtSwap(RandomIndex, EAllowShrinking::No);

        if (!IsValid(DroppedProduct)) continue;

        DroppedProduct->DropFromCart(GetOwner());
    }
    UpdateLoadInfo();
}

int32 UCartLoadComponent::GetTotalValue() const
{
    if (LoadInfo.CurrentLoadedCount == 0) return 0;

    int32 TotalValue = 0;
    for (const AProductBase* Product : LoadedProducts)
    {
        if (IsValid(Product))
        {
            TotalValue += Product->GetValue();
        }
    }

    return TotalValue;
}

void UCartLoadComponent::ClearProducts()
{
    if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority()) return;

    for (int32 i = LoadedProducts.Num() - 1; i >= 0; --i)
    {
        if (IsValid(LoadedProducts[i]))
        {
            LoadedProducts[i]->Destroy();
        }
    }

    LoadedProducts.Empty();

    UpdateLoadInfo();
}

int32 UCartLoadComponent::CalculateDropCount(float Impulse) const
{
    // 적재량에 따라 보정 필요함

    // 최소 기준값
    if (Impulse < 300.f)
    {
        return 0;
    }

    if (Impulse < 700.f)
    {
        return FMath::RandRange(0, 1);
    }

    if (Impulse < 1200.f)
    {
        return FMath::RandRange(1, 2);
    }

    return FMath::RandRange(2, 3);
}

void UCartLoadComponent::UpdateLoadInfo()
{
    if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority()) return;

    int32 TotalWeight = 0;
    int32 Count = 0;

    for (AProductBase* Product : LoadedProducts)
    {
        if (IsValid(Product))
        {
            TotalWeight += Product->GetWeight();
            ++Count;
        }
    }

    LoadInfo.CurrentLoadedCount = Count;
    LoadInfo.CurrentWeight = TotalWeight;

    // 리슨 서버면 본인도 UI 갱신
    if (GetNetMode() != NM_DedicatedServer)
    {
        OnRep_LoadInfo();
    }
}

void UCartLoadComponent::OnRep_LoadInfo()
{
    if (!IsValid(GetOwner())) return;

    OnLoadInfoChanged.Broadcast(GetOwner(), LoadInfo);
}
