// Fill out your copyright notice in the Description page of Project Settings.


#include "Cart/Component/CartGrabComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Product/ProductBase.h"
#include "CartLoadComponent.h"


UCartGrabComponent::UCartGrabComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

    SetIsReplicatedByDefault(true);

    MappingPriority = 0;

    GrabbedProduct.Reset();

    // 조준선 관련
    CachedAimDirection = FVector::ZeroVector;
    CachedAimTargetLocation = FVector::ZeroVector;
    AimUpdateInterval = 0.1f;

    // 로봇손 관련
    GrabSpeed = 300.f;
    GrabRange = 500.f;
    GrabRadius = 30.f;
    bCanGrab = true;
    SocketName = TEXT("GrabPoint");
}

void UCartGrabComponent::SetupInput(UInputComponent* PlayerInputComponent)
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());

    // 다른 클라이언트의 Pawn이면 등록 X
    if (!IsValid(OwnerPawn) || !OwnerPawn->IsLocallyControlled()) return;


    if (APlayerController* PlayerController = Cast<APlayerController>(OwnerPawn->GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            // IMC 등록
            Subsystem->AddMappingContext(GrabMappingContext, 0);

            // IA 등록
            if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
            {
                if (GrabAction)
                {
                    EnhancedInputComponent->BindAction(
                        GrabAction,
                        ETriggerEvent::Started,
                        this,
                        &ThisClass::RequestGrab
                    );
                }
            }
        }
    }

    StartAimTimer();
}

void UCartGrabComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopAimTimer();

    Super::EndPlay(EndPlayReason);
}

void UCartGrabComponent::StartAimTimer()
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!IsValid(OwnerPawn) || !OwnerPawn->IsLocallyControlled()) return;

    UWorld* World = GetWorld();
    if (!IsValid(World)) return;

    World->GetTimerManager().SetTimer(
        GrabAimUpdateTimer,
        this,
        &ThisClass::UpdateGrabAim,
        AimUpdateInterval,
        true
    );

    UpdateGrabAim();
}

void UCartGrabComponent::StopAimTimer()
{
    UWorld* World = GetWorld();
    if (!IsValid(World)) return;

    World->GetTimerManager().ClearTimer(GrabAimUpdateTimer);

    CachedAimDirection = FVector::ZeroVector;
    CachedAimTargetLocation = FVector::ZeroVector;
}

void UCartGrabComponent::Multicast_StretchGrab_Implementation(FVector_NetQuantize StartLocation, FVector_NetQuantize EndLocation, float Duration)
{

}

void UCartGrabComponent::HandleFinishGrab()
{
    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority()) return;

    AProductBase* Product = GrabbedProduct.Get();
    GrabbedProduct.Reset();
    if (!IsValid(Product)) return;

    // 컴포넌트 가져오기 실패하면 되돌리기
    UCartLoadComponent* LoadComp = OwnerActor->FindComponentByClass<UCartLoadComponent>();
    if (!IsValid(LoadComp))
    {
        Product->SetProductState(EProductState::Display);
        return;
    }

    // 상품 적재에 실패해도 되돌리기
    if (!LoadComp->TryAddProduct(Product))
    {
        Product->SetProductState(EProductState::Display);
        return;
    }
    bCanGrab = true;
}

void UCartGrabComponent::UpdateGrabAim()
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!IsValid(OwnerPawn))
    {
        StopAimTimer();
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(OwnerPawn->GetController());
    if (!IsValid(PlayerController))
    {
        StopAimTimer();
        return;
    }

    FHitResult HitResult;

    bool bHit = PlayerController->GetHitResultUnderCursorByChannel(
        UEngineTypes::ConvertToTraceType(ECC_Visibility),
        false,
        HitResult
    );

    if (!bHit) return;

    FVector Start = OwnerPawn->GetActorLocation();
    FVector Target = HitResult.ImpactPoint;

    Target.Z = Start.Z;

    FVector AimDirection = Target - Start;
    AimDirection.Z = 0.f;

    if (AimDirection.IsNearlyZero()) return;

    CachedAimDirection = AimDirection.GetSafeNormal();
    CachedAimTargetLocation = Target;

    ShowMouseAim(Start, Target);
}

void UCartGrabComponent::ShowMouseAim(const FVector& Start, const FVector& End)
{
    // 임시로 디버그 라인 그려서 보여주기
    UWorld* World = GetWorld();
    if (!IsValid(World)) return;

    DrawDebugLine(
        World,
        Start,
        End,
        FColor::Green,
        false,
        AimUpdateInterval + 0.02f,
        0,
        5.f
    );
}

void UCartGrabComponent::RequestGrab()
{
    // 클라이언트에서 계산한 값이므로 인자로 넘겨야 함
    Server_GrabProduct(CachedAimDirection);
}

void UCartGrabComponent::Server_GrabProduct_Implementation(FVector_NetQuantizeNormal AimDirection)
{
    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority()) return;
    if (!bCanGrab) return;

    // Trace, Sweep 으로 판정, 상품 찾기
    FHitResult Hit;
    if (!PerformGrabTrace(AimDirection, Hit)) return;


    // 상품을 Grabbed 상태로 변경
    AProductBase* Product = Cast<AProductBase>(Hit.GetActor());
    if (!IsValid(Product)) return;

    if (!Product->TrySetGrabbed()) return;

    // 붙잡은 Proudct WeakPtr 갱신
    GrabbedProduct = Product;


    // 속도와 거리를 이용해서 시간 구하기
    FVector StartLocation = OwnerActor->GetActorLocation();
    FVector TargetLocation = Product->GetActorLocation();

    float Distance = FVector::Distance(StartLocation, TargetLocation);
    float Duration = Distance / GrabSpeed;


    // 해당 시간만큼 모든 클라이언트에게 해당 클라이언트 손 뻗는 연출 요청하기
    Multicast_StretchGrab(StartLocation, TargetLocation, Duration);


    // 쿨다운 적용하기
    UWorld* World = GetWorld();
    if (!IsValid(World)) return;

    World->GetTimerManager().SetTimer(
        GrabFinishTimer,
        this,
        &ThisClass::HandleFinishGrab,
        Duration,
        false);
}

bool UCartGrabComponent::PerformGrabTrace(FVector_NetQuantizeNormal AimDirection, FHitResult& Hit)
{
    UWorld* World = GetWorld();
    if (!IsValid(World)) return false;

    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor)) return false;

    FVector Start = OwnerActor->GetActorLocation();
    FVector End = Start + AimDirection * GrabRange;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerActor);

    World->SweepSingleByChannel(
        Hit,
        Start,
        End,
        FQuat::Identity,
        ECC_GameTraceChannel1,
        FCollisionShape::MakeSphere(GrabRadius),
        Params
    );


    return false;
}

