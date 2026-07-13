#include "ItemSpawn/ProductShelfManager/ProductShelfManager.h"

#include "ItemSpawn/ProductShelf/ProductShelf.h"
#include "Kismet/GameplayStatics.h"
#include "EventManager/BC_EventManager.h"
#include "ItemSpawn/ProductShelfManager/ProductShelfManagerConfig.h"

AProductShelfManager::AProductShelfManager()
{
	PrimaryActorTick.bCanEverTick = false;

    // 클라이언트로 복제 금지 - 서버에만 존재
    bNetLoadOnClient = false;
}

void AProductShelfManager::BeginPlay()
{
	Super::BeginPlay();

    if (!HasAuthority()) return;

    AActor* FoundBC_EventManager = UGameplayStatics::GetActorOfClass(GetWorld(), ABC_EventManager::StaticClass());
    ABC_EventManager* BC_EventManager = Cast<ABC_EventManager>(FoundBC_EventManager);

    if (IsValid(BC_EventManager))
    {
        BC_EventManager->RegisterProductShelfManager(this);
    }

    if (IsValid(SpawnConfig))
    {
        MaxSpawnCount = SpawnConfig->MaxSpawnCount;
        MaxSpawnLimit = SpawnConfig->MaxSpawnLimit;
        RespawnDelay = SpawnConfig->RespawnDelay;

        UE_LOG(LogTemp, Log, TEXT("[선반매니저] 데이터 에셋 적용"));
    }

    // 게임 모드에서 호출시 삭제 예정 - 테스트용
    GetWorldTimerManager().SetTimer(GameStartTimerHandle, this, &AProductShelfManager::StartProductSpawning, RespawnDelay, false);
}

void AProductShelfManager::RegisterShelf(AProductShelf* InShelf, EShelfType InType)
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

    UE_LOG(LogTemp, Log, TEXT("[제품 선반 매니저] 선반 등록 (종류: %d, 일반: %d, 세일: %d, 한정: %d)"),
        (int32)InType, NormalProductShelfs.Num(), SaleProductShelfs.Num(), LimitedProductShelfs.Num());
}

void AProductShelfManager::ProductSpawnCall()
{
    if (!HasAuthority()) return;

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
                    UE_LOG(LogTemp, Warning, TEXT("[ProductShelfManager] 맵의 제품이 최대로 스폰되었습니다."));
                    return;
                }

                AProductBase* SpawnedProduct = Shelf->SpawnRandomProduct();

                // 스폰 성공
                if (IsValid(SpawnedProduct))
                {
                    CurrentProductCount++;

                    // 제품이 제거 되었을때 매니저의 현재 스폰된 수 관리를 위해
                    // ProductBase에 SetManager(AProductShelfManager* InManager) 추가 요청
                    // 변수로 TObjectPtr<AProductShelfManager> 추가 요청
                    //SpawnedProduct->SetManager(this);
                }
            }
        }
    }
    UE_LOG(LogTemp, Log, TEXT("[ProductShelfManager] 스폰된 제품 수 : %d"), CurrentProductCount);
}

void AProductShelfManager::OnProductDestroyed()
{
    if (CurrentProductCount > 0)
    {
        CurrentProductCount--;
    }
}

void AProductShelfManager::StartProductSpawning()
{
    GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AProductShelfManager::ProductSpawnCall, RespawnDelay, true);
}

void AProductShelfManager::InitializeConfig(UProductShelfManagerConfig* InConfig)
{
    if (IsValid(InConfig))
    {
        SpawnConfig = InConfig;
        UE_LOG(LogTemp, Log, TEXT("[선반 매니저] GameMode에서 데이터 에셋 로드완료"));
    }
}

void AProductShelfManager::SaleProductSpawn(TSubclassOf<AProductBase> SaleProduct)
{
    if (!HasAuthority() || !SaleProduct) return;

    if (SaleProductShelfs.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[ProductShelfManager] 세일 선반이 없습니다!"));
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
                UE_LOG(LogTemp, Log, TEXT("[ProductShelfManager] 세일 제품 스폰 및 체크 완료."));

                // 넷 멀티캐스트로 UI 알림 추가해야함
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[ProductShelfManager] 세일 제품 스폰 실패."));
            }
        }
    }
}

void AProductShelfManager::LimitedProductSpawn(TSubclassOf<AProductBase> LimitedProduct)
{
    if (!HasAuthority() || !LimitedProduct) return;

    if (LimitedProductShelfs.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[ProductShelfManager] 한정 제품 선반이 없습니다!"));
        return;
    }

    // 한정 제품 선반이 여러 개일 경우 선반 하나 선택 
    int32 RandomIndex = FMath::RandRange(0, LimitedProductShelfs.Num() - 1);
    AProductBase* SpawnedProduct = LimitedProductShelfs[RandomIndex]->SpawnSpecificItem(LimitedProduct);

    if (IsValid(SpawnedProduct))
    {
        // 한정 제품 체크 (변수, Set() 함수 추가 요청)
        // SpawnedProduct->SetLimitedProduct(true);

        CurrentProductCount++;
        UE_LOG(LogTemp, Log, TEXT("[ProductShelfManager] 한정 제품 스폰 및 체크 완료."));

        // 넷 멀티캐스트로 UI 알림 추가해야함
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ProductShelfManager] 한정 제품 스폰 실패."));
    }
}

