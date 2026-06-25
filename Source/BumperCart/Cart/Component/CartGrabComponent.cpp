// Fill out your copyright notice in the Description page of Project Settings.


#include "Cart/Component/CartGrabComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Product/ProductBase.h"
#include "CartLoadComponent.h"
#include "TimerManager.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"


UCartGrabComponent::UCartGrabComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;

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
    SocketName = TEXT("ProductSocket");
}

void UCartGrabComponent::SetupInput()
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
            if (GrabMappingContext)
            {
                Subsystem->AddMappingContext(GrabMappingContext, MappingPriority);
            }

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

void UCartGrabComponent::CreateVisualComponents()
{
    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor)) return;

    USceneComponent* Root = OwnerActor->GetRootComponent();
    if (!IsValid(Root)) return;

    if (!ArmSpline)
    {
        // OwnerActor 에 로봇손 팔 부분 부착
        ArmSpline = NewObject<USplineMeshComponent>(OwnerActor, TEXT("RobotArmSplineMesh"));
        ArmSpline->SetMobility(EComponentMobility::Movable);
        ArmSpline->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ArmSpline->SetVisibility(false);

        if (ArmMeshAsset)
        {
            ArmSpline->SetStaticMesh(ArmMeshAsset);
        }

        ArmSpline->SetForwardAxis(ESplineMeshAxis::X);
        ArmSpline->SetupAttachment(Root);

        OwnerActor->AddInstanceComponent(ArmSpline);
        ArmSpline->RegisterComponent();
    }

    if (!Hand)
    {
        // OwnerActor 에 로봇손 손 부분 부착
        Hand = NewObject<UStaticMeshComponent>(OwnerActor, TEXT("RobotArmHandMesh"));
        Hand->SetMobility(EComponentMobility::Movable);
        Hand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Hand->SetVisibility(false);

        if (HandMeshAsset)
        {
            Hand->SetStaticMesh(HandMeshAsset);
        }

        Hand->SetupAttachment(Root);

        OwnerActor->AddInstanceComponent(Hand);
        Hand->RegisterComponent();
    }
}

void UCartGrabComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (VisualState == EGrabVisualState::None) return;

    VisualElapsedTime += DeltaTime;

    if (VisualState == EGrabVisualState::Extending)
    {
        // 진행 정도 확인
        float Progress = VisualElapsedTime / VisualReachDuration;

        FVector CurrentLocation = FMath::Lerp(GetGrabStartLocation(), VisualTargetLocation, Progress);
        UpdateGrabVisual(CurrentLocation);

        // 상품 or 끝에 도달하면 로봇손 회수
        if (Progress >= 1.f)
        {
            AttachProductToHand();

            VisualElapsedTime = 0.f;
            VisualState = EGrabVisualState::Returning;
        }
    }
    else if (VisualState == EGrabVisualState::Returning)
    {
        // 진행 정도 확인
        float Progress = VisualElapsedTime / VisualReachDuration;

        FVector CurrentLocation = FMath::Lerp(GetGrabStartLocation(), VisualTargetLocation, 1.f - Progress);
        UpdateGrabVisual(CurrentLocation);

        if (Progress >= 1.f)
        {
            FinishGrabVisual();
        }
    }
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

void UCartGrabComponent::Multicast_PlayGrab_Implementation(AProductBase* Product, FVector_NetQuantize Start,
    FVector_NetQuantize Target)
{
    PlayGrabVisual(Product, Start, Target);
}

void UCartGrabComponent::HandleFinishGrab()
{
    bCanGrab = true;

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
    if (CachedAimDirection.IsNearlyZero()) return;
    if (GrabSpeed <= 0.f) return;

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
    Multicast_PlayGrab(Product, StartLocation, TargetLocation);

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
        Multicast_PlayGrab(nullptr, Start, End);
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

void UCartGrabComponent::UpdateGrabVisual(const FVector& Location)
{
    if (!ArmSpline || !Hand) return;

    FTransform MeshTransform = ArmSpline->GetComponentTransform();

    FVector Start = MeshTransform.InverseTransformPosition(GetGrabStartLocation());
    FVector End = MeshTransform.InverseTransformPosition(Location);

    FVector Direction = End - Start;
    FVector NormalizedDirection = Direction.GetSafeNormal();

    if (NormalizedDirection == FVector::ZeroVector) return;

    ArmSpline->SetStartAndEnd(
        Start,
        Direction,
        End,
        Direction,
        true
        );

    FVector WorldDirection = (Location - VisualStartLocation).GetSafeNormal();

    Hand->SetWorldLocation(Location);
    Hand->SetWorldRotation(WorldDirection.Rotation());
}

void UCartGrabComponent::PlayGrabVisual(AProductBase* Product, const FVector& Start, const FVector& Target)
{
    CreateVisualComponents();
    if (!ArmSpline || !Hand) return;

    GrabbedProduct = Product;
    VisualStartLocation = Start;
    VisualTargetLocation = Target;

    // Duration은 거리 / 속도의 절반, 도달, 회수를 해야하기 때문
    float Distance = FVector::Distance(VisualStartLocation, VisualTargetLocation);
    VisualReachDuration = Distance / FMath::Max(GrabSpeed, 1.f) / 2.f;
    VisualReturnDuration = VisualReachDuration;

    VisualElapsedTime = 0.f;
    VisualState = EGrabVisualState::Extending;

    ArmSpline->SetVisibility(true);
    Hand->SetVisibility(true);

    UpdateGrabVisual(Start);

    SetComponentTickEnabled(true);
}

void UCartGrabComponent::FinishGrabVisual()
{
    VisualState = EGrabVisualState::None;
    VisualElapsedTime = 0.f;

    if (ArmSpline)
    {
        ArmSpline->SetVisibility(false);
    }
    if (Hand)
    {
        Hand->SetVisibility(false);
    }

    GrabbedProduct.Reset();

    SetComponentTickEnabled(false);
}

void UCartGrabComponent::AttachProductToHand()
{
    AProductBase* Product = GrabbedProduct.Get();
    if (!IsValid(Product) || !Hand) return;

    Product->AttachToGrabHand(Hand, SocketName);
}

FVector UCartGrabComponent::GetGrabStartLocation() const
{
    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor)) return {};

    return OwnerActor->GetActorLocation();
}
