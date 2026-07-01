#include "Checkout/CheckoutZone.h"

#include "Cart/Component/CartLoadComponent.h"
#include "Cart/CartPawn.h"
#include "Product/ProductTypes.h"
#include "Checkout/CheckoutScoreCalculator.h"
#include "Checkout/CheckoutBarrier.h"
#include "GameState/MainGameState.h"
#include "PlayerState/MainPlayerState.h"

#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/Engine.h"


ACheckoutZone::ACheckoutZone()
{
 	PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    CheckoutZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CheckoutZoneMesh"));
    CheckoutZoneMesh->SetupAttachment(SceneRoot);

    CheckoutTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("CheckoutTrigger"));
    CheckoutTrigger->SetupAttachment(SceneRoot);
    CheckoutTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CheckoutTrigger->SetGenerateOverlapEvents(true);

    // 불필요한 충돌 방지
    CheckoutTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    CheckoutTrigger->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);

    // 차단벽 생성
    LeftBarrierComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("LeftBarrierComponent"));
    LeftBarrierComponent->SetupAttachment(SceneRoot);
    LeftBarrierComponent->SetChildActorClass(ACheckoutBarrier::StaticClass());

    RightBarrierComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("RightBarrierComponent"));
    RightBarrierComponent->SetupAttachment(SceneRoot);
    RightBarrierComponent->SetChildActorClass(ACheckoutBarrier::StaticClass());

    EntranceBarrierComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("EntranceBarrierComponent"));
    EntranceBarrierComponent->SetupAttachment(SceneRoot);
    EntranceBarrierComponent->SetChildActorClass(ACheckoutBarrier::StaticClass());

    EjectPoint = CreateDefaultSubobject<USceneComponent>(TEXT("EjectPoint"));
    EjectPoint->SetupAttachment(SceneRoot);

    CheckoutZoneVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CheckoutZoneVisual"));

    CheckoutZoneVisual->SetupAttachment(SceneRoot);
    CheckoutZoneVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CheckoutZoneVisual->SetGenerateOverlapEvents(false);
    CheckoutZoneVisual->SetCastShadow(false);
}

void ACheckoutZone::BeginPlay()
{
	Super::BeginPlay();

    CheckoutTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnCheckoutZoneBeginOverlap);
    CheckoutTrigger->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnCheckoutZoneEndOverlap);

    InitializeCheckoutZoneMaterials();

    OnRep_CurrentCheckoutZoneState();

    ApplyBarrierState();
}

void ACheckoutZone::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ACheckoutZone, CurrentCheckoutZoneState);
    DOREPLIFETIME(ACheckoutZone, CurrentCheckoutPlayer);

    DOREPLIFETIME(ACheckoutZone, bIsCheckoutInProgress);
    DOREPLIFETIME(ACheckoutZone, CheckoutStartTime);
    DOREPLIFETIME(ACheckoutZone, RequiredCheckoutTime);

    DOREPLIFETIME(ACheckoutZone, bUseEntranceBarrier);
    DOREPLIFETIME(ACheckoutZone, bIsEntranceBarrierEnabled);
}

// ------------------------------------------------------------
// Overlap 이벤트
// ------------------------------------------------------------
void ACheckoutZone::OnCheckoutZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 플레이어의 상태를 서버가 결정
    if (!HasAuthority())
    {
        return;
    }

    ACartPawn* PlayerCharacter = Cast<ACartPawn>(OtherActor);

    if (!IsValid(PlayerCharacter))
    {
        return;
    }

    if (bUseEntranceBarrier)
    {
        // 이미 정산중인 플레이어가 있으면 후발 플레이어 배출
        if (IsValid(CurrentCheckoutPlayer) && CurrentCheckoutPlayer != PlayerCharacter)
        {
            EjectPlayer(PlayerCharacter);
            return;
        }

        // 아이템이 0개일 경우
        UCartLoadComponent* CartLoadComponent = PlayerCharacter->FindComponentByClass<UCartLoadComponent>();
        if (!IsValid(CartLoadComponent))
        {
            return;
        }

        //// 아이템 개수가 0개일 경우
        //// 갇히는 문제 방지
        //if (CartLoadComponent->GetCurrentLoadedCount() <= 0)
        //{
        //    return;
        //}
    }

    AddPlayerInZone(PlayerCharacter);
}

