#include "Checkout/CheckoutZone.h"

#include "Cart/CartPawn.h"
#include "Cart/Component/CartLoadComponent.h"
#include "Cart/Component/CartGrabComponent.h"
#include "Product/ProductTypes.h"
#include "Checkout/CheckoutScoreCalculator.h"
#include "Checkout/CheckoutBarrier.h"
#include "GameState/MainGameState.h"
#include "PlayerState/MainPlayerState.h"

#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"


ACheckoutZone::ACheckoutZone()
{
 	PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    CheckoutZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CheckoutZoneMesh"));
    CheckoutZoneMesh->SetupAttachment(SceneRoot);
    CheckoutZoneMesh->SetMobility(EComponentMobility::Static);

    CheckoutTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("CheckoutTrigger"));
    CheckoutTrigger->SetupAttachment(SceneRoot);
    CheckoutTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CheckoutTrigger->SetGenerateOverlapEvents(true);

    // 불필요한 충돌 방지
    CheckoutTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    CheckoutTrigger->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);

    // 차단벽 생성
    CheckoutBarrierComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("CheckoutBarrierComponent"));
    CheckoutBarrierComponent->SetupAttachment(SceneRoot);
    CheckoutBarrierComponent->SetChildActorClass(ACheckoutBarrier::StaticClass());

    CheckoutZoneVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CheckoutZoneVisual"));
    CheckoutZoneVisual->SetupAttachment(SceneRoot);
    CheckoutZoneVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CheckoutZoneVisual->SetGenerateOverlapEvents(false);
    CheckoutZoneVisual->SetCastShadow(false);

    CheckoutProcessingAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("CheckoutProcessingAudio"));
    CheckoutProcessingAudio->SetupAttachment(SceneRoot);
    CheckoutProcessingAudio->bAutoActivate = false;

    OpenStateEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("OpenStateEffect"));
    OpenStateEffect->SetupAttachment(CheckoutZoneVisual);
    OpenStateEffect->SetAutoActivate(false);
    OpenStateEffect->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
}

void ACheckoutZone::BeginPlay()
{
	Super::BeginPlay();

    CreateEjectPointsFromComponents();

    CheckoutTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnCheckoutZoneBeginOverlap);
    CheckoutTrigger->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnCheckoutZoneEndOverlap);

    InitializeCheckoutZoneMaterials();

    if (IsValid(OpenStateEffect))
    {
        OpenStateEffect->DeactivateImmediate();
    }

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

    DOREPLIFETIME(ACheckoutZone, bUseCheckoutBarrier);
    DOREPLIFETIME(ACheckoutZone, bIsCheckoutBarrierEnabled);
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

    //if (bUseCheckoutBarrier)
    //{
    //    // 이미 정산중인 플레이어가 있으면 후발 플레이어 배출
    //    if (IsValid(CurrentCheckoutPlayer) && CurrentCheckoutPlayer != PlayerCharacter)
    //    {
    //        EjectPlayer(PlayerCharacter);
    //        return;
    //    }

    //    // 아이템이 0개일 경우
    //    UCartLoadComponent* CartLoadComponent = PlayerCharacter->FindComponentByClass<UCartLoadComponent>();
    //    if (!IsValid(CartLoadComponent))
    //    {
    //        return;
    //    }

    //    //// 아이템 개수가 0개일 경우
    //    //// 갇히는 문제 방지
    //    //if (CartLoadComponent->GetCurrentLoadedCount() <= 0)
    //    //{
    //    //    return;
    //    //}
    //}

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
    UpdateCheckoutEffect();

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

        // Closing Soon 펄스 적용
        const bool bShouldPulse = CurrentCheckoutZoneState == ECheckoutZoneState::ClosingSoon;

        CheckoutZoneVisualMID->SetScalarParameterValue(TEXT("PulseEnabled"), bShouldPulse ? 1.0f : 0.0f);
        CheckoutZoneVisualMID->SetScalarParameterValue(TEXT("PulseSpeed"), bShouldPulse ? 2.2f : 0.0f);
        CheckoutZoneVisualMID->SetScalarParameterValue(TEXT("PulseMin"), bShouldPulse ? 0.25f : 1.0f);
        CheckoutZoneVisualMID->SetScalarParameterValue(TEXT("PulseMax"), bShouldPulse ? 1.0f : 1.0f);

        CheckoutZoneVisualMID->SetScalarParameterValue(TEXT("FillPulseEnabled"), bShouldPulse ? 1.0f : 0.0f);
        CheckoutZoneVisualMID->SetScalarParameterValue(TEXT("FillPulseSpeed"), bShouldPulse ? 1.1f : 0.0f);
        CheckoutZoneVisualMID->SetScalarParameterValue(TEXT("FillPulseMin"), bShouldPulse ? 0.9f : 1.0f);
        CheckoutZoneVisualMID->SetScalarParameterValue(TEXT("FillPulseMax"), bShouldPulse ? 1.2f : 1.0f);
    }
}

