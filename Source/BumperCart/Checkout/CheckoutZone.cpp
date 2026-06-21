#include "Checkout/CheckoutZone.h"

#include "Cart/Component/CartLoadComponent.h"
#include "Product/ProductTypes.h"

#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
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
    // 정산 시작 조건 검사
    if (!CanStartCheckout(PlayerCharacter))
    {
        return;
    }

    UCartLoadComponent* CartLoadComponent = CurrentCheckoutPlayer->FindComponentByClass<UCartLoadComponent>();

    if (!IsValid(CartLoadComponent))
    {
        return;
    }

    // 정산 데이터
    CurrentCheckoutPlayer = PlayerCharacter;
    bIsCheckoutInProgress = true;
    CheckoutProgress = 0.0f;
    ElapsedCheckoutTime = 0.0f;

    // 적재된 상품 수에 따라 추가 정산 시간
    int32 ProductCount = CartLoadComponent->GetCurrentLoadedCount();
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

    ACharacter* CompletedPlayer = CurrentCheckoutPlayer;

    if (!IsValid(CompletedPlayer))
    {
        CancelCheckout();
        TryStartCheckout();
        return;
    }

    UCartLoadComponent* CartLoadComponent = CompletedPlayer->FindComponentByClass<UCartLoadComponent>();

    if (!IsValid(CartLoadComponent))
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
    CheckoutScore = CalculateCheckoutScore(CheckoutProducts);
    //PlayerState->AddScore(CehckoutScore);
    

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

    // 정산 완료 후 계산대 폐쇄 안할 경우 다음 플레이어가 정산 시도
    // TryStartCheckout();


    // 정산 완료 후 계산대 폐쇄
    //CurrentCounterState = ECounterState::Closed;

    //OnRep_CurrentCounterState();

    //UE_LOG(LogTemp, Warning, TEXT("계산대 상태: Close"));
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

int32 ACheckoutZone::CalculateCheckoutScore(const TArray<FLoadedProductInfo>& Products) const
{
    int32 TotalScore = 0;

    // 상품 목록의 점수 합산 및 리턴
    for (const FLoadedProductInfo& ProductInfo : Products)
    {
        TotalScore += ProductInfo.Value;
    }

    return TotalScore;
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

bool ACheckoutZone::IsCheckoutInProgress() const
{
    return bIsCheckoutInProgress;
}

ACharacter* ACheckoutZone::GetCurrentCheckoutPlayer() const
{
    return CurrentCheckoutPlayer;
}

ECounterState ACheckoutZone::GetCounterState() const
{
    return CurrentCounterState;
}

// ------------------------------------------------------------
// Setter
// ------------------------------------------------------------

int32 ACheckoutZone::GetCounterID() const
{
    return CounterID;
}

void ACheckoutZone::SetCounterState(ECounterState NewState)
{
    if (!HasAuthority())
    {
        return;
    }

    if (CurrentCounterState == NewState)
    {
        return;
    }

    CurrentCounterState = NewState;

    // 콜백 함수 호출
    // 계산대 상태에 따른 색상, UI 등 변경
    OnRep_CurrentCounterState();
}