void ACheckoutZone::OnCheckoutZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
    // 플레이어의 상태를 서버가 결정
    if (!HasAuthority())
    {
        return;
    }

    ACartPawn* PlayerCharacter = Cast<ACartPawn>(OtherActor);

    if (!IsValid(PlayerCharacter))
    {
        return;
    }


    RemovePlayerFromZone(PlayerCharacter);

    UE_LOG(LogTemp, Warning, TEXT("계산 구역 이탈"));
}

// ------------------------------------------------------------
// 컴포넌트
// ------------------------------------------------------------

void ACheckoutZone::InitializeCheckoutZoneMaterials()
{
    if (!IsValid(CheckoutZoneVisual))
    {
        return;
    }

    CheckoutZoneVisualMID =  CheckoutZoneVisual->CreateAndSetMaterialInstanceDynamic(0);
}

void ACheckoutZone::UpdateCheckoutZoneVisual()
{
    if (!IsValid(CheckoutZoneVisual))
    {
        return;
    }

    if (CurrentCheckoutZoneState == ECheckoutZoneState::None)
    {
        CheckoutZoneVisual->SetVisibility(false);
        return;
    }

    CheckoutZoneVisual->SetVisibility(true);

    switch (CurrentCheckoutZoneState)
    {
    case ECheckoutZoneState::Open:
        ApplyCheckoutZoneVisual(OpenCheckoutZoneStyle);
        break;

    case ECheckoutZoneState::ClosingSoon:
        ApplyCheckoutZoneVisual(ClosingSoonCheckoutZoneStyle);
        break;

    case ECheckoutZoneState::Closed:
        ApplyCheckoutZoneVisual(ClosedCheckoutZoneStyle);
        break;

    default:
        CheckoutZoneVisual->SetVisibility(false);
        break;
    }
}

void ACheckoutZone::ApplyCheckoutZoneVisual(const FCheckoutZoneVisualStyle& Style)
{
    if (IsValid(CheckoutZoneVisualMID))
    {
        CheckoutZoneVisualMID->SetVectorParameterValue(TEXT("BorderColor"), Style.RingColor);
        CheckoutZoneVisualMID->SetScalarParameterValue(TEXT("BorderEmissiveStrength"), Style.RingEmissiveStrength);
        CheckoutZoneVisualMID->SetScalarParameterValue(TEXT("BorderOpacity"), Style.RingOpacity);
        CheckoutZoneVisualMID->SetVectorParameterValue(TEXT("FillColor"), Style.FillColor);
        CheckoutZoneVisualMID->SetScalarParameterValue(TEXT("FillEmissiveStrength"), Style.FillEmissiveStrength);
        CheckoutZoneVisualMID->SetScalarParameterValue(TEXT("FillOpacity"), Style.FillOpacity);
    }
}

// ------------------------------------------------------------
// 차단벽
// ------------------------------------------------------------

ACheckoutBarrier* ACheckoutZone::GetLeftBarrier() const
{
    if (!IsValid(LeftBarrierComponent))
    {
        return nullptr;
    }

    return Cast<ACheckoutBarrier>(LeftBarrierComponent->GetChildActor());
}

ACheckoutBarrier* ACheckoutZone::GetRightBarrier() const
{
    if (!IsValid(RightBarrierComponent))
    {
        return nullptr;
    }

    return Cast<ACheckoutBarrier>(RightBarrierComponent->GetChildActor());
}

ACheckoutBarrier* ACheckoutZone::GetEntranceBarrier() const
{
    if (!IsValid(EntranceBarrierComponent))
    {
        return nullptr;
    }

    return Cast<ACheckoutBarrier>(EntranceBarrierComponent->GetChildActor());
}