// ------------------------------------------------------------
// 사운드
// ------------------------------------------------------------

void ACheckoutZone::MulticastPlayCheckoutCompleteSound_Implementation()
{
    if (!IsValid(CheckoutCompleteSound))
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, CheckoutCompleteSound, GetActorLocation());

    UE_LOG(LogTemp, Warning, TEXT("정산 완료 사운드 재생"));
}

void ACheckoutZone::MulticastPlayCheckoutStateChangeSound_Implementation()
{
    if (!IsValid(CheckoutStateChangeSound))
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, CheckoutStateChangeSound, GetActorLocation());

    UE_LOG(LogTemp, Warning, TEXT("계산대 오픈"));
}

void ACheckoutZone::UpdateCheckoutEffect()
{
    if (!IsValid(OpenStateEffect))
    {
        return;
    }

    // 오픈 상태만 작용
    const bool bShouldPlayOpenNiagara = CurrentCheckoutZoneState == ECheckoutZoneState::Open;

    if (bShouldPlayOpenNiagara)
    {
        if (!OpenStateEffect->IsActive())
        {
            OpenStateEffect->Activate(true);
        }

        return;
    }

    if (OpenStateEffect->IsActive())
    {
        OpenStateEffect->DeactivateImmediate();
    }
}

void ACheckoutZone::MulticastPlayCheckoutCompleteEffect_Implementation(ACartPawn* CompletedPlayer)
{
    if (!IsValid(CompletedPlayer))
    {
        return;
    }

    if (!IsValid(CheckoutCompleteEffect))
    {
        return;
    }

    const FVector SpawnLocation = CompletedPlayer->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);

    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        this,
        CheckoutCompleteEffect,
        SpawnLocation,
        CompletedPlayer->GetActorRotation()
    );
}


// ------------------------------------------------------------
// 차단벽
// ------------------------------------------------------------


void ACheckoutZone::SetUseCheckoutBarrier(bool bUseBarrier)
{
    if (!HasAuthority())
    {
        return;
    }

    if (bUseCheckoutBarrier == bUseBarrier)
    {
        return;
    }

    bUseCheckoutBarrier = bUseBarrier;

    // 기능을 끄면 현재 차단벽도 즉시 해제
    if (!bUseCheckoutBarrier)
    {
        SetCheckoutBarrierEnabled(false);
    }

    // 서버 자기 화면에 즉시 적용
    ApplyBarrierState();

    // 클라이언트에 복제
    ForceNetUpdate();
}

bool ACheckoutZone::IsUsingCheckoutBarrier() const
{
    return bUseCheckoutBarrier;
}

void ACheckoutZone::SetCheckoutBarrierEnabled(bool bIsEnabled)
{
    if (!HasAuthority())
    {
        return;
    }

    // 차단벽 방식 활성화 여부
    const bool bNewBarrierEnabled = bUseCheckoutBarrier && bIsEnabled;

    if (bIsCheckoutBarrierEnabled == bNewBarrierEnabled)
    {
        return;
    }

    bIsCheckoutBarrierEnabled = bNewBarrierEnabled;

    // 서버 자기 화면에 즉시 적용
    ApplyBarrierState();

    // 클라이언트에 복제
    ForceNetUpdate();
}

void ACheckoutZone::OnRep_UseCheckoutBarrier()
{
    ApplyBarrierState();
}

