#include "EventManager/BC_EventSubsystem.h"

#include "ProductShelfSubsystem/ProductShelfSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "EventManager/SaleEventConfig.h"
#include "EventManager/LimitedEventConfig.h"

void UBC_EventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UBC_EventSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UBC_EventSubsystem::InitializeSaleEventConfig(USaleEventConfig* InSaleEnvantConfig)
{
    if (IsValid(InSaleEnvantConfig))
    {
        SaleEventConfig = InSaleEnvantConfig;
        UE_LOG(LogTemp, Log, TEXT("[이벤트 서브시스템] GameMode에서 세일 이벤트 데이터 에셋 로드완료"));

        SaleProductList = SaleEventConfig->SaleProductList;
        SaleEventTime = SaleEventConfig->SaleEventTime;
        SaleProductSpawnInterval = SaleEventConfig->SaleProductSpawnInterval;

        // 서버
        //if (GetWorld() && GetWorld()->GetNetMode() != NM_Client)
        //{
        //    // 게임 모드에서 호출시 삭제 예정 - 테스트용
        //    GetWorld()->GetTimerManager().SetTimer(SaleEventTimerHandle, this, &UBC_EventSubsystem::StartSaleEvent, SaleProductSpawnInterval, false);
        //}
    }
}

TSubclassOf<AProductBase> UBC_EventSubsystem::SaleProductSelection()
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) return nullptr;

    if (SaleProductList.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[이벤트 서브시스템] 제품선반매니저에 등록된 제품 목록이 비어있습니다."));
        return nullptr;
    }

    // 세일 제품 랜덤 선택
    int32 RandomIndex = FMath::RandRange(0, SaleProductList.Num() - 1);
    TSubclassOf<AProductBase> SaleProduct = SaleProductList[RandomIndex];

    UE_LOG(LogTemp, Log, TEXT("[이벤트 서브시스템] 세일 제품 : %s"), *SaleProduct->GetName());

    return SaleProduct;
}

void UBC_EventSubsystem::StartSaleEvent()
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) return;

    GetWorld()->GetTimerManager().ClearTimer(SaleEventTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(SaleProductSpawnTimerHandle);

    // 세일 제품 저장
    CurrentSaleProduct = SaleProductSelection();

    if (IsValid(CurrentSaleProduct))
    {
        UE_LOG(LogTemp, Log, TEXT("[이벤트 서브시스템] 세일 이벤트 시작"));
        GetWorld()->GetTimerManager().SetTimer(SaleProductSpawnTimerHandle, this, &UBC_EventSubsystem::ExecuteRepeatSpawn, SaleProductSpawnInterval, true, 0.0f);
        GetWorld()->GetTimerManager().SetTimer(SaleEventTimerHandle, this, &UBC_EventSubsystem::StopSaleEvent, SaleEventTime, false);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[이벤트 서브시스템] 세일 제품이 선택되지 않았습니다."));
    }
}

void UBC_EventSubsystem::StopSaleEvent()
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) return;

    GetWorld()->GetTimerManager().ClearTimer(SaleEventTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(SaleProductSpawnTimerHandle);

    CurrentSaleProduct = nullptr;

    UE_LOG(LogTemp, Log, TEXT("[이벤트 서브시스템] 세일 이벤트 종료, "));
}

void UBC_EventSubsystem::ExecuteRepeatSpawn()
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) return;

    if (!IsValid(CurrentSaleProduct)) return;

    if (UWorld* World = GetWorld())
    {
        if (auto* ProductShelfSubsystem = World->GetSubsystem<UProductShelfSubsystem>())
        {
            ProductShelfSubsystem->SaleProductSpawn(CurrentSaleProduct);

            UE_LOG(LogTemp, Log, TEXT("[이벤트 서브시스템] 선반 서브시스템 -> 세일제품 스폰 반복 호출"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[이벤트 서브시스템] 선반 서브시스템을 찾지 못했습니다."));
        }
    }
}

TSubclassOf<AProductBase> UBC_EventSubsystem::LimitedProductSelection()
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) return nullptr;

    if (LimitedProductList.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[이벤트 서브시스템] 제품선반매니저에 등록된 한정판 제품 목록이 비어있습니다."));
        return nullptr;
    }

    // 한정 제품 랜덤 선택
    int32 RandomIndex = FMath::RandRange(0, LimitedProductList.Num() - 1);
    TSubclassOf<AProductBase> LimitedProduct = LimitedProductList[RandomIndex];

    UE_LOG(LogTemp, Log, TEXT("[이벤트 서브시스템] 한정판 제품 : %s"), *LimitedProduct->GetName());

    return LimitedProduct;
}

void UBC_EventSubsystem::InitializeLimitedEventConfig(ULimitedEventConfig* InLimitedEventConfig)
{
    if (IsValid(InLimitedEventConfig))
    {
        LimitedEventConfig = InLimitedEventConfig;
        UE_LOG(LogTemp, Log, TEXT("[이벤트 서브시스템] GameMode에서 한정 이벤트 데이터 에셋 로드완료"));

        LimitedProductList = LimitedEventConfig->LimitedProductList;

        // 서버
        //if (GetWorld() && GetWorld()->GetNetMode() != NM_Client)
        //{
        //    // 게임 모드에서 호출시 삭제 예정 - 테스트용
        //    GetWorld()->GetTimerManager().SetTimer(TestLimitedEventTimerHandle, this, &UBC_EventSubsystem::StartLimitedEvent, 20.0f, false);
        //}
    }
}

void UBC_EventSubsystem::StartLimitedEvent()
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) return;

    GetWorld()->GetTimerManager().ClearTimer(TestLimitedEventTimerHandle);

    UE_LOG(LogTemp, Log, TEXT("[이벤트 서브시스템] 한정판 이벤트 시작."));

    TSubclassOf<AProductBase> LimitedProduct = LimitedProductSelection();

    if (!IsValid(LimitedProduct)) return;

    if (UWorld* World = GetWorld())
    {
        if (auto* ProductShelfSubsystem = World->GetSubsystem<UProductShelfSubsystem>())
        {
            ProductShelfSubsystem->LimitedProductSpawn(LimitedProduct);

            UE_LOG(LogTemp, Log, TEXT("[이벤트 서브시스템] 선반 서브시스템 -> 한정판 제품 스폰"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[이벤트 서브시스템] 선반 서브시스템을 찾지 못했습니다."));
        }
    }
}