void ACheckoutZone::SetUseEntranceBarrier(bool bUseBarrier)
{
    if (!HasAuthority())
    {
        return;
    }

    if (bUseEntranceBarrier == bUseBarrier)
    {
        return;
    }

    bUseEntranceBarrier = bUseBarrier;

    // 기능을 끄면 현재 차단벽도 즉시 해제
    if (!bUseEntranceBarrier)
    {
        SetEntranceBarrierEnabled(false);
    }

    // 서버 자기 화면에 즉시 적용
    ApplyBarrierState();

    // 클라이언트에 복제
    ForceNetUpdate();
}

bool ACheckoutZone::IsUsingEntranceBarrier() const
{
    return bUseEntranceBarrier;
}

void ACheckoutZone::SetEntranceBarrierEnabled(bool bIsEnabled)
{
    if (!HasAuthority())
    {
        return;
    }

    const bool bNewBarrierEnabled = bUseEntranceBarrier && bIsEnabled;

    if (bIsEntranceBarrierEnabled == bNewBarrierEnabled)
    {
        return;
    }

    bIsEntranceBarrierEnabled = bNewBarrierEnabled;

    // 서버 자기 화면에 즉시 적용
    ApplyBarrierState();

    // 클라이언트에 복제
    ForceNetUpdate();
}

void ACheckoutZone::OnRep_UseEntranceBarrier()
{
    ApplyBarrierState();
}

void ACheckoutZone::OnRep_EntranceBarrierEnabled()
{
    ApplyBarrierState();
}

void ACheckoutZone::ApplyBarrierState()
{
    ACheckoutBarrier* LeftBarrier = GetLeftBarrier();
    ACheckoutBarrier* RightBarrier = GetRightBarrier();
    ACheckoutBarrier* EntranceBarrier = GetEntranceBarrier();

    // 벽차단 방식 비활성화 -> 좌우 고정벽도 비활성화
    if (IsValid(LeftBarrier))
    {
        LeftBarrier->SetBarrierEnabled(bUseEntranceBarrier);
    }

    if (IsValid(RightBarrier))
    {
        RightBarrier->SetBarrierEnabled(bUseEntranceBarrier);
    }

    // 차단벽 방식 활성화 and
    // 실제 입구벽 활성 상태도 true
    const bool bShouldEnableEntranceBarrier = bUseEntranceBarrier && bIsEntranceBarrierEnabled;

    if (IsValid(EntranceBarrier))
    {
        EntranceBarrier->SetBarrierEnabled(bShouldEnableEntranceBarrier);
    }
}

// ------------------------------------------------------------
// 플레이어 동시 진입
// ------------------------------------------------------------

void ACheckoutZone::EjectPlayer(ACartPawn* PlayerCharacter)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!IsValid(PlayerCharacter) || !IsValid(EjectPoint))
    {
        return;
    }

    // 혹시 이미 배열에 들어가 있었다면 계산 후보에서 제거
    if (PlayersInZone.Contains(PlayerCharacter))
    {
        RemovePlayerFromZone(PlayerCharacter);
    }

    // 방향 계산
    FVector EjectDirection = EjectPoint->GetComponentLocation() - PlayerCharacter->GetActorLocation();
    EjectDirection.Z = 0.0f;

    if (EjectDirection.IsNearlyZero())
    {
        return;
    }

    PlayerCharacter->ApplyExternalKnockback(EjectDirection, EjectStrength);
}

void ACheckoutZone::EjectNonCheckoutPlayers()
{
    if (!HasAuthority())
    {
        return;
    }

    for (int32 Index = PlayersInZone.Num() - 1; Index >= 0; --Index)
    {
        ACartPawn* PlayerCharacter = PlayersInZone[Index].Get();

        if (!IsValid(PlayerCharacter))
        {
            PlayersInZone.RemoveAt(Index);
            continue;
        }

        if (PlayerCharacter == CurrentCheckoutPlayer)
        {
            continue;
        }

        // EjectPlayer 내부에서 PlayersInZone 제거
        // 델리게이트 해제도 같이
        EjectPlayer(PlayerCharacter);
    }
}