void ACheckoutZone::OnRep_CheckoutBarrierEnabled()
{
    ApplyBarrierState();
}

void ACheckoutZone::ApplyBarrierState()
{
    ACheckoutBarrier* CheckoutBarrier = GetCheckoutBarrier();

    // 차단벽 방식 활성화 and
    // 실제 입구벽 활성 상태도 true
    const bool bShouldEnableCheckoutBarrier = bUseCheckoutBarrier && bIsCheckoutBarrierEnabled;

    if (IsValid(CheckoutBarrier))
    {
        CheckoutBarrier->SetBarrierEnabled(bShouldEnableCheckoutBarrier);
    }
}

ACheckoutBarrier* ACheckoutZone::GetCheckoutBarrier() const
{
    if (!IsValid(CheckoutBarrierComponent))
    {
        return nullptr;
    }

    return Cast<ACheckoutBarrier>(CheckoutBarrierComponent->GetChildActor());
}

// ------------------------------------------------------------
// 플레이어 동시 진입
// ------------------------------------------------------------

void ACheckoutZone::CreateEjectPointsFromComponents()
{
    EjectPoints.Empty();

    TArray<USceneComponent*> SceneComponents;
    GetComponents<USceneComponent>(SceneComponents);

    for (USceneComponent* SceneComponent : SceneComponents)
    {
        if (!IsValid(SceneComponent))
        {
            continue;
        }

        const FString ComponentName = SceneComponent->GetName();

        if (!ComponentName.StartsWith(TEXT("EjectPoint_")))
        {
            continue;
        }

        EjectPoints.Add(SceneComponent);
    }
}

void ACheckoutZone::EjectPlayer(ACartPawn* PlayerCharacter)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!IsValid(PlayerCharacter))
    {
        return;
    }

    // 이미 배출 중이면 중복 넉백 X
    if (EjectingPlayers.Contains(PlayerCharacter))
    {
        return;
    }

    USceneComponent* ClosestEjectPoint = FindBestEjectPoint(PlayerCharacter);

    if (!IsValid(ClosestEjectPoint))
    {
        return;
    }

    // 방향 계산
    FVector EjectDirection = ClosestEjectPoint->GetComponentLocation() - PlayerCharacter->GetActorLocation();
    EjectDirection.Z = 0.0f;

    if (EjectDirection.IsNearlyZero())
    {
        return;
    }

    // 배출 플레이어 배열에 추가
    EjectingPlayers.AddUnique(PlayerCharacter);

    MulticastSetPlayerBarrierIgnore(PlayerCharacter, true);

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

USceneComponent* ACheckoutZone::FindBestEjectPoint(const ACartPawn* PlayerCharacter) const
{
    if (!IsValid(PlayerCharacter))
    {
        return nullptr;
    }

    const FVector PlayerLocation = PlayerCharacter->GetActorLocation();

    // 가장 가까운 지점
    USceneComponent* ClosestEjectPoint = nullptr;
    float ClosestDistanceSquared = MAX_flt;

    // 막히지 않은 지점 중 가장 가까운 지점
    USceneComponent* ClosestClearEjectPoint = nullptr;
    float ClosestClearDistanceSquared = MAX_flt;

    for (USceneComponent* EjectPoint : EjectPoints)
    {
        if (!IsValid(EjectPoint))
        {
            continue;
        }

        const FVector EjectPointLocation = EjectPoint->GetComponentLocation();

        const float DistanceSquared = FVector::DistSquared2D(PlayerLocation, EjectPointLocation);

        // 가장 가까운 지점
        if (DistanceSquared < ClosestDistanceSquared)
        {
            ClosestDistanceSquared = DistanceSquared;
            ClosestEjectPoint = EjectPoint;
        }

        // 경로가 막히면 제외
        if (!IsEjectPathClear(PlayerCharacter,EjectPointLocation))
        {
            continue;
        }

        // 막힌 지점 제외하고, 가장 가까운 지점
        if (DistanceSquared < ClosestClearDistanceSquared)
        {
            ClosestClearDistanceSquared = DistanceSquared;
            ClosestClearEjectPoint = EjectPoint;
        }
    }

    // 막히지 않은 곳 중 가장 가까운 지점
    if (IsValid(ClosestClearEjectPoint))
    {
        return ClosestClearEjectPoint;
    }

    // 모든 지점이 막혔다면 가장 가까운 방향
    return ClosestEjectPoint;
}

