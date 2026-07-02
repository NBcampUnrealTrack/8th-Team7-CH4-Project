#include "Checkout/CheckoutBarrier.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

ACheckoutBarrier::ACheckoutBarrier()
{
 	PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    BarrierMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrierMesh"));
    BarrierMesh->SetupAttachment(SceneRoot);

    // 에디터에서는 표시, 게임 시작 시 표시 X
    BarrierMesh->SetVisibility(true, true);
    BarrierMesh->SetHiddenInGame(true);

    // 메시 충돌 설정
    BarrierMesh->SetCollisionObjectType(ECC_WorldStatic);
    BarrierMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    BarrierMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    BarrierMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BarrierMesh->SetGenerateOverlapEvents(false);
    BarrierMesh->CanCharacterStepUpOn = ECB_No; // 발판 방지
}

void ACheckoutBarrier::BeginPlay()
{
    Super::BeginPlay();

    InitializeBarrierMaterial();

    bIsBarrierEnabled = false;

    CurrentRevealValue = 0.0f;
    RevealStartValue = 0.0f;
    RevealTargetValue = 0.0f;

    SetRevealValue(0.0f);
    SetBarrierCollisionEnabled(false);

    if (IsValid(BarrierMesh))
    {
        BarrierMesh->SetHiddenInGame(true);
    }
}

void ACheckoutBarrier::SetBarrierEnabled(bool bIsEnabled)
{
    if (!IsValid(BarrierMesh))
    {
        return;
    }

    if (bIsBarrierEnabled == bIsEnabled)
    {
        return;
    }

    bIsBarrierEnabled = bIsEnabled;

    if (bIsBarrierEnabled)
    {
        // 연출 시작 전에 메시를 먼저 표시
        BarrierMesh->SetHiddenInGame(false);

        // 메시 생성 중에는 통과 가능
        SetBarrierCollisionEnabled(false);

        // 아래에서 위로 생성
        StartRevealAnimation(1.0f);
    }
    else
    {
        // 제거 연출 시작과 동시에 충돌 해제
        SetBarrierCollisionEnabled(false);

        // 위에서 아래로 제거
        StartRevealAnimation(0.0f);
    }
}

bool ACheckoutBarrier::IsBarrierEnabled() const
{
    return bIsBarrierEnabled;
}

// ------------------------------------------------------------
// 머티리얼
// ------------------------------------------------------------

void ACheckoutBarrier::InitializeBarrierMaterial()
{
    if (!IsValid(BarrierMesh))
    {
        return;
    }

    BarrierMID = BarrierMesh->CreateAndSetMaterialInstanceDynamic(0);

    if (!IsValid(BarrierMID))
    {
        return;
    }

    SetRevealValue(0.0f);
}

void ACheckoutBarrier::StartRevealAnimation(float TargetReveal)
{
    GetWorldTimerManager().ClearTimer(RevealTimerHandle);

    RevealStartValue = CurrentRevealValue;
    RevealTargetValue = FMath::Clamp(TargetReveal, 0.0f, 1.0f);

    if (!IsValid(BarrierMID) || RevealDuration <= KINDA_SMALL_NUMBER)
    {
        SetRevealValue(RevealTargetValue);
        FinishRevealAnimation();
        return;
    }

    if (FMath::IsNearlyEqual(RevealStartValue, RevealTargetValue))
    {
        SetRevealValue(RevealTargetValue);
        FinishRevealAnimation();
        return;
    }

    RevealAnimationStartTime = GetWorld()->GetTimeSeconds();

    GetWorldTimerManager().SetTimer(
        RevealTimerHandle,
        this,
        &ThisClass::UpdateRevealAnimation,
        FMath::Max(RevealUpdateInterval, 0.001f),
        true
    );

    // 한 프레임 기다리지 않고 즉시 첫 값 적용
    UpdateRevealAnimation();
}

void ACheckoutBarrier::UpdateRevealAnimation()
{
    if (!IsValid(GetWorld()))
    {
        return;
    }

    const float ElapsedTime = GetWorld()->GetTimeSeconds() - RevealAnimationStartTime;

    const float Alpha = FMath::Clamp(ElapsedTime / RevealDuration, 0.0f, 1.0f);

    // 초반은 빠르고 끝부분은 부드럽게 감속
    const float EasedAlpha = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);

    const float NewRevealValue = FMath::Lerp(RevealStartValue, RevealTargetValue, EasedAlpha);

    SetRevealValue(NewRevealValue);

    if (Alpha >= 1.0f)
    {
        GetWorldTimerManager().ClearTimer(RevealTimerHandle);
        FinishRevealAnimation();
    }
}

void ACheckoutBarrier::FinishRevealAnimation()
{
    SetRevealValue(RevealTargetValue);

    if (bIsBarrierEnabled && RevealTargetValue >= 1.0f)
    {
        // 벽이 완전히 올라온 뒤 충돌 활성화
        SetBarrierCollisionEnabled(true);
        return;
    }

    if (!bIsBarrierEnabled && RevealTargetValue <= 0.0f)
    {
        SetBarrierCollisionEnabled(false);

        if (IsValid(BarrierMesh))
        {
            // 제거 연출이 끝난 뒤 메시 숨김
            BarrierMesh->SetHiddenInGame(true);
        }
    }
}

void ACheckoutBarrier::SetRevealValue(float RevealValue)
{
    CurrentRevealValue = FMath::Clamp(RevealValue, 0.0f, 1.0f);

    if (!IsValid(BarrierMID))
    {
        return;
    }

    BarrierMID->SetScalarParameterValue(TEXT("Reveal"), CurrentRevealValue);
}

// ------------------------------------------------------------
// Collision
// ------------------------------------------------------------

void ACheckoutBarrier::SetBarrierCollisionEnabled(bool bIsEnabled)
{
    if (!IsValid(BarrierMesh))
    {
        return;
    }

    BarrierMesh->SetCollisionEnabled(
        bIsEnabled
        ? ECollisionEnabled::QueryOnly
        : ECollisionEnabled::NoCollision
    );
}