void ACheckoutZone::CloseEntranceBarrier()
{
    if (!HasAuthority())
    {
        return;
    }

    // 지연 시간 동안 정산이 취소됐다면 닫지 않음
    if (!bIsCheckoutInProgress || !IsValid(CurrentCheckoutPlayer))
    {
        return;
    }

    SetEntranceBarrierEnabled(true);
}

// ------------------------------------------------------------
// 구역 내 플레이어
// ------------------------------------------------------------
void ACheckoutZone::AddPlayerInZone(ACartPawn* PlayerCharacter)
{
    if (!IsValid(PlayerCharacter))
    {
        return;
    }

    PlayersInZone.AddUnique(PlayerCharacter);

    // 계산대 진입 시 HandleLoadInfoChanged 델리게이트 등록
    UCartLoadComponent* CartLoadComponent = PlayerCharacter->FindComponentByClass<UCartLoadComponent>();
    if (IsValid(CartLoadComponent))
    {
        CartLoadComponent->OnLoadInfoChanged.AddUniqueDynamic(this,&ThisClass::HandleLoadInfoChanged);
    }

    UE_LOG(LogTemp, Warning, TEXT("계산 구역 진입: %s / 현재 계산대 내 인원: %d"), *GetNameSafe(PlayerCharacter), PlayersInZone.Num());

    // 배열에 추가 후 Checkout 시도
    // CheckoutZone에 진입 시 자동으로 정산 시작
    TryStartCheckout();
}

void ACheckoutZone::RemovePlayerFromZone(ACartPawn* PlayerCharacter)
{
    if (!IsValid(PlayerCharacter))
    {
        return;
    }

    // 계산대 이탈 시 HandleLoadInfoChanged 델리게이트 해제
    UCartLoadComponent* CartLoadComponent = PlayerCharacter->FindComponentByClass<UCartLoadComponent>();
    if (IsValid(CartLoadComponent))
    {
        CartLoadComponent->OnLoadInfoChanged.RemoveDynamic(this, &ThisClass::HandleLoadInfoChanged);
    }

    PlayersInZone.Remove(PlayerCharacter);

    UE_LOG(LogTemp, Warning, TEXT("계산 구역 이탈: %s / 현재 계산대 내 인원: %d"), *GetNameSafe(PlayerCharacter), PlayersInZone.Num());

    // 현재 정산 중인 플레이어가 이탈한 경우 정산 취소
    if (CurrentCheckoutPlayer == PlayerCharacter)
    {
        CancelCheckout();

        // 배열에 있는 다음 플레이어가 정산 시작
        TryStartCheckout();
    }
}

void ACheckoutZone::HandleLoadInfoChanged(AActor* OwnerActor, const FLoadInfo& LoadInfo)
{
    if (!HasAuthority())
    {
        return;
    }

    ACartPawn* PlayerCharacter = Cast<ACartPawn>(OwnerActor);
    if (!IsValid(PlayerCharacter))
    {
        return;
    }

    // 계산대 내부에서만 처리 가능
    if (!PlayersInZone.Contains(PlayerCharacter))
    {
        return;
    }

    // 상품 0개로 계산대에 들어온 뒤, 상품을 획득한 경우
    const int32 CurrentLoadedProductCount = FMath::Max(LoadInfo.CurrentLoadedCount, 0);

    if (!bIsCheckoutInProgress)
    {
        if (CurrentLoadedProductCount > 0)
        {
            TryStartCheckout();
        }

        return;
    }

    if (CurrentCheckoutPlayer != PlayerCharacter)
    {
        return;
    }

    // 정산 중 증가한 상품 수만큼 정산 시간 추가
    const int32 AddedProductCount = CurrentLoadedProductCount - LastLoadedProductCount;

    if (AddedProductCount > 0)
    {
        RequiredCheckoutTime += AdditionalCheckoutTime * AddedProductCount;

        UE_LOG(LogTemp, Warning, TEXT("상품 %d개 추가 / 필요 시간: %.1f초"), AddedProductCount, RequiredCheckoutTime);
    }

    // 현재 적재한 상품 수 갱신
    LastLoadedProductCount = CurrentLoadedProductCount;
}


