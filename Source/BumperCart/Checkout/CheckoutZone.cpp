#include "Checkout/CheckoutZone.h"

#include "GameFramework/Character.h"
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

void ACheckoutZone::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ACheckoutZone, CurrentCounterState);
    DOREPLIFETIME(ACheckoutZone, CurrentCheckoutPlayer);
    DOREPLIFETIME(ACheckoutZone, bIsCheckoutInProgress);
    DOREPLIFETIME(ACheckoutZone, CheckoutStartTime);
    DOREPLIFETIME(ACheckoutZone, RequiredCheckoutTime);
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

    ACharacter* PlayerCharacter = Cast<ACharacter>(OtherActor);

    if (!IsValid(PlayerCharacter))
    {
        return;
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

    ACharacter* PlayerCharacter = Cast<ACharacter>(OtherActor);

    if (!IsValid(PlayerCharacter))
    {
        return;
    }


    RemovePlayerFromZone(PlayerCharacter);

    UE_LOG(LogTemp, Warning, TEXT("계산 구역 이탈"));
}

// ------------------------------------------------------------
// 구역 내 플레이어
// ------------------------------------------------------------
void ACheckoutZone::AddPlayerInZone(ACharacter* PlayerCharacter)
{
    if (!IsValid(PlayerCharacter))
    {
        return;
    }

    PlayersInZone.AddUnique(PlayerCharacter);

    UE_LOG(LogTemp, Warning, TEXT("계산 구역 진입: %s / 현재 계산대 내 인원: %d"), *GetNameSafe(PlayerCharacter), PlayersInZone.Num());

    // 배열에 추가 후 Checkout 시도
    // CheckoutZone에 진입 시 자동으로 정산 시작
    TryStartCheckout();
}

void ACheckoutZone::RemovePlayerFromZone(ACharacter* PlayerCharacter)
{
    if (!IsValid(PlayerCharacter))
    {
        return;
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

ACharacter* ACheckoutZone::FindNextCheckoutPlayer()
{
    for (int32 i = 0; i < PlayersInZone.Num(); ++i)
    {
        ACharacter* PlayerCharacter = PlayersInZone[i].Get();

        if (IsValid(PlayerCharacter))
        {
            return PlayerCharacter;
        }

        // 유효하지 않은 Pawn 배열에서 제거
        PlayersInZone.RemoveAt(i);
        --i;
    }

    return nullptr;
}

void ACheckoutZone::OnRep_CurrentCounterState()
{
    switch (CurrentCounterState)
    {
    case ECounterState::None:
        UE_LOG(LogTemp, Warning, TEXT("계산대 상태: None"));
        break;

    case ECounterState::Open:
        UE_LOG(LogTemp, Warning, TEXT("계산대 상태: Open"));
        // 초록색 색상 로직
        break;

    case ECounterState::ClosingSoon:
        UE_LOG(LogTemp, Warning, TEXT("계산대 상태: ClosingSoon"));
        // 노란색 색상 로직
        break;

    case ECounterState::Closed:
        UE_LOG(LogTemp, Warning, TEXT("계산대 상태: Closed"));
        // 빨간색 색상 로직
        break;

    default:
        break;
    }
}

// ------------------------------------------------------------
// 계산 조건
// ------------------------------------------------------------
bool ACheckoutZone::CanStartCheckout(ACharacter* PlayerCharacter) const
{
    // 플레이어인지
    if (!IsValid(PlayerCharacter))
    {
        return false;
    }

    // 계산대 오픈 중인지
    if (CurrentCounterState == ECounterState::Closed)
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

    ACharacter* NextPlayer = FindNextCheckoutPlayer();

    if (!CanStartCheckout(NextPlayer))
    {
        return;
    }

    StartCheckout(NextPlayer);
}

// ------------------------------------------------------------
// 계산 진행
// ------------------------------------------------------------
void ACheckoutZone::StartCheckout(ACharacter* PlayerCharacter)
{
    if (!CanStartCheckout(PlayerCharacter))
    {
        return;
    }

    CurrentCheckoutPlayer = PlayerCharacter;
    bIsCheckoutInProgress = true;

    // 정산 시간 계산
    CheckoutProgress = 0.0f;
    ElapsedCheckoutTime = 0.0f;
    RequiredCheckoutTime = CalculateCheckoutDuration(1);     // 최종 정산 시간 계산
    CheckoutStartTime = GetWorld()->GetTimeSeconds();       // 정산 시작 시점

    GetWorldTimerManager().SetTimer(
        CheckoutTimerHandle,
        this,
        &ThisClass::UpdateCheckoutProgress,
        0.05f,
        true
    );

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

    // 정산 시작부터 경과 시간 계산
    float CurrentTime = GetWorld()->GetTimeSeconds();       // 월드 경과 시간
    ElapsedCheckoutTime = CurrentTime - CheckoutStartTime;  // 월드 경과 시간 - 정산 시작 순간
    CheckoutProgress = FMath::Clamp(ElapsedCheckoutTime / RequiredCheckoutTime, 0.0f, 1.0f);

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

    ACharacter* CompletedPlayer = CurrentCheckoutPlayer;

    // 정산 점수 계산
    // 나중에 PlayerState로 전달 필요
    {
        CheckoutScore = CalculateCheckoutScore();
        //PlayerState->AddScore(CehckoutScore);
    }

    UE_LOG(LogTemp, Warning, TEXT("정산 완료 - 획득 점수: %d"), CheckoutScore);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            1,
            3.0f,
            FColor::Blue,
            FString::Printf(TEXT("정산 완료 - 획득 점수: %d"), CheckoutScore)
        );
    }

    // 정산이 완료되면 플레이어는 대기열에서 제거
    PlayersInZone.Remove(CompletedPlayer);

    // 계산대 세팅 초기화
    ResetCheckout();

    // 다음에 들어온 플레이어 정산 시작
    TryStartCheckout();
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
float ACheckoutZone::CalculateCheckoutDuration(int32 ProductCount) const
{
    return BaseCheckoutTime + AdditionalCheckoutTime * ProductCount;
}

int32 ACheckoutZone::CalculateCheckoutScore() const
{
    // 상품 점수 계산
    {

    }

    return 1;
}
