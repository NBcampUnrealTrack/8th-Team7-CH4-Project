#include "ProductShelfSubsystem/ProductShelfSubsystem.h"

#include "ItemSpawn/ProductShelf/ProductShelf.h"
#include "Kismet/GameplayStatics.h"
#include "EventManager/BC_EventManager.h"
#include "ItemSpawn/ProductShelfManager/ProductShelfManagerConfig.h"

void UProductShelfSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UProductShelfSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UProductShelfSubsystem::RegisterShelf(AProductShelf* InShelf, EShelfType InType)
{
    if (!IsValid(InShelf))    return;

    switch (InType)
    {
    case EShelfType::Normal:
        NormalProductShelfs.Add(InShelf);
        break;
    case EShelfType::Sale:
        SaleProductShelfs.Add(InShelf);
        break;
    case EShelfType::Limited:
        LimitedProductShelfs.Add(InShelf);
        break;
    }

    FString NetModeStr = (GetWorld() && GetWorld()->GetNetMode() != NM_Client) ? TEXT("서버") : TEXT("클라이언트");
    UE_LOG(LogTemp, Log, TEXT("[%s][제품 선반 매니저] 선반 등록 (종류: %d, 일반: %d, 세일: %d, 한정: %d)"),
        *NetModeStr, (int32)InType, NormalProductShelfs.Num(), SaleProductShelfs.Num(), LimitedProductShelfs.Num());
}

void UProductShelfSubsystem::InitializeConfig(UProductShelfManagerConfig* InConfig)
{
    if (IsValid(InConfig))
    {
        SpawnConfig = InConfig;

        MaxSpawnCount = SpawnConfig->MaxSpawnCount;
        MaxSpawnLimit = SpawnConfig->MaxSpawnLimit;
        RespawnDelay = SpawnConfig->RespawnDelay;
        SpawnFX = SpawnConfig->SpawnFX;

        FString NetModeStr = (GetWorld() && GetWorld()->GetNetMode() != NM_Client) ? TEXT("서버") : TEXT("클라이언트");
        UE_LOG(LogTemp, Log, TEXT("[%s][선반매니저] 데이터 에셋 적용"), *NetModeStr);

        // 서버에서만
        //if (GetWorld() && GetWorld()->GetNetMode() != NM_Client)
        //{
        //    // 게임 모드에서 호출시 삭제 예정 - 테스트용
        //    GetWorld()->GetTimerManager().SetTimer(GameStartTimerHandle, this, &UProductShelfSubsystem::StartProductSpawning, RespawnDelay, false);
        //    UE_LOG(LogTemp, Log, TEXT("[선반매니저] 아이템 스폰"));
        //}
    }
}

void UProductShelfSubsystem::ProductSpawnCall()
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) return;

    // 선반 목록 섞기
    for (int32 i = NormalProductShelfs.Num() - 1; i > 0; --i)
    {
        int32 RandomIndex = FMath::RandRange(0, i);
        if (i != RandomIndex)
        {
            NormalProductShelfs.Swap(i, RandomIndex);
        }
    }

    // 선반 목록 순회하면 스폰호출
    for (AProductShelf* Shelf : NormalProductShelfs)
    {
        if (IsValid(Shelf))
        {
            int32 RandomCount = FMath::RandRange(1, MaxSpawnLimit);

            for (int32 i = 0; i < RandomCount; i++)
            {
                if (CurrentProductCount > MaxSpawnCount)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[제품선반 서브시스템] 맵의 제품이 최대로 스폰되었습니다."));
                    return;
                }

                AProductBase* SpawnedProduct = Shelf->SpawnRandomProduct();

                // 스폰 성공
                if (IsValid(SpawnedProduct))
                {
                    CurrentProductCount++;
                }
            }
        }
    }
    UE_LOG(LogTemp, Log, TEXT("[제품선반 서브시스템] 스폰된 제품 수 : %d"), CurrentProductCount);
}

void UProductShelfSubsystem::OnProductDestroyed()
{
    if (CurrentProductCount > 0)
    {
        CurrentProductCount--;
    }
}

void UProductShelfSubsystem::StartProductSpawning()
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) return;

    GetWorld()->GetTimerManager().SetTimer(RespawnTimerHandle, this, &UProductShelfSubsystem::ProductSpawnCall, RespawnDelay, true, 0.0f);
}

void UProductShelfSubsystem::SaleProductSpawn(TSubclassOf<AProductBase> SaleProduct)
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) return;

    if(!IsValid(SaleProduct)) return;

    if (SaleProductShelfs.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[제품선반 서브시스템] 세일 선반이 없습니다!"));
        return;
    }

    // 세일 선반 섞기
    for (int32 i = SaleProductShelfs.Num() - 1; i > 0; --i)
    {
        int32 RandomIndex = FMath::RandRange(0, i);
        if (i != RandomIndex)
        {
            SaleProductShelfs.Swap(i, RandomIndex);
        }
    }

    for (AProductShelf* SaleShelf : SaleProductShelfs)
    {
        if (IsValid(SaleShelf))
        {
            AProductBase* SpawnedProduct = SaleShelf->SpawnSpecificItem(SaleProduct, true);

            if (IsValid(SpawnedProduct))
            {
                CurrentProductCount++;
                UE_LOG(LogTemp, Log, TEXT("[제품선반 서브시스템] 세일 제품 스폰 및 체크 완료."));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[제품선반 서브시스템] 세일 제품 스폰 실패."));
            }
        }
    }
}

void UProductShelfSubsystem::LimitedProductSpawn(TSubclassOf<AProductBase> LimitedProduct)
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) return;

    if(!IsValid(LimitedProduct)) return;

    if (LimitedProductShelfs.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[제품선반 서브시스템] 한정 제품 선반이 없습니다!"));
        return;
    }

    // 한정 제품 선반이 여러 개일 경우 선반 하나 선택 
    int32 RandomIndex = FMath::RandRange(0, LimitedProductShelfs.Num() - 1);
    AProductBase* SpawnedProduct = LimitedProductShelfs[RandomIndex]->SpawnSpecificItem(LimitedProduct);

    if (IsValid(SpawnedProduct))
    {
        CurrentProductCount++;

        UE_LOG(LogTemp, Log, TEXT("[제품선반 서브시스템] 한정 제품 스폰 및 체크 완료."));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[제품선반 서브시스템] 한정 제품 스폰 실패."));
    }
}
