#include "MapGimmickManager/WaterHoleGimmick/WaterHoleGimmick.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Cart/CartPawn.h"

AWaterHoleGimmick::AWaterHoleGimmick()
{
	PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
    BoxCollision->SetupAttachment(GetRootComponent());

    WaterHoleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterHoleMesh"));
    WaterHoleMesh->SetupAttachment(BoxCollision);

    SpinningCharacter = nullptr;
}

void AWaterHoleGimmick::BeginPlay()
{
	Super::BeginPlay();

    if (HasAuthority())
    {
        BoxCollision->OnComponentBeginOverlap.AddDynamic(this, &AWaterHoleGimmick::OnWaterHoleBeginOverlap);
    }
}

void AWaterHoleGimmick::OnWaterHoleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if(!OtherActor || !OtherActor->Implements<USlideAffectable>())
    {
        return;
    }

    const float Yaw = FMath::RandBool() ? 60.f : -60.f;
    ISlideAffectable::Execute_ApplySlip(OtherActor, EffectDuration, Yaw);
}

void AWaterHoleGimmick::OnWaterHoleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}

void AWaterHoleGimmick::UpdateCharacterSpin()
{
    if (!IsValid(SpinningCharacter))
    {
        GetWorldTimerManager().ClearTimer(SpinTimerHandle);
        return;
    }

    SpinElapsed += TimerInterval;
    UCharacterMovementComponent* MoveComp = SpinningCharacter->GetCharacterMovement();

    if (SpinElapsed <= SpinDuration)
    {
        FRotator CurrentRot = SpinningCharacter->GetActorRotation();
        FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRotation, TimerInterval, 15.0f);
        SpinningCharacter->SetActorRotation(NewRot);
    }
    else if (SpinElapsed > SpinDuration && !SpinningCharacter->bUseControllerRotationYaw)
    {
        SpinningCharacter->bUseControllerRotationYaw = true;
        if (IsValid(MoveComp))
        {
            MoveComp->bOrientRotationToMovement = true;
        }
    }

    if (SpinElapsed >= EffectDuration)
    {
        if (IsValid(MoveComp))
        {
            MoveComp->GroundFriction = OriginalGroundFriction;
            MoveComp->BrakingDecelerationWalking = OriginalBrakingDeceleration;
        }

        SpinningCharacter->bUseControllerRotationYaw = true;
        if (IsValid(MoveComp))
        {
            MoveComp->bOrientRotationToMovement = true;
        }

        UE_LOG(LogTemp, Log, TEXT("스핀 종료"));

        GetWorldTimerManager().ClearTimer(SpinTimerHandle);

        SpinningCharacter = nullptr;
    }

}

void AWaterHoleGimmick::StopCharacterSpin()
{
    GetWorldTimerManager().ClearTimer(SpinTimerHandle);

    if (IsValid(SpinningCharacter))
    {
        UCharacterMovementComponent* MoveComp = SpinningCharacter->GetCharacterMovement();
        if (IsValid(MoveComp))
        {
            MoveComp->GroundFriction = OriginalGroundFriction;
            MoveComp->BrakingDecelerationWalking = OriginalBrakingDeceleration;

            MoveComp->bOrientRotationToMovement = true;
        }

        UE_LOG(LogTemp, Log, TEXT("[WaterHoleGimmick] %s 캐릭터 회전 종료"), *SpinningCharacter->GetName());

        SpinningCharacter = nullptr;
    }
}