bool ACheckoutZone::IsEjectPathClear(const ACartPawn* PlayerCharacter, const FVector& TargetLocation) const
{
    if (!IsValid(PlayerCharacter) || !IsValid(GetWorld()))
    {
        return false;
    }

    const UCapsuleComponent* CapsuleComponent = PlayerCharacter->FindComponentByClass<UCapsuleComponent>();

    if (!IsValid(CapsuleComponent))
    {
        return false;
    }

    const FVector StartLocation = PlayerCharacter->GetActorLocation();
    FVector EndLocation = TargetLocation;

    // 수평 경로만 검사
    EndLocation.Z = StartLocation.Z;

    // 현재 캡슐과 바닥이 접촉한 상태이므로
    // 초기 겹침 오검출을 줄이기 위해 약간 축소
    const float SweepRadius = FMath::Max(CapsuleComponent->GetScaledCapsuleRadius() - 2.0f, 1.0f);

    const float SweepHalfHeight = FMath::Max(CapsuleComponent->GetScaledCapsuleHalfHeight() - 2.0f, SweepRadius);

    const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(SweepRadius, SweepHalfHeight);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CheckoutEjectSweep), false);

    QueryParams.AddIgnoredActor(PlayerCharacter);
    QueryParams.AddIgnoredActor(this);

    // 차단벽 충돌 방지
    ACheckoutBarrier* CheckoutBarrier = GetCheckoutBarrier();
    if (IsValid(CheckoutBarrier))
    {
        QueryParams.AddIgnoredActor(CheckoutBarrier);
    }

    if (IsValid(CurrentCheckoutPlayer))
    {
        QueryParams.AddIgnoredActor(CurrentCheckoutPlayer);
    }

    FHitResult HitResult;

    const ECollisionChannel TraceChannel = CapsuleComponent->GetCollisionObjectType();

    const bool bHasBlockingHit = GetWorld()->SweepSingleByChannel(
            HitResult,
            StartLocation,
            EndLocation,
            FQuat::Identity,
            TraceChannel,
            CapsuleShape,
            QueryParams
        );

    return !bHasBlockingHit;
}

// ------------------------------------------------------------
// 충돌 무시
// ------------------------------------------------------------

void ACheckoutZone::SetPlayerBarrierIgnore(ACartPawn* PlayerCharacter, bool bShouldIgnore)
{
    if (!IsValid(PlayerCharacter))
    {
        return;
    }

    // 차단벽 있는지
    ACheckoutBarrier* CheckoutBarrier = GetCheckoutBarrier();
    if (!IsValid(CheckoutBarrier))
    {
        return;
    }

    // 캡슐 컴포넌트 있는지
    UCapsuleComponent* CapsuleComponent =  PlayerCharacter->FindComponentByClass<UCapsuleComponent>();
    if (!IsValid(CapsuleComponent))
    {
        return;
    }

    CapsuleComponent->IgnoreActorWhenMoving(CheckoutBarrier, bShouldIgnore);
}

void ACheckoutZone::MulticastSetPlayerBarrierIgnore_Implementation(ACartPawn* PlayerCharacter, bool bShouldIgnore)
{
    SetPlayerBarrierIgnore(PlayerCharacter, bShouldIgnore);
}

