#include "Checkout/CheckoutManager.h"

#include "Checkout/CheckoutZone.h"
#include "Checkout/CheckoutTypes.h"

#include "TimerManager.h"

ACheckoutManager::ACheckoutManager()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;

    // 기본 스케줄
    RotationSteps =
    {
        { 30.0f, 2 },
        { 60.0f, 2 },
        { 90.0f, 1 },
        { 120.0f, 1 },
        { 150.0f, 1 },
        { 180.0f, 0 }
    };
}

void ACheckoutManager::BeginPlay()
{
    Super::BeginPlay();

    // 서버에서만 수행
    if (!HasAuthority())
    {
        return;
    }

    // 계산대 초기 세팅
    if (!InitializeCheckoutZones())
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("계산대 수: %d"), CheckoutZones.Num());

    for (ACheckoutZone* CheckoutZone : CheckoutZones)
    {
        UE_LOG(LogTemp, Warning, TEXT("계산대 ID: %d"), CheckoutZone->GetCheckoutZoneID());
    }
}

// ------------------------------------------------------------
// 차단벽 설정
// ------------------------------------------------------------

void ACheckoutManager::SetUseCheckoutBarrier(bool bUseBarrier)
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

    for (ACheckoutZone* CheckoutZone : CheckoutZones)
    {
        if (!IsValid(CheckoutZone))
        {
            continue;
        }

        CheckoutZone->SetUseCheckoutBarrier(bUseCheckoutBarrier);
    }
}

bool ACheckoutManager::IsUsingCheckoutBarrier() const
{
    return bUseCheckoutBarrier;
}

// ------------------------------------------------------------
// 계산대 목록 및 세팅
// ------------------------------------------------------------

bool ACheckoutManager::InitializeCheckoutZones()
{
    // 월드에 계산대가 하나라도 존재하는지
    if (CheckoutZones.IsEmpty())
    {
        return false;
    }

    // 각 계산대가 모두 유효한지
    for (ACheckoutZone* CheckoutZone : CheckoutZones)
    {
        if (!IsValid(CheckoutZone))
        {
            return false;
        }
    }

    // 게임 시작 시
    for (ACheckoutZone* CheckoutZone : CheckoutZones)
    {
        // 델리게이트 연결
        //CheckoutZone->OnCheckoutCompleted.AddUObject(this, &ThisClass::HandleCheckoutCompleted);
        // 모든 계산대 오픈
        CheckoutZone->SetCheckoutZoneState(ECheckoutZoneState::Open);
        // 차단벽 적용 설정 적용
        CheckoutZone->SetUseCheckoutBarrier(bUseCheckoutBarrier);
    }

    return true;
}

bool ACheckoutManager::AreCheckoutZonesValid() const
{
    if (CheckoutZones.IsEmpty())
    {
        return false;
    }

    for (ACheckoutZone* CheckoutZone : CheckoutZones)
    {
        if (!IsValid(CheckoutZone))
        {
            return false;
        }
    }

    return true;
}

// ------------------------------------------------------------
// 계산대 상태 변경
// ------------------------------------------------------------
void ACheckoutManager::StartCheckoutRotation()
{
    if (!HasAuthority())
    {
        return;
    }

    if (!AreCheckoutZonesValid())
    {
        return;
    }

    StopCheckoutRotation();

    bIsCheckoutRotationRunning = true;
    CheckoutRotationStartWorldTime = GetWorld()->GetTimeSeconds();
    CurrentRotationStepIndex = 0;

    // 라운드 시작 시 계산대는 전부 Open
    OpenAllCheckoutZones();

    // 첫 번째 step의 ClosingSoon 시작 예약
    ScheduleNextRotationStepWarning();
}

void ACheckoutManager::StopCheckoutRotation()
{
    if (!HasAuthority())
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(RotationStepWarningTimerHandle);
    GetWorldTimerManager().ClearTimer(ClosingSoonTimerHandle);

    bIsCheckoutRotationRunning = false;
    CurrentRotationStepIndex = INDEX_NONE;

    RandomOpenCheckoutZones.Empty();

    // 라운드 종료 시 계산대 전부 Closed
    CloseAllCheckoutZones();
}

void ACheckoutManager::ScheduleNextRotationStepWarning()
{
    if (!HasAuthority())
    {
        return;
    }

    if (!bIsCheckoutRotationRunning)
    {
        return;
    }

    if (!RotationSteps.IsValidIndex(CurrentRotationStepIndex))
    {
        return;
    }

    const FCheckoutRotationStep& CurrentStep = RotationSteps[CurrentRotationStepIndex];

    const float CurrentElapsedTime =
        GetWorld()->GetTimeSeconds() - CheckoutRotationStartWorldTime;

    // ApplyTime보다 ClosingSoonDuration만큼 먼저 경고 시작
    const float WarningStartTime = FMath::Max(CurrentStep.ApplyTime - ClosingSoonDuration, 0.0f);

    const float WarningDelay = FMath::Max(WarningStartTime - CurrentElapsedTime, 0.0f);

    GetWorldTimerManager().SetTimer(
        RotationStepWarningTimerHandle,
        this,
        &ThisClass::HandleRotationStepWarning,
        WarningDelay,
        false
    );
}