ACartPawn* ACheckoutZone::FindNextCheckoutPlayer()
{
    for (int32 i = 0; i < PlayersInZone.Num(); ++i)
    {
        ACartPawn* PlayerCharacter = PlayersInZone[i].Get();

        // 유효하지 않은 플레이어 배열에서 제거
        if (!IsValid(PlayerCharacter))
        {
            PlayersInZone.RemoveAt(i);
            --i;
            continue;
        }

        // 정산 가능한 플레이어 리턴
        if (CanStartCheckout(PlayerCharacter))
        {
            return PlayerCharacter;
        }
    }

    return nullptr;
}

// ------------------------------------------------------------
// 계산 조건
// ------------------------------------------------------------
bool ACheckoutZone::CanStartCheckout(ACartPawn* PlayerCharacter) const
{
    // 플레이어인지
    if (!IsValid(PlayerCharacter))
    {
        return false;
    }

    // GameState가 없거나 라운드가 종료되었을 경우
    const AMainGameState* MainGameState = GetWorld()->GetGameState<AMainGameState>();
    if (!IsValid(MainGameState))
    {
        return false;
    }
    if (MainGameState->GetCurrentPhase() == ERoundPhase::RoundEnd || MainGameState->GetCurrentPhase() == ERoundPhase::None)
    {
        return false;
    }


    // 계산대 오픈 중인지
    if (CurrentCheckoutZoneState == ECheckoutZoneState::Closed)
    {
        return false;
    }

    // 계산 중인지
    if (bIsCheckoutInProgress)
    {
        return false;
    }

    // 플레이어가 배열 안에 있는지
    if (!PlayersInZone.Contains(PlayerCharacter))
    {
        return false;
    }

    UCartLoadComponent* CartLoadComponent = PlayerCharacter->FindComponentByClass<UCartLoadComponent>();

    // 적재 컴포넌트가 존재하는지
    if(!IsValid(CartLoadComponent))
    {
        return false;
    }

    if (CartLoadComponent->GetCurrentLoadedCount() <= 0)
    {
        return false;
    }

    return true;
}

void ACheckoutZone::TryStartCheckout()
{
    // 이미 정산 중인지 검사
    if (bIsCheckoutInProgress)
    {
        return;
    }

    // 배열에서 정산 가능한 플레이어 찾기
    ACartPawn* NextPlayer = FindNextCheckoutPlayer();

    if (!CanStartCheckout(NextPlayer))
    {
        return;
    }

    StartCheckout(NextPlayer);
}

