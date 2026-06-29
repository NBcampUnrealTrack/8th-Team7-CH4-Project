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
#include "Util/BCCollisionChannels.h"


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
    CachedAimDistance = 0.f;
    AimUpdateInterval = 0.1f;
    AimDashLength = 40.f;
    AimDashGap = 25.f;
    AimDashThickness = 1.f;

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
    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        Super::EndPlay(EndPlayReason);
        return;
    }

    World->GetTimerManager().ClearTimer(GrabFinishTimer);
    World->GetTimerManager().ClearTimer(TryGrabTimer);
    StopAimTimer();

    Super::EndPlay(EndPlayReason);
}

void UCartGrabComponent::EnsureVisualComponents()
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

    if (!VisualProductMesh)
    {
        VisualProductMesh = NewObject<UStaticMeshComponent>(OwnerActor, TEXT("VisualProductMesh"));
        VisualProductMesh->SetMobility(EComponentMobility::Movable);
        VisualProductMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        VisualProductMesh->SetVisibility(false);

        VisualProductMesh->SetupAttachment(Hand, SocketName);

        OwnerActor->AddInstanceComponent(VisualProductMesh);
        VisualProductMesh->RegisterComponent();

        if (IsValid(Hand))
        {
            VisualProductMesh->AttachToComponent(
                Hand,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                SocketName
            );
        }
    }
}

void UCartGrabComponent::EnsureAimDashMeshes(int32 RequiredCount)
{
    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor)) return;

    USceneComponent* Root = OwnerActor->GetRootComponent();
    if (!IsValid(Root)) return;

    // 필요한 만큼 반복 생성
    while (AimDashMeshes.Num() < RequiredCount)
    {
        int32 Index = AimDashMeshes.Num();

        FName ComponentName = *FString::Printf(TEXT("AimDashMesh_%d"), Index);

        USplineMeshComponent* DashMesh = NewObject<USplineMeshComponent>(OwnerActor, ComponentName);
        if (!IsValid(DashMesh)) return;

        DashMesh->SetMobility(EComponentMobility::Movable);
        DashMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        DashMesh->SetVisibility(false);
        DashMesh->SetForwardAxis(ESplineMeshAxis::X);

        DashMesh->SetStartScale(FVector2D(AimDashThickness, AimDashThickness));
        DashMesh->SetEndScale(FVector2D(AimDashThickness, AimDashThickness));

        if (AimDashMeshAsset)
        {
            DashMesh->SetStaticMesh(AimDashMeshAsset);
        }

        OwnerActor->AddInstanceComponent(DashMesh);
        DashMesh->RegisterComponent();

        AimDashMeshes.Add(DashMesh);
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
            VisualElapsedTime = 0.f;
            VisualState = EGrabVisualState::Returning;
        }
    }
    else if (VisualState == EGrabVisualState::Returning)
    {
        // 진행 정도 확인
        float Progress = VisualElapsedTime / VisualReturnDuration;

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

void UCartGrabComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetNetMode() == NM_DedicatedServer) return;

    // 최대 그랩 사거리만큼 미리 만들어두기
    float StepLength = AimDashLength + AimDashGap;
    if (StepLength > KINDA_SMALL_NUMBER)
    {
        int32 MaxDashCount = FMath::CeilToInt(GrabRange / StepLength);
        EnsureAimDashMeshes(MaxDashCount);
        HideAimDashMeshes();
    }
}

void UCartGrabComponent::StopAimTimer()
{
    UWorld* World = GetWorld();
    if (!IsValid(World)) return;

    World->GetTimerManager().ClearTimer(GrabAimUpdateTimer);

    CachedAimDirection = FVector::ZeroVector;
    CachedAimTargetLocation = FVector::ZeroVector;
}