void ACheckoutManager::OpenAllCheckoutZones()
{
    if (!HasAuthority())
    {
        return;
    }

    if (!AreCheckoutZonesValid())
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(ClosingSoonTimerHandle);

    RandomOpenCheckoutZones.Empty();

    for (ACheckoutZone* CheckoutZone : CheckoutZones)
    {
        if (!IsValid(CheckoutZone))
        {
            continue;
        }

        CheckoutZone->SetCheckoutZoneState(ECheckoutZoneState::Open);
    }
}

void ACheckoutManager::CloseAllCheckoutZones()
{
    if (!HasAuthority())
    {
        return;
    }

    if (!AreCheckoutZonesValid())
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(ClosingSoonTimerHandle);

    RandomOpenCheckoutZones.Empty();

    for (ACheckoutZone* CheckoutZone : CheckoutZones)
    {
        if (!IsValid(CheckoutZone))
        {
            continue;
        }

        CheckoutZone->SetCheckoutZoneState(ECheckoutZoneState::Closed);
    }
}

void ACheckoutManager::HandleRotationStepWarning()
{
    if (!HasAuthority())
    {
        return;
    }

    if (!bIsCheckoutRotationRunning)
    {
        return;
    }

    if (!RotationSteps.IsValidIndex(CurrentRotationStepIndex))
    {
        return;
    }

    const FCheckoutRotationStep& CurrentStep = RotationSteps[CurrentRotationStepIndex];

    // 기존 함수 재사용
    // 여기서 닫힐 계산대가 ClosingSoon으로 바뀌고,
    // ClosingSoonDuration 뒤 ChangeNextCheckoutZoneState()가 호출됨
    PrepareNextCheckoutZoneState(CurrentStep.OpenCount);
}

// 계산대 정산 완료 시,
// 모든 계산대가 Open 상태일 경우   -> 계속 Open 상태로 유지
// Closed 계산대가 하나 이상일 경우 -> Closed 계산대 중 하나 랜덤 선택하여 Open 상태로 변경
void ACheckoutManager::HandleCheckoutCompleted(ACheckoutZone* CompletedCheckoutZone)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!IsValid(CompletedCheckoutZone))
    {
        return;
    }

    // 이미 닫힌 계산대여도 진행 중인 정산이 완료되면 정산
    if (CompletedCheckoutZone->GetCheckoutZoneState() == ECheckoutZoneState::Closed)
    {
        return;
    }

    if (CompletedCheckoutZone->GetCheckoutZoneState() == ECheckoutZoneState::ClosingSoon)
    {
        return;
    }

    TArray<ACheckoutZone*> ClosedCheckoutZones;

    // 완료된 계산대를 제외한 Closed 계산대 검색
    for (ACheckoutZone* CheckoutZone : CheckoutZones)
    {
        if (!IsValid(CheckoutZone))
        {
            continue;
        }

        // 같은 계산대가 Open 되는 것을 방지
        if (CheckoutZone == CompletedCheckoutZone)
        {
            continue;
        }

        if (CheckoutZone->GetCheckoutZoneState() == ECheckoutZoneState::Closed)
        {
            ClosedCheckoutZones.Add(CheckoutZone);
        }
    }

    // 다른 Closed 계산대가 없음
    if (ClosedCheckoutZones.IsEmpty())
    {
        return;
    }

    // 정산 완료된 계산대 폐쇄
    CompletedCheckoutZone->SetCheckoutZoneState(ECheckoutZoneState::Closed);

    // Closed 계산대 중 하나 선택하여 오픈 상태로 변경
    const int32 RandomIndex = FMath::RandRange(0, ClosedCheckoutZones.Num() - 1);
    ACheckoutZone* CheckoutZoneToOpen = ClosedCheckoutZones[RandomIndex];
    CheckoutZoneToOpen->SetCheckoutZoneState(ECheckoutZoneState::Open);
}