// ------------------------------------------------------------
// 계산 진행
// ------------------------------------------------------------
void ACheckoutZone::StartCheckout(ACartPawn* PlayerCharacter)
{
    // 정산 시작 조건 검사
    if (!CanStartCheckout(PlayerCharacter))
    {
        return;
    }

    UCartLoadComponent* CartLoadComponent = PlayerCharacter->FindComponentByClass<UCartLoadComponent>();
    if (!IsValid(CartLoadComponent))
    {
        return;
    }

    // 정산 데이터
    CurrentCheckoutPlayer = PlayerCharacter;
    bIsCheckoutInProgress = true;
    CheckoutProgress = 0.0f;
    ElapsedCheckoutTime = 0.0f;

    // 기존에 먼저 들어와 있던 비정산 플레이어 배출
    if (bUseEntranceBarrier)
    {
        EjectNonCheckoutPlayers();
    }

    // 정산 시작 시 입구 차단벽 활성화
    //SetEntranceBarrierEnabled(true);
    if (bUseEntranceBarrier)
    {
        GetWorldTimerManager().ClearTimer(EntranceBarrierCloseTimerHandle);
        if (EntranceBarrierCloseDelay <= 0.0f)
        {
            CloseEntranceBarrier();
        }
        else
        {
            GetWorldTimerManager().SetTimer(
                EntranceBarrierCloseTimerHandle,
                this,
                &ThisClass::CloseEntranceBarrier,
                EntranceBarrierCloseDelay,
                false
            );
        }
    }

    // 적재된 상품 수에 따라 추가 정산 시간
    int32 ProductCount = CartLoadComponent->GetCurrentLoadedCount();
    LastLoadedProductCount = ProductCount;
    RequiredCheckoutTime = CalculateCheckoutDuration(ProductCount);

    // 클라이언트와 동기화된 정산 시작 시점
    AGameStateBase* GameStateBase = GetWorld()->GetGameState<AGameStateBase>();
    if (IsValid(GameStateBase))
    {
        CheckoutStartTime = GameStateBase->GetServerWorldTimeSeconds();
    }
    // 현재 월드 시간
    else
    {
        CheckoutStartTime = GetWorld()->GetTimeSeconds();
    }

    GetWorldTimerManager().SetTimer(
        CheckoutTimerHandle,
        this,
        &ThisClass::UpdateCheckoutProgress,
        0.05f,
        true
    );

    // 정산 시작 시 호출
    OnRep_CheckoutSession();

    UE_LOG(LogTemp, Warning, TEXT("%s 정산 시작"), *GetNameSafe(CurrentCheckoutPlayer));
}

void ACheckoutZone::UpdateCheckoutProgress()
{
    if (!bIsCheckoutInProgress)
    {
        return;
    }

    // 정산 중인 플레이어가 중도 이탈할 경우,
    // 정산 취소 및 다음 플레이어가 정산 시작
    if (!IsValid(CurrentCheckoutPlayer))
    {
        CancelCheckout();
        TryStartCheckout();
        return;
    }

    CheckoutProgress = GetCheckoutProgress();
    ElapsedCheckoutTime = CheckoutProgress * RequiredCheckoutTime;

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            1,
            0.1f,
            FColor::Green,
            FString::Printf(TEXT("정산 진행 시간: %.1f"), ElapsedCheckoutTime)
        );

        GEngine->AddOnScreenDebugMessage(
            2,
            0.1f,
            FColor::Green,
            FString::Printf(TEXT("정산 진행도: %.0f%%"), CheckoutProgress * 100.0f)
        );
    }

    if (CheckoutProgress >= 1.0f)
    {
        CompleteCheckout();
    }
}

