// Fill out your copyright notice in the Description page of Project Settings.


#include "CartLoadComponent.h"
#include "Product/ProductBase.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "NiagaraFunctionLibrary.h"


UCartLoadComponent::UCartLoadComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    SetIsReplicatedByDefault(true);

    MaxWeight = 20;
    WeightScaling = 0.5f;

    BoosterInstigatorDropMultiplier = 0.4f;
    BoostedTargetDropMultiplier = 1.4f;

    // 드롭 개수 규칙 지정
    DropCountRules.Add({ 300.f, 0, 1, 0.5f });
    DropCountRules.Add({ 700.f, 1, 2, 1.f });
    DropCountRules.Add({ 1200.f, 2, 3, 1.f });
    DropCountRules.Add({ 1800.f, 3, 4, 1.f });
    DropCountRules.Add({ 2500.f, 4, 5, 1.f });

    // 소켓 이름 지정
    for (int32 i = 1; i <= 6; ++i)
    {
        FName SocketName = *FString::Printf(TEXT("LoadDummy_0%d"), i);
        LoadDummySocketNames.Add(SocketName);
    }

    Initialize();
}

void UCartLoadComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetNetMode() != NM_DedicatedServer)
    {
        CreateDummyMeshes();
    }
}

void UCartLoadComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, LoadInfo);
}

void UCartLoadComponent::Initialize()
{
    LoadInfo.CurrentLoadedCount = 0;
    LoadInfo.CurrentWeight = 0;

    LoadedProducts.Reset();
}

bool UCartLoadComponent::TryAddProduct(AProductBase* Product)
{
    if (!IsValid(Product)) return false;

    // Owner 확인, Owner 권한 확인
    if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority()) return false;

    // 이전 더미 메시 개수
    int32 PrevDummyCount = GetVisibleDummyCount();

    // Loaded 상태로 변경가능한지 확인하면서 변경
    if (!Product->TrySetLoaded()) return false;

    // 배열에 추가
    LoadedProducts.Add(Product);

    // 적재 정보 갱신
    UpdateLoadInfo();

    // 더미 메시 위치에 이펙트 출력, 이전 더미 메시 개수와 비교
    int32 DummyCount = GetVisibleDummyCount();
    int32 DummyIndex = DummyCount > PrevDummyCount ? DummyCount - 1 : INDEX_NONE;

    Multicast_PlayCartLoadEffect(DummyIndex);

    return true;
}

void UCartLoadComponent::RequestDropProduct(float Impulse, EDropCollisionRole Role)
{
    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor)) return;

    // 서버라면 즉시 실행
    if (OwnerActor->HasAuthority())
    {
        DropProducts(Impulse, Role);
        return;
    }

    // 클라이언트면 서버로 요청
    Server_RequestDropProducts(Impulse, Role);
}

void UCartLoadComponent::DropProducts(float Impulse, EDropCollisionRole Role)
{
    if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority()) return;
    if (LoadedProducts.IsEmpty()) return;


    // 충격량만큼 떨어뜨릴 개수 판정
    int32 DropCount = CalculateDropCount(Impulse, Role);
    int32 ActualDropCount = FMath::Min(DropCount, LoadedProducts.Num());
    if (ActualDropCount <= 0) return;

    // 현재 적재중인 상품에서 랜덤 인덱스 선택
    // 적재 상품 배열에서 제거, Falling 상태로 전환
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

int32 UCartLoadComponent::GetCurrentLoadedCount() const
{
    return LoadInfo.CurrentLoadedCount;
}

float UCartLoadComponent::GetLoadRatio() const
{
    return MaxWeight > 0 ? (float)LoadInfo.CurrentWeight / MaxWeight : 0.f;
}

bool UCartLoadComponent::CheckoutProducts(TArray<FLoadedProductInfo>& OutProducts)
{
    OutProducts.Empty();

    if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority()) return false;
    if (LoadedProducts.IsEmpty()) return false;

    // 정산에 필요한 데이터를 Out 배열에 넣고, 정산 상태로 변경한뒤 삭제
    for (int32 i = LoadedProducts.Num() - 1; i >= 0; --i)
    {
        if (!IsValid(LoadedProducts[i])) continue;

        OutProducts.Add(LoadedProducts[i]->GetLoadedProductInfo());

        LoadedProducts[i]->SetProductState(EProductState::Paid);

        LoadedProducts[i]->SetLifeSpan(2.f);
    }

    LoadedProducts.Empty();

    UpdateLoadInfo();

    return OutProducts.Num() > 0;
}