void UCartGrabComponent::Multicast_PlayGrab_Implementation(FVector_NetQuantize Start,
    FVector_NetQuantize Target)
{
    PlayGrabVisual(Start, Target);
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

    // WorldLocation : 마우스가 위치한 3D월드 시작점, 보통 카메라 위치
    // WorldDirection : 마우스 위치를 향해 뻗어나가는 방향 벡터, 카메라에서 마우스 커서 방향으로 나가는 방향
    // 마우스 방향 광선 식 : WorldLocation + WorldDirection * T (T는 임의의 값)
    FVector WorldLocation, WorldDirection;

    bool bDeprojected = PlayerController->DeprojectMousePositionToWorld(WorldLocation, WorldDirection);
    if (!bDeprojected) return;

    FVector Start = OwnerPawn->GetActorLocation();
    float PawnZ = Start.Z;

    if (FMath::IsNearlyZero(WorldDirection.Z)) return;

    // 카트높이 = WorldLocation + WorldDirection * T (광선위의 카트의 높이과 Z가 같은 임의의 점)
    // T = (카트높이 - WorldLocation) / WorldDirection
    //   -> T는 카트와 같은 높이가 되는데 필요한 길이
    float T = (PawnZ - WorldLocation.Z) / WorldDirection.Z;
    if (T < 0.f) return;

    FVector Target = WorldLocation + WorldDirection * T;
    Target.Z = PawnZ;   // 오차 보정을 위한 대입

    FVector AimVector = Target - Start;
    if (AimVector.IsNearlyZero()) return;

    float Distance = AimVector.Size();
    float ActualDistance = FMath::Min(Distance, GrabRange);

    CachedAimDirection = AimVector.GetSafeNormal();
    CachedAimTargetLocation = Start + CachedAimDirection * ActualDistance;
    CachedAimDistance = ActualDistance;

    UpdateDashedAimVisual(Start, CachedAimTargetLocation);
}

void UCartGrabComponent::TryGrabProduct()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;

    AProductBase* Product = GrabbedProduct.Get();
    if (!IsValid(Product)) return;

    // 잡는데 실패하면 WeakPtr 초기화
    if (!Product->TrySetGrabbed())
    {
        GrabbedProduct.Reset();
        return;
    }

    Multicast_ShowVisualProductMesh(Product);
}

void UCartGrabComponent::RequestGrab()
{
    if (CachedAimDirection.IsNearlyZero()) return;
    if (CachedAimDistance <= KINDA_SMALL_NUMBER) return;
    if (GrabSpeed <= KINDA_SMALL_NUMBER) return;

    // 클라이언트에서 계산한 값이므로 인자로 넘겨야 함
    Server_GrabProduct(CachedAimDirection, CachedAimDistance);
}

void UCartGrabComponent::Server_GrabProduct_Implementation(FVector_NetQuantizeNormal AimDirection, float AimDistance)
{
    UWorld* World = GetWorld();
    if (!IsValid(World)) return;

    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority()) return;
    if (!bCanGrab) return;

    float ActualAimDistance = FMath::Clamp(AimDistance, 0.f, GrabRange);

    // Trace, Sweep 으로 판정, 상품 찾기
    FHitResult Hit;
    if (!PerformGrabTrace(AimDirection, ActualAimDistance, Hit)) return;

    AProductBase* Product = Cast<AProductBase>(Hit.GetActor());
    if (!IsValid(Product)) return;

    // 붙잡을 Proudct WeakPtr 확인
    GrabbedProduct = Product;

    // 속도와 거리를 이용해서 시간 구하기
    FVector StartLocation = OwnerActor->GetActorLocation();
    FVector TargetLocation = Product->GetActorLocation();

    float Distance = FVector::Distance(StartLocation, TargetLocation);
    float ReachDuration = Distance / FMath::Max(GrabSpeed, 1.f);

    // 상품과 접촉하는 시간에 잡을 수 있는지 체크하도록 타이머 설정
    World->GetTimerManager().SetTimer(
        TryGrabTimer,
        this,
        &ThisClass::TryGrabProduct,
        ReachDuration,
        false
    );

    // 모든 클라이언트에게 해당 클라이언트 손 뻗는 연출 요청하기
    Multicast_PlayGrab(StartLocation, TargetLocation);

    // 쿨다운 적용
    SetGrabCooldown(ReachDuration * 2.f);
}

