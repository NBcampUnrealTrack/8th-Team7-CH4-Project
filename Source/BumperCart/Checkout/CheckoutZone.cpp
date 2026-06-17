#include "Checkout/CheckoutZone.h"

#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Engine/Engine.h"

ACheckoutZone::ACheckoutZone()
{
 	PrimaryActorTick.bCanEverTick = false;
    //bReplicates = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    CounterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CounterMesh"));
    CounterMesh->SetupAttachment(SceneRoot);

    CheckoutTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("CheckoutTrigger"));
    CheckoutTrigger->SetupAttachment(SceneRoot);
    CheckoutTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CheckoutTrigger->SetGenerateOverlapEvents(true);
}

void ACheckoutZone::BeginPlay()
{
	Super::BeginPlay();

    CheckoutTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnCheckoutZoneBeginOverlap);
    CheckoutTrigger->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnCheckoutZoneEndOverlap);
}

// ------------------------------------------------------------
// Overlap 이벤트
// ------------------------------------------------------------
void ACheckoutZone::OnCheckoutZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority())
    {
        return;
    }

    APawn* PlayerPawn = Cast<APawn>(OtherActor);

    if (!IsValid(PlayerPawn))
    {
        return;
    }

    AddPlayerInZone(PlayerPawn);

    UE_LOG(LogTemp, Warning, TEXT("계산 구역 진입"));
}

void ACheckoutZone::OnCheckoutZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
    if (!HasAuthority())
    {
        return;
    }

    APawn* PlayerPawn = Cast<APawn>(OtherActor);

    if (!IsValid(PlayerPawn))
    {
        return;
    }

    RemovePlayerFromZone(PlayerPawn);

    UE_LOG(LogTemp, Warning, TEXT("계산 구역 이탈"));
}

// ------------------------------------------------------------
// 구역 내 플레이어
// ------------------------------------------------------------
void ACheckoutZone::AddPlayerInZone(APawn* PlayerPawn)
{
    if (!IsValid(PlayerPawn))
    {
        return;
    }

    PlayersInZone.AddUnique(PlayerPawn);

    // 배열에 추가 후 Checkout 시도
    // CheckoutZone에 진입 시 자동으로 정산 시작
    TryStartCheckout();
}

void ACheckoutZone::RemovePlayerFromZone(APawn* PlayerPawn)
{
    if (!IsValid(PlayerPawn))
    {
        return;
    }

    PlayersInZone.Remove(PlayerPawn);

    // 현재 정산 중인 플레이어가 이탈한 경우 정산 취소
    if (CurrentCheckoutPlayer == PlayerPawn)
    {
        CancelCheckout();

        // 배열에 있는 다음 플레이어가 정산 시작
        TryStartCheckout();
    }
}

APawn* ACheckoutZone::FindNextCheckoutPlayer()
{
    for (int32 i = 0; i < PlayersInZone.Num(); ++i)
    {
        APawn* PlayerPawn = PlayersInZone[i].Get();

        if (IsValid(PlayerPawn))
        {
            return PlayerPawn;
        }

        // 유효하지 않은 Pawn 배열에서 제거
        PlayersInZone.RemoveAt(i);
        --i;
    }

    return nullptr;
}

// ------------------------------------------------------------
// 계산 조건
// ------------------------------------------------------------
bool ACheckoutZone::CanStartCheckout(APawn* PlayerPawn) const
{
    // 플레이어인지
    if (!IsValid(PlayerPawn))
    {
        return false;
    }

    // 계산대 오픈 중인지
    if (CurrentCounterState != ECounterState::OPEN)
    {
        return false;
    }

    // 계산 중인지
    if (bIsCheckoutInProgress)
    {
        return false;
    }

    // 플레이어가 배열 안에 있는지
    if (!PlayersInZone.Contains(PlayerPawn))
    {
        return false;
    }

    // 상품이 1개 이상인지
    {

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

    APawn* NextPlayer = FindNextCheckoutPlayer();

    if (!CanStartCheckout(NextPlayer))
    {
        return;
    }

    StartCheckout(NextPlayer);
}

// ------------------------------------------------------------
// 계산 진행
// ------------------------------------------------------------
void ACheckoutZone::StartCheckout(APawn* PlayerPawn)
{
    if (!CanStartCheckout(PlayerPawn))
    {
        return;
    }

    CurrentCheckoutPlayer = PlayerPawn;
    bIsCheckoutInProgress = true;

    // 정산 시간 계산
    CheckoutProgress = 0.0f;
    ElapsedCheckoutTime = 0.0f;
    RequiredCheckoutTime = CalculateCheckoutDuration();     // 최종 정산 시간 계산
    CheckoutStartTime = GetWorld()->GetTimeSeconds();       // 정산 시작 시점, 월드 시간 기준임

    GetWorldTimerManager().SetTimer(
        CheckoutTimerHandle,
        this,
        &ThisClass::UpdateCheckoutProgress,
        0.05f,
        true
    );

    UE_LOG(LogTemp, Warning, TEXT("정산 시작"));
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

    const double CurrentTime = GetWorld()->GetTimeSeconds();
    ElapsedCheckoutTime = static_cast<float>(CurrentTime - CheckoutStartTime);
    CheckoutProgress = FMath::Clamp(ElapsedCheckoutTime / RequiredCheckoutTime, 0.0f, 1.0f);

    UE_LOG(LogTemp, Log, TEXT("정산 진행도: %.0f%%"),CheckoutProgress * 100.0f);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            1,
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

    APawn* CompletedPlayer = CurrentCheckoutPlayer;

    //UE_LOG(LogTemp, Warning, TEXT("정산 완료 - 획득 점수: %d"), CheckoutScore);

    // 정산 로직
    {

    }

    // 정산이 완료되면 플레이어는 대기열에서 제거
    PlayersInZone.Remove(CompletedPlayer);
    ResetCheckout();

    // 계산대 비활성화
    {

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

    CurrentCheckoutPlayer = nullptr;
    bIsCheckoutInProgress = false;

    CheckoutStartTime = 0.0;
    ElapsedCheckoutTime = 0.0f;
    RequiredCheckoutTime = 0.0f;
    CheckoutProgress = 0.0f;
}

// ------------------------------------------------------------
// 계산 시간 및 점수
// ------------------------------------------------------------

float ACheckoutZone::CalculateCheckoutDuration() const
{
    // 상품 개수에 따른 추가 시간 계산
    {

    }

    return BaseCheckoutTime + AdditionalCheckoutTime;
}

int32 ACheckoutZone::CalculateCheckoutScore() const
{
    // 상품 점수 계산
    {

    }

    return 1;
}