// Closing Soon 상태를 표시하기 위해 계산대의 상태를 즉시 변경하지 않고,
// 미리 다음 라운드에 Open할 계산대를 랜덤으로 미리 추첨한다.
// 그리고 ClosingSoonDuration 이후에 계산대의 상태가 변경된다.
void ACheckoutManager::PrepareNextCheckoutZoneState(int32 OpenCount)
{
    if (!HasAuthority())
    {
        return;
    }

    // 월드에 계산대가 하나라도 존재하는지
    if (CheckoutZones.IsEmpty())
    {
        return;
    }

    // 각 계산대가 모두 유효한지
    for (ACheckoutZone* CheckoutZone : CheckoutZones)
    {
        if (!IsValid(CheckoutZone))
        {
            return;
        }
    }

    // Closing Soon 타이머 제거
    GetWorldTimerManager().ClearTimer(ClosingSoonTimerHandle);

    // 최대 오픈 개수는 월드에 존재하는 계산대 수만큼
    const int32 ClampedOpenCount = FMath::Clamp(OpenCount, 0, CheckoutZones.Num());

    // 입력된 개수만큼 계산대를 무작위 선택
    RandomOpenCheckoutZones = SelectRandomOpenCheckoutZones(ClampedOpenCount);

    for (ACheckoutZone* CheckoutZone : CheckoutZones)
    {
        if (!IsValid(CheckoutZone))
        {
            continue;
        }

        // 다음 턴에 오픈될 계산대인지
        const bool bShouldBeOpen = RandomOpenCheckoutZones.Contains(CheckoutZone);

        // Open 대상일 경우
        // Open -> Open, Closed -> Open 상태가 됨
        if (bShouldBeOpen)
        {
            continue;
        }
        else
        {
            // 만약 현재 오픈 중인데 다음 턴에 오픈하지 않을 계산대일 경우
            // Closing Soon 상태로 변경
            if (CheckoutZone->GetCheckoutZoneState() == ECheckoutZoneState::Open)
            {
                CheckoutZone->SetCheckoutZoneState(ECheckoutZoneState::ClosingSoon);
           }
        }
    }

    // ClosingSoonDuration 이후 계산대 상태 확정
    GetWorldTimerManager().SetTimer(
        ClosingSoonTimerHandle,
        this,
        &ThisClass::ChangeNextCheckoutZoneState,
        ClosingSoonDuration,
        false
    );
}

TArray<TObjectPtr<ACheckoutZone>> ACheckoutManager::SelectRandomOpenCheckoutZones(int32 OpenCount) const
{
    TArray<TObjectPtr<ACheckoutZone>> Candidates;               // 후보 계산대
    TArray<TObjectPtr<ACheckoutZone>> SelectedCheckoutZones;    // 최종 선택 계산대

    // 존재하는 계산대만 후보에 추가
    for (ACheckoutZone* CheckoutZone : CheckoutZones)
    {
        if (IsValid(CheckoutZone))
        {
            Candidates.Add(CheckoutZone);
        }
    }

    // 최대 오픈 개수는 월드에 존재하는 계산대 수만큼
    const int32 ClampedOpenCount = FMath::Clamp(OpenCount, 0,Candidates.Num());

    SelectedCheckoutZones.Reserve(ClampedOpenCount);

    // 서로 다른 계산대만 선택될 수 있도록
    // 같은 인덱스 중복 방지
    for (int32 i = 0; i < ClampedOpenCount; ++i)
    {
        const int32 RandomIndex = FMath::RandRange(0, Candidates.Num() - 1);

        SelectedCheckoutZones.Add(Candidates[RandomIndex]);

        // 선택된 계산대는 후보에서 제거
        Candidates.RemoveAtSwap(RandomIndex);
    }

    return SelectedCheckoutZones;
}

void ACheckoutManager::ChangeNextCheckoutZoneState()
{
    if (!HasAuthority())
    {
        return;
    }

    // 월드에 계산대가 하나라도 존재하는지
    if (CheckoutZones.IsEmpty())
    {
        return;
    }

    // 각 계산대가 모두 유효한지
    for (ACheckoutZone* CheckoutZone : CheckoutZones)
    {
        if (!IsValid(CheckoutZone))
        {
            return;
        }
    }

    for (ACheckoutZone* CheckoutZone : CheckoutZones)
    {
        if (!IsValid(CheckoutZone))
        {
            continue;
        }

        // 다음 턴에 오픈될 계산대인지
        const bool bShouldBeOpen = RandomOpenCheckoutZones.Contains(CheckoutZone);

        // 상태 변경 확정
        if (bShouldBeOpen)
        {
            CheckoutZone->SetCheckoutZoneState(ECheckoutZoneState::Open);
        }
        else
        {
            CheckoutZone->SetCheckoutZoneState(ECheckoutZoneState::Closed);
        }
    }

    // 랜덤 선택된 계산대 비우기
    RandomOpenCheckoutZones.Empty();

    // 다음 Step으로 넘어가기
    if (bIsCheckoutRotationRunning)
    {
        ++CurrentRotationStepIndex;
        ScheduleNextRotationStepWarning();
    }
}