bool UCartGrabComponent::PerformGrabTrace(FVector_NetQuantizeNormal AimDirection, float AimDistance, FHitResult& Hit)
{
    UWorld* World = GetWorld();
    if (!IsValid(World)) return false;

    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor)) return false;

    FVector Start = OwnerActor->GetActorLocation();
    FVector End = Start + FVector(AimDirection) * AimDistance;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerActor);

    bool bHit = World->SweepSingleByChannel(
        Hit,
        Start,
        End,
        FQuat::Identity,
        BCCollisionChannel::RobotHandGrabTrace,
        FCollisionShape::MakeSphere(GrabRadius),
        Params
    );

    // 맞은게 없다면 연출만하고 종료
    if (!bHit)
    {
        float ReachDuration = AimDistance / FMath::Max(GrabSpeed, 1.f);

        Multicast_PlayGrab(Start, End);
        SetGrabCooldown(ReachDuration * 2.f);
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

void UCartGrabComponent::PlayGrabVisual(const FVector& Start, const FVector& Target)
{
    EnsureVisualComponents();
    if (!ArmSpline || !Hand) return;

    VisualStartLocation = Start;
    VisualTargetLocation = Target;

    // Duration은 거리 / 속도,
    float Distance = FVector::Distance(VisualStartLocation, VisualTargetLocation);
    VisualReachDuration = Distance / FMath::Max(GrabSpeed, 1.f);
    VisualReturnDuration = VisualReachDuration;

    VisualElapsedTime = 0.f;
    VisualState = EGrabVisualState::Extending;

    ArmSpline->SetVisibility(true);
    Hand->SetVisibility(true);

    // 시작위치에서 한번 그려주기
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
    HideVisualProductMesh();

    SetComponentTickEnabled(false);
}

void UCartGrabComponent::ShowVisualProductMesh(AProductBase* Product)
{
    EnsureVisualComponents();

    if (!IsValid(Product) || !IsValid(VisualProductMesh)) return;

    UStaticMesh* ProductMesh = Product->GetProductMesh();
    if (!IsValid(ProductMesh)) return;

    VisualProductMesh->SetStaticMesh(ProductMesh);
    VisualProductMesh->SetVisibility(true);

    // 원본 상품은 가리기
    Product->SetActorHiddenInGame(true);
}

void UCartGrabComponent::HideVisualProductMesh()
{
    if (VisualProductMesh)
    {
        VisualProductMesh->SetVisibility(false);
        VisualProductMesh->SetStaticMesh(nullptr);
    }
}

void UCartGrabComponent::Multicast_ShowVisualProductMesh_Implementation(AProductBase* Product)
{
    ShowVisualProductMesh(Product);
}

void UCartGrabComponent::Multicast_HideVisualProductMesh_Implementation()
{
    HideVisualProductMesh();
}

FVector UCartGrabComponent::GetGrabStartLocation() const
{
    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor)) return {};

    return OwnerActor->GetActorLocation();
}


void UCartGrabComponent::UpdateDashedAimVisual(const FVector& Start, const FVector& End)
{
    FVector AimVector = End - Start;
    float TotalLength = AimVector.Size();

    if (TotalLength <= KINDA_SMALL_NUMBER)
    {
        HideAimDashMeshes();
        return;
    }

    // 메시 길이 + 간격 길이로 점선 하나당 길이 구하기
    float StepLength = AimDashLength + AimDashGap;
    if (StepLength <= KINDA_SMALL_NUMBER)
    {
        HideAimDashMeshes();
        return;
    }

    // 점선 하나당 길이로 필요한 개수 결정하기
    int32 RequiredCount = FMath::CeilToInt(TotalLength / StepLength);
    EnsureAimDashMeshes(RequiredCount);

    FVector Direction = AimVector / TotalLength;

    for (int32 i = 0; i < AimDashMeshes.Num(); ++i)
    {
        USplineMeshComponent* DashMesh = AimDashMeshes[i];
        if (!IsValid(DashMesh)) continue;

        // 기존에 만들어둔게 필요로 하는것보다 많다면 Visibility 끄기
        if (i >= RequiredCount)
        {
            DashMesh->SetVisibility(false);
            continue;
        }

        float StartDistance = i * StepLength;
        float EndDistance = FMath::Min(StartDistance + AimDashLength, TotalLength); // 끝부분 넘어가면 자르기

        FVector DashStart = Start + Direction * StartDistance;
        FVector DashEnd = Start + Direction * EndDistance;

        FTransform MeshTransform = DashMesh->GetComponentTransform();

        FVector LocalStart = MeshTransform.InverseTransformPosition(DashStart);
        FVector LocalEnd = MeshTransform.InverseTransformPosition(DashEnd);

        FVector LocalVector = LocalEnd - LocalStart;
        float LocalLength = LocalVector.Size();

        if (LocalLength <= KINDA_SMALL_NUMBER)
        {
            DashMesh->SetVisibility(false);
            continue;
        }

        FVector Tangent = LocalVector.GetSafeNormal() * LocalLength;

        DashMesh->SetStartAndEnd(
            LocalStart,
            Tangent,
            LocalEnd,
            Tangent,
            true
        );

        DashMesh->SetVisibility(true);
    }
}

void UCartGrabComponent::HideAimDashMeshes()
{
    for (USplineMeshComponent* DashMesh : AimDashMeshes)
    {
        if (IsValid(DashMesh))
        {
            DashMesh->SetVisibility(false);
        }
    }
}