void ACheckoutZone::UpdateCheckoutPlayerBarrierIgnore()
{
    ACartPawn* NewIgnoredPlayer = nullptr;

    if (bUseCheckoutBarrier &&
        bIsCheckoutInProgress &&
        IsValid(CurrentCheckoutPlayer))
    {
        NewIgnoredPlayer = CurrentCheckoutPlayer;
    }

    ACartPawn* PreviousIgnoredPlayer = BarrierIgnoredPlayer.Get();

    if (PreviousIgnoredPlayer == NewIgnoredPlayer)
    {
        return;
    }

    // 이전 정산 플레이어 복구
    if (IsValid(PreviousIgnoredPlayer))
    {
        SetPlayerBarrierIgnore(PreviousIgnoredPlayer, false);
    }

    BarrierIgnoredPlayer = NewIgnoredPlayer;

    // 현재 정산 플레이어만 이 계산대 차단벽 무시
    if (IsValid(NewIgnoredPlayer))
    {
        SetPlayerBarrierIgnore(NewIgnoredPlayer, true);
    }
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

    const bool bWasEjecting = EjectingPlayers.Contains(PlayerCharacter);

    PlayersInZone.Remove(PlayerCharacter);
    EjectingPlayers.Remove(PlayerCharacter);
    CancelCheckoutPlayers.Remove(PlayerCharacter);

    // 밖으로 나간 뒤에는 다시 벽과 충돌
    if (bWasEjecting)
    {
        MulticastSetPlayerBarrierIgnore(PlayerCharacter, false);
    }

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

    // 배출 중 상품 획득 X
    if (EjectingPlayers.Contains(PlayerCharacter))
    {
        return;
    }

    // 상품 0개로 계산대에 들어온 뒤, 상품을 획득한 경우
    const int32 CurrentLoadedProductCount = FMath::Max(LoadInfo.CurrentLoadedCount, 0);

    if (!bIsCheckoutInProgress)
    {
        if (CurrentLoadedProductCount >= MinimumCheckoutProductCount)
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
    if (!IsValid(PlayerCharacter))
    {
        return false;
    }

    if (EjectingPlayers.Contains(PlayerCharacter))
    {
        return false;
    }

    //if (PlayerCharacter->IsCancelCheckoutState())
    //{
    //    return false;
    //}

    //// 정산 중 충돌로 정산이 취소된 상태인지
    //if (CancelCheckoutPlayers.Contains(PlayerCharacter))
    //{
    //    return false;
    //}

    const AMainGameState* MainGameState = GetWorld()->GetGameState<AMainGameState>();
    if (!IsValid(MainGameState))
    {
        return false;
    }

    if (MainGameState->GetCurrentPhase() == ERoundPhase::RoundEnd ||
        MainGameState->GetCurrentPhase() == ERoundPhase::WaitingToStart)
    {
        return false;
    }

    if (CurrentCheckoutZoneState == ECheckoutZoneState::Closed)
    {
        return false;
    }

    // 정산 중인 계산대인지
    if (bIsCheckoutInProgress)
    {
        return false;
    }

    // 정산 구역에 있는지
    if (!PlayersInZone.Contains(PlayerCharacter))
    {
        return false;
    }

    UCartLoadComponent* CartLoadComponent = PlayerCharacter->FindComponentByClass<UCartLoadComponent>();
    if (!IsValid(CartLoadComponent))
    {
        return false;
    }

    // 최소 개수
    const int32 LoadedProductCount =  FMath::Max(CartLoadComponent->GetCurrentLoadedCount(), 0);

    if (LoadedProductCount < MinimumCheckoutProductCount)
    {
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("정산 가능: %s / 상품 수: %d"), *GetNameSafe(PlayerCharacter), CartLoadComponent->GetCurrentLoadedCount());

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

    UCartGrabComponent* GrabComponent = CurrentCheckoutPlayer->FindComponentByClass<UCartGrabComponent>();
    if (IsValid(GrabComponent))
    {
        GrabComponent->SetGrabDisabledByCheckout(true);
    }

    //// 벽 충돌 켜질 시
    //if (bUseCheckoutBarrier)
    //{
    //    // 정산자는 충돌 무시 처리
    //    UpdateCheckoutPlayerBarrierIgnore();

    //    // 비정산 플레이어 배출
    //    EjectNonCheckoutPlayers();

    //    // 충돌 즉시 활성화
    //    // 메시는 0.2초에 걸쳐 생성
    //    SetCheckoutBarrierEnabled(true);
    //}

    AMainGameState* MainGameState = GetWorld()->GetGameState<AMainGameState>();
    if (!IsValid(MainGameState))
    {
        return;
    }

    bool bIsFinalPhase = false;

    if (MainGameState->GetCurrentPhase() == ERoundPhase::FinalWarningOneOpen)
    {
        bIsFinalPhase = true;
    }

    // 기존 일괄 정산 방식
    // 적재된 상품 수에 따라 추가 정산 시간
    //int32 ProductCount = CartLoadComponent->GetCurrentLoadedCount();
    //LastLoadedProductCount = ProductCount;
    //RequiredCheckoutTime = CalculateCheckoutDuration(ProductCount, bIsFinalPhase);

    // 상품 당 정산 방식
    const int32 ProductCount = CartLoadComponent->GetCurrentLoadedCount();
    LastLoadedProductCount = ProductCount;
    if (bIsUseSingleCheckout)
    {
        // 마지막 라운드일 경우 제한 시간 다르게
        RequiredCheckoutTime = RequiredCheckoutTime = bIsFinalPhase
            ? FMath::Max(FinalPhaseCheckoutTime, 0.1f)
            : FMath::Max(BaseCheckoutTime, 0.1f);
    }
    else
    {
        // 기존 정산 방식
        RequiredCheckoutTime = CalculateCheckoutDuration(ProductCount, bIsFinalPhase);
    }


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

    //UE_LOG(
    //    LogTemp,
    //    Warning,
    //    TEXT("[CheckoutTick] Player=%s / CancelState=%d / Progress=%.2f"),
    //    *GetNameSafe(CurrentCheckoutPlayer),
    //    IsValid(CurrentCheckoutPlayer) ? CurrentCheckoutPlayer->IsCancelCheckoutState() : false,
    //    CheckoutProgress
    //);

    //// 충돌 취소 상태일 경우
    //if (CurrentCheckoutPlayer->IsCancelCheckoutState())
    //{
    //    // 정산 취소 플레이어 배열에 추가
    //    CancelCheckoutPlayers.AddUnique(CurrentCheckoutPlayer);

    //    CancelCheckout();
    //    TryStartCheckout();
    //    return;
    //}

    CheckoutProgress = GetCheckoutProgress();
    ElapsedCheckoutTime = CheckoutProgress * RequiredCheckoutTime;

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
    if (MainGameState->GetCurrentPhase() == ERoundPhase::RoundEnd || MainGameState->GetCurrentPhase() == ERoundPhase::WaitingToStart)
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
    if (bIsUseSingleCheckout)
    {
        FLoadedProductInfo CheckoutProduct;

        if (!CartLoadComponent->CheckoutSingleProduct(CheckoutProduct))
        {
            CancelCheckout();
            TryStartCheckout();
            return;
        }

        CheckoutProducts.Add(CheckoutProduct);
    }
    else
    {
        // 기존 정산 방식
        if (!CartLoadComponent->CheckoutProducts(CheckoutProducts)) // LoadedProducts 순회하며 정산 데이터 가져옴
        {
            CancelCheckout();
            TryStartCheckout();
            return;
        }
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

    MulticastPlayCheckoutCompleteEffect(CompletedPlayer);
    MulticastPlayCheckoutCompleteSound();

    // 단일 상품 정산 true이고, 정산할 품목이 남은 경우
    if (bIsUseSingleCheckout && CartLoadComponent->GetCurrentLoadedCount() > 0)
    {
        CheckoutProgress = 0.0f;
        ElapsedCheckoutTime = 0.0f;
        RequiredCheckoutTime = bIsLastCheckoutBonusApplied
            ? FMath::Max(FinalPhaseCheckoutTime, 0.1f)
            : FMath::Max(BaseCheckoutTime, 0.1f);

        AGameStateBase* GameStateBase = GetWorld()->GetGameState<AGameStateBase>();

        if (IsValid(GameStateBase))
        {
            CheckoutStartTime = GameStateBase->GetServerWorldTimeSeconds();
        }
        else
        {
            CheckoutStartTime = GetWorld()->GetTimeSeconds();
        }

        ForceNetUpdate();

        return;
    }

    MainPlayerState->AddCheckoutCount(1);

    UE_LOG(LogTemp, Warning, TEXT("정산 완료 - 획득 점수: %d"), LastCheckoutScore);

    // 정산이 완료되면 플레이어는 대기열에서 제거
    //PlayersInZone.Remove(CompletedPlayer);

    // 계산대 세팅 초기화
    ResetCheckout();

    // 정산 완료 후 계산대 상태는 Manager에서 판단
    OnCheckoutCompleted.Broadcast(this);

    // Manager에서 계산대를 닫지 않은 경우, 다른 플레이어가 바로 정산 시도
    //if (!bUseCheckoutBarrier && CurrentCheckoutZoneState == ECheckoutZoneState::Open)
    //{
    //    TryStartCheckout();
    //}
    if (CurrentCheckoutZoneState != ECheckoutZoneState::Closed)
    {
        TryStartCheckout();
    }
}

void ACheckoutZone::CancelCheckout()
{
    if (!bIsCheckoutInProgress)
    {
        return;
    }

    ResetCheckout();

    UE_LOG(LogTemp, Warning, TEXT("정산 취소"));
}

void ACheckoutZone::ResetCheckout()
{
    GetWorldTimerManager().ClearTimer(CheckoutTimerHandle);

    // 정산 완료 시 벽 해제
    SetCheckoutBarrierEnabled(false);

    if (IsValid(CurrentCheckoutPlayer))
    {
        UCartGrabComponent* GrabComponent = CurrentCheckoutPlayer->FindComponentByClass<UCartGrabComponent>();
        if (IsValid(GrabComponent))
        {
            GrabComponent->SetGrabDisabledByCheckout(false);
        }
    }

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
float ACheckoutZone::CalculateCheckoutDuration(int32 ProductCount, bool bIsFinalPhase) const
{
    if (bIsFinalPhase)
    {
        return FMath::Max(FinalPhaseCheckoutTime, 0.1f);
    }

    // 아이템 수
    const int32 SafeProductCount = FMath::Max(ProductCount, 0);

    // 기본 정산 시간
    const float CalculatedDuration = BaseCheckoutTime + AdditionalCheckoutTime * SafeProductCount;

    // 최대 정산 시간
    const float SafeMaxCheckoutTime = FMath::Max(MaxCheckoutTime, 0.1f);

    return FMath::Min(CalculatedDuration, SafeMaxCheckoutTime);
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

bool ACheckoutZone::IsPlayerInsideCheckoutZone(const ACartPawn* PlayerCharacter) const
{
    if (!IsValid(PlayerCharacter))
    {
        return false;
    }

    return PlayersInZone.Contains(PlayerCharacter);
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

    const ECheckoutZoneState PreviousState = CurrentCheckoutZoneState;

    CurrentCheckoutZoneState = NewState;

    OnRep_CurrentCheckoutZoneState();

    // 정산 중 계산대 닫힐 경우 정산 취소
    if (bIsCheckoutInProgress && NewState == ECheckoutZoneState::Closed)
    {
        CancelCheckout();
    }

    // 계산대 열리거나 닫혔을 때 사운드 재생
    if (CurrentCheckoutZoneState == ECheckoutZoneState::Open ||
        CurrentCheckoutZoneState == ECheckoutZoneState::Closed)
    {
        MulticastPlayCheckoutStateChangeSound();
    }

    // 계산대가 닫혔다 다시 열렸을 때,
    // 이미 구역 안에 대기중이던 플레이어 바로 정산 시작
    if (CurrentCheckoutZoneState == ECheckoutZoneState::Open)
    {
        TryStartCheckout();
    }

    // 클라이언트에 상태 변경을 빠르게 복제
    ForceNetUpdate();
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
    UpdateCheckoutPlayerBarrierIgnore();

    OnCheckoutSessionChanged.Broadcast(CheckoutZoneID, CurrentCheckoutPlayer, bIsCheckoutInProgress);

    if (!IsValid(CheckoutProcessingAudio))
    {
        return;
    }

    // 정산자만 정산 중 오디오 재생
    if (bIsCheckoutInProgress)
    {
        if (!CheckoutProcessingAudio->IsPlaying())
        {
            CheckoutProcessingAudio->Play();
        }
    }
    else
    {
        CheckoutProcessingAudio->Stop();
    }
}