// ------------------------------------------------------------
// 계산 종료
// ------------------------------------------------------------
void ACheckoutZone::CompleteCheckout()
{
    if (!bIsCheckoutInProgress)
    {
        return;
    }

    ACartPawn* CompletedPlayer = CurrentCheckoutPlayer;

    // 계산 취소 및 다음 플레이어 계산 시작
    if (!IsValid(CompletedPlayer))
    {
        CancelCheckout();
        TryStartCheckout();
        return;
    }

    // Load 컴포넌트 있는지
    UCartLoadComponent* CartLoadComponent = CompletedPlayer->FindComponentByClass<UCartLoadComponent>();
    if (!IsValid(CartLoadComponent))
    {
        CancelCheckout();
        TryStartCheckout();
        return;
    }

    // MainGameState 검사
    const AMainGameState* MainGameState = GetWorld()->GetGameState<AMainGameState>();
    if (!IsValid(MainGameState))
    {
        CancelCheckout();
        TryStartCheckout();
        return;
    }

    // 라운드 종료 시 정산 멈춤
    if (MainGameState->GetCurrentPhase() == ERoundPhase::RoundEnd || MainGameState->GetCurrentPhase() == ERoundPhase::None)
    {
        CancelCheckout();
        return;
    }

    // 현재 마지막 라운드인지
    const bool bIsLastCheckoutBonusApplied = MainGameState->GetCurrentPhase() == ERoundPhase::FinalWarningOneOpen;

    // PlayerState 검사
    AMainPlayerState* MainPlayerState = CompletedPlayer->GetPlayerState<AMainPlayerState>();
    if (!IsValid(MainPlayerState))
    {
        CancelCheckout();
        TryStartCheckout();
        return;
    }
    // 정산 점수 계산
    TArray<FLoadedProductInfo> CheckoutProducts;
    if (!CartLoadComponent->CheckoutProducts(CheckoutProducts)) // LoadedProducts 순회하며 정산 데이터 가져옴
    {
        CancelCheckout();
        TryStartCheckout();
        return;
    }

    // 보너스 점수 적용
    const FCheckoutScoreResult ScoreResult = UCheckoutScoreCalculator::CalculateCheckoutScore(
            CheckoutProducts,
            SaleBonusMultiplier,
            bIsLastCheckoutBonusApplied,
            LastCheckoutBonusMultiplier
        );

    // 정산 실패 시
    if (!ScoreResult.bIsCalculationCompleted)
    {
        CancelCheckout();
        TryStartCheckout();
        return;
    }

    // 최종 점수 계산
    LastCheckoutScore = ScoreResult.TotalScore;

    // 최종 점수 Player State 반영
    MainPlayerState->AddPlayerScore(LastCheckoutScore);
    MainPlayerState->AddCheckoutCount(1);

    UE_LOG(LogTemp, Warning, TEXT("정산 완료 - 획득 점수: %d"), LastCheckoutScore);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            1,
            3.0f,
            FColor::Blue,
            FString::Printf(TEXT("정산 완료 - 획득 점수: %d"), LastCheckoutScore)
        );
    }

    // 정산이 완료되면 플레이어는 대기열에서 제거
    //PlayersInZone.Remove(CompletedPlayer);

    // 계산대 세팅 초기화
    ResetCheckout();

    // 정산 완료 후 계산대 상태는 Manager에서 판단
    OnCheckoutCompleted.Broadcast(this);

    // 정산이 끝났으므로 입구 차단 해제
    SetEntranceBarrierEnabled(false);

    // Manager에서 계산대를 닫지 않은 경우, 다른 플레이어가 바로 정산 시도
    if (!bUseEntranceBarrier && CurrentCheckoutZoneState == ECheckoutZoneState::Open)
    {
        TryStartCheckout();
    }

    //UE_LOG(LogTemp, Warning, TEXT("계산대 상태: Close"));
}

void ACheckoutZone::CancelCheckout()
{
    if (!bIsCheckoutInProgress)
    {
        return;
    }

    ResetCheckout();

    // 정산 취소 시 입구 해제
    SetEntranceBarrierEnabled(false);

    UE_LOG(LogTemp, Warning, TEXT("정산 취소"));
}

void ACheckoutZone::ResetCheckout()
{
    GetWorldTimerManager().ClearTimer(CheckoutTimerHandle);

    GetWorldTimerManager().ClearTimer(EntranceBarrierCloseTimerHandle);
    SetEntranceBarrierEnabled(false);

    CurrentCheckoutPlayer = nullptr;
    bIsCheckoutInProgress = false;

    LastLoadedProductCount = 0;

    CheckoutStartTime = 0.0;
    ElapsedCheckoutTime = 0.0f;
    RequiredCheckoutTime = 0.0f;
    CheckoutProgress = 0.0f;

    // 정산 초기화 시 호출
    OnRep_CheckoutSession();
}

// ------------------------------------------------------------
// 계산 시간 및 점수
// ------------------------------------------------------------
float ACheckoutZone::CalculateCheckoutDuration(int32 ProductCount) const
{
    return BaseCheckoutTime + AdditionalCheckoutTime * ProductCount;
}

// ------------------------------------------------------------
// Getter
// ------------------------------------------------------------