int32 UCartLoadComponent::CalculateDropCount(float Impulse, EDropCollisionRole Role) const
{
    if (LoadedProducts.IsEmpty()) return 0;
    if (MaxWeight <= 0) return 0;

    // 적재 비율 확인
    float LoadRatio = FMath::Clamp(static_cast<float>(LoadInfo.CurrentWeight) / MaxWeight, 0.f, 1.f);

    // 무게 보정치, 부스터 사용에따른 보정치
    float WeightMultiplier = 1.f + LoadRatio * WeightScaling;
    float RoleMultiplier = GetCollisionRoleMultiplier(Role);

    float FinalImpulse = Impulse * WeightMultiplier * RoleMultiplier;

    // 어느 구간인지 확인
    const FDropCountRule* SelectedRule = nullptr;
    for (const FDropCountRule& Rule : DropCountRules)
    {
        // 요구 충격량을 만족하면
        if (FinalImpulse >= Rule.RequiredImpulse)
        {
            // 현재 요구 충격량이 기존보다 클 경우에만 선택
            // 오름차순으로 정렬을 보장하면 필요없음
            if (!SelectedRule || Rule.RequiredImpulse > SelectedRule->RequiredImpulse)
            {
                SelectedRule = &Rule;
            }
        }
    }

    // 어느 구간에도 속하지 않으면 충격량이 낮으므로 0개 떨어뜨리도록
    if (!SelectedRule)
    {
        return 0;
    }

    // 1 - DropChance 확률로 안떨어뜨림
    if (FMath::FRand() > SelectedRule->DropChance)
    {
        return 0;
    }

    int32 MinCount = FMath::Max(0, SelectedRule->MinDropCount);
    int32 MaxCount = FMath::Max(MinCount, SelectedRule->MaxDropCount);

    return FMath::RandRange(MinCount, MaxCount);
}

float UCartLoadComponent::GetCollisionRoleMultiplier(EDropCollisionRole Role) const
{
    switch (Role)
    {
    case EDropCollisionRole::BoosterInstigator:
        return BoosterInstigatorDropMultiplier;

    case EDropCollisionRole::BoostedTarget:
        return BoostedTargetDropMultiplier;

    case EDropCollisionRole::Normal:    // FallThrough
    default:
        return 1.f;
    }
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

void UCartLoadComponent::CreateDummyMeshes()
{
    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor)) return;

    // 스태틱 메시 전부 가져오기
    TArray<UStaticMeshComponent*> MeshComponents;
    OwnerActor->GetComponents<UStaticMeshComponent>(MeshComponents);

    if (LoadDummySocketNames.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("더미 메시가 부착될 소켓을 지정해주세요!!"));
        return;
    }
    FName DummySocketName = LoadDummySocketNames[0];

    // 지정한 소켓 이름이 있는 카트면 가져오고 없다면 넘기기
    UStaticMeshComponent* CartMesh = nullptr;
    for (UStaticMeshComponent* MeshComp : MeshComponents)
    {
        if (IsValid(MeshComp) && MeshComp->DoesSocketExist(DummySocketName))
        {
            CartMesh = MeshComp;
        }
    }
    if (!IsValid(CartMesh)) return;

    for (const FName& SocketName : LoadDummySocketNames)
    {
        UStaticMeshComponent* Dummy = NewObject<UStaticMeshComponent>(OwnerActor, SocketName);
        Dummy->SetStaticMesh(LoadDummyMesh);
        Dummy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Dummy->SetVisibility(false);
        Dummy->SetupAttachment(CartMesh, SocketName);

        OwnerActor->AddInstanceComponent(Dummy);
        Dummy->RegisterComponent();

        LoadDummyMeshes.Add(Dummy);
    }

    UpdateLoadVisual();
}

void UCartLoadComponent::UpdateLoadVisual()
{
    int32 VisibleCount = GetVisibleDummyCount();

    // VisibleCount보다 i가 작으면 보여주고, 넘으면 가리기
    for (int32 i = 0; i < LoadDummyMeshes.Num(); ++i)
    {
        UStaticMeshComponent* Dummy = LoadDummyMeshes[i];
        if (IsValid(Dummy))
        {
            Dummy->SetVisibility(i < VisibleCount);
        }
    }
}

int32 UCartLoadComponent::GetVisibleDummyCount() const
{
    if (MaxWeight <= 0)
    {
        return 0;
    }

    float LoadRatio = static_cast<float>(LoadInfo.CurrentWeight) / MaxWeight;
    LoadRatio = FMath::Clamp(LoadRatio, 0.f, 1.f);

    if (LoadRatio < KINDA_SMALL_NUMBER)
    {
        return 0;
    }
    else if (LoadRatio < 0.1f)
    {
        return 1;
    }
    else if (LoadRatio < 0.3f)
    {
        return 2;
    }
    else if (LoadRatio < 0.5f)
    {
        return 3;
    }
    else if (LoadRatio < 0.7f)
    {
        return 4;
    }
    else if (LoadRatio < 0.9f)
    {
        return 5;
    }

    return 6;
}

void UCartLoadComponent::Multicast_PlayCartLoadEffect_Implementation(int32 DummyIndex)
{
    if (GetNetMode() == NM_DedicatedServer) return;

    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor)) return;

    FVector EffectLocation = OwnerActor->GetActorLocation();

    if (LoadDummyMeshes.IsValidIndex(DummyIndex) && IsValid(LoadDummyMeshes[DummyIndex]))
    {
        EffectLocation = LoadDummyMeshes[DummyIndex]->GetComponentLocation();
    }

    if (LoadEffect)
    {
        UWorld* World = GetWorld();
        if (!IsValid(World)) return;

        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            World,
            LoadEffect,
            EffectLocation,
            FRotator::ZeroRotator,
            FVector::OneVector,
            true
        );
    }
}

void UCartLoadComponent::Server_RequestDropProducts_Implementation(float Impulse, EDropCollisionRole Role)
{
    DropProducts(Impulse, Role);
}

void UCartLoadComponent::OnRep_LoadInfo()
{
    if (!IsValid(GetOwner())) return;

    // 더미 메시 보여주기
    UpdateLoadVisual();

    OnLoadInfoChanged.Broadcast(GetOwner(), LoadInfo);
}
