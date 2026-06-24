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
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Multicast Message"));
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

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Loaded!"));
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

    // 마우스 커서 위치에서 지정한 콜리전 채널 기준으로 광선 발사
    // 부딪힌 위치 정보를 HitResult에 저장, 없다면 false 반환
    FHitResult HitResult;

    bool bHit = PlayerController->GetHitResultUnderCursorByChannel(
        UEngineTypes::ConvertToTraceType(ECC_Visibility),
        false,
        HitResult
    );
    if (!bHit) return;


    // 시작점, 마우스 위치 기준으로 방향 구하기
    FVector Start = OwnerPawn->GetActorLocation();
    FVector Target = HitResult.ImpactPoint;

    Target.Z = Start.Z;

    FVector AimDirection = Target - Start;
    AimDirection.Z = 0.f;

    if (AimDirection.IsNearlyZero()) return;

    //// 조준선을 Start위치로 올리면서 어긋나는 문제 있음
    // 보여주는거랑 실제 판정은 다르게 해야함

    // 조준선이 GrabRange 넘어가면 Min 으로 자르기
    float Distance = AimDirection.Size();
    float ActualDistance = FMath::Min(Distance, GrabRange);

    CachedAimDirection = AimDirection.GetSafeNormal();
    CachedAimTargetLocation = Start + CachedAimDirection * ActualDistance;

    ShowMouseAim(Start);
}

void UCartGrabComponent::ShowMouseAim(const FVector& Start)
{
    // 임시로 디버그 라인 그려서 보여주는 중
    UWorld* World = GetWorld();
    if (!IsValid(World)) return;

    DrawDebugLine(
        World,
        Start,
        CachedAimTargetLocation,
        FColor::Green,
        false,
        AimUpdateInterval,
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

    // 쿨다운 적용
    SetGrabCooldown(Duration);
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

    bool bHit = World->SweepSingleByChannel(
        Hit,
        Start,
        End,
        FQuat::Identity,
        ECC_GameTraceChannel2,
        FCollisionShape::MakeSphere(GrabRadius),
        Params
    );

    // 맞은게 없다면 연출만하고 종료
    if (!bHit)
    {
        float Distance = FVector::Distance(Start, End);
        float Duration = Distance / GrabSpeed;
        Multicast_StretchGrab(Start, End, Duration);
        SetGrabCooldown(Duration);
        return false;
    }

    return true;
}

void UCartGrabComponent::SetGrabCooldown(float Duration)
{
    bCanGrab = false;

    // 쿨다운 적용하기
    UWorld* World = GetWorld();
    if (!IsValid(World)) return;

    World->GetTimerManager().SetTimer(
        GrabFinishTimer,
        this,
        &ThisClass::HandleFinishGrab,
        Duration,
        false
    );
}