float ACheckoutZone::GetCheckoutProgress() const
{
    // 정산 중이 아닐 경우
    if (!bIsCheckoutInProgress)
    {
        return 0.0f;
    }

    // 0으로 나누는 상황 방지
    if (RequiredCheckoutTime <= KINDA_SMALL_NUMBER)
    {
        return 0.0f;
    }

    // 현재 월드 시간
    float CurrentServerTime = GetWorld()->GetTimeSeconds();

    const AGameStateBase* GameStateBase = GetWorld()->GetGameState<AGameStateBase>();
    if (IsValid(GameStateBase))
    {
        // 서버와 동기화된 월드 시간
        CurrentServerTime = GameStateBase->GetServerWorldTimeSeconds();
    }

    // 정산이 시작된 시점부터 정산 경과 시간
    const float CurrentElapsedTime = CurrentServerTime - CheckoutStartTime;

    // 정산 진행도 반환
    return FMath::Clamp(CurrentElapsedTime / RequiredCheckoutTime, 0.0f, 1.0f);
}

float ACheckoutZone::GetRequiredCheckoutTime() const
{
    return RequiredCheckoutTime;
}

float ACheckoutZone::GetRemainingCheckoutTime() const
{
    if (!bIsCheckoutInProgress)
    {
        return 0.0f;
    }

    if (RequiredCheckoutTime <= KINDA_SMALL_NUMBER)
    {
        return 0.0f;
    }

    return FMath::Max(RequiredCheckoutTime * (1.0f - GetCheckoutProgress()), 0.0f);
}

bool ACheckoutZone::IsCheckoutInProgress() const
{
    return bIsCheckoutInProgress;
}

ACartPawn* ACheckoutZone::GetCurrentCheckoutPlayer() const
{
    return CurrentCheckoutPlayer;
}

ECheckoutZoneState ACheckoutZone::GetCheckoutZoneState() const
{
    return CurrentCheckoutZoneState;
}

int32 ACheckoutZone::GetCheckoutZoneID() const
{
    return CheckoutZoneID;
}

// ------------------------------------------------------------
// Setter
// ------------------------------------------------------------

void ACheckoutZone::SetCheckoutZoneState(ECheckoutZoneState NewState)
{
    if (!HasAuthority())
    {
        return;
    }

    if (CurrentCheckoutZoneState == NewState)
    {
        return;
    }

    CurrentCheckoutZoneState = NewState;

    // 콜백 함수 호출
    // 계산대 상태에 따른 색상, UI 등 변경
    OnRep_CurrentCheckoutZoneState();

    // 계산대가 닫혔다 다시 열렸을 때,
    // 이미 구역 안에 대기중이던 플레이어 바로 정산 시작
    if (CurrentCheckoutZoneState == ECheckoutZoneState::Open)
    {
        TryStartCheckout();
    }
}

// ------------------------------------------------------------
// RepNotify
// ------------------------------------------------------------

void ACheckoutZone::OnRep_CurrentCheckoutZoneState()
{
    switch (CurrentCheckoutZoneState)
    {
    case ECheckoutZoneState::None:
        UE_LOG(LogTemp, Warning, TEXT("계산대 상태: None"));
        break;

    case ECheckoutZoneState::Open:
        UE_LOG(LogTemp, Warning, TEXT("계산대 상태: Open"));
        // 초록색 색상 로직
        break;

    case ECheckoutZoneState::ClosingSoon:
        UE_LOG(LogTemp, Warning, TEXT("계산대 상태: ClosingSoon"));
        // 노란색 색상 로직
        break;

    case ECheckoutZoneState::Closed:
        UE_LOG(LogTemp, Warning, TEXT("계산대 상태: Closed"));
        // 빨간색 색상 로직
        break;

    default:
        break;
    }

    UpdateCheckoutZoneVisual();

    // 브로드캐스트
    OnCheckoutZoneStateChanged.Broadcast(CheckoutZoneID, CurrentCheckoutZoneState);
}

void ACheckoutZone::OnRep_CheckoutSession()
{
    //SetEntranceBarrierEnabled(bIsCheckoutInProgress);

    OnCheckoutSessionChanged.Broadcast(CheckoutZoneID, CurrentCheckoutPlayer, bIsCheckoutInProgress);
}
