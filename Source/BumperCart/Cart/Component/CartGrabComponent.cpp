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
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/DecalComponent.h"


namespace NiagaraParamName
{
    static const FName NAME_StartPosition(TEXT("User.StartPosition"));
    static const FName NAME_EndPosition(TEXT("User.EndPosition"));
    static const FName NAME_ActiveDistance(TEXT("User.ActiveDistance"));
    static const FName NAME_DotSpacing(TEXT("User.DotSpacing"));
    static const FName NAME_DotSize(TEXT("User.DotSize"));
}


UCartGrabComponent::UCartGrabComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;

    SetIsReplicatedByDefault(true);

    MappingPriority = 0;

    GrabbedProduct.Reset();

    // 조준선 갱신 관련
    CachedAimDirection = FVector::ZeroVector;
    CachedAimTargetLocation = FVector::ZeroVector;
    CachedAimDistance = 0.f;
    AimUpdateInterval = 0.05f;

    // 조준선 나이아가라 변수
    AimDotSpacing = 45.f;
    AimDotSize = 20.f;
    NiagaraHeightOffset = 10.f;

    // 로봇손 관련
    GrabSpeed = 300.f;
    GrabRange = 500.f;
    GrabRadius = 10.f;
    bCanGrab = true;
    SocketName = TEXT("ProductSocket");
}

void UCartGrabComponent::SetupInput()
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());

    // 다른 클라이언트의 Pawn이면 등록 X
    if (!IsValid(OwnerPawn) || !OwnerPawn->IsLocallyControlled()) return;

    // 입력 바인딩
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

    // 로컬 플레이어 조준선 생성
    EnsureAimNiagara();
    EnsureAimDecal();

    // 조준선 활성화
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

void UCartGrabComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 서버는 판정 처리
    AActor* OwnerActor = GetOwner();
    if (IsValid(OwnerActor) && OwnerActor->HasAuthority() && bServerGrab)
    {
        TickGrab(DeltaTime);
    }

    // 클라이언트는 연출 처리만 하기
    if (VisualState == EGrabVisualState::None || GetNetMode() == NM_DedicatedServer) return;

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
    SetAimVisual(true);
}

void UCartGrabComponent::BeginPlay()
{
    Super::BeginPlay();

    // 데디케이트 서버가 아니라면 모든 Pawn의 로봇손 생성
    if (GetNetMode() != NM_DedicatedServer)
    {
        EnsureVisualComponents();
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
    if (GetNetMode() != NM_DedicatedServer)
    {
        PlayGrabVisual(Start, Target);
    }
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
    Start.Z = 10.f; // 강제로 상품 높이로 보정
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

    UpdateAimNiagaraVisual(Start, CachedAimTargetLocation);
    UpdateAimDecal(CachedAimTargetLocation);
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

    FVector Direction = FVector(AimDirection).GetSafeNormal();
    if (Direction.IsNearlyZero()) return;

    float ActualAimDistance = FMath::Clamp(AimDistance, 0.f, GrabRange);
    if (ActualAimDistance <= KINDA_SMALL_NUMBER) return;

    FVector Start = OwnerActor->GetActorLocation();
    Start.Z = 10.f;
    FVector End = Start + Direction * ActualAimDistance;

    // 그랩 끝나기 전까진 사용 불가능, 
    bCanGrab = false;
    GrabbedProduct.Reset();

    // 서버 그랩 세팅
    bServerGrab = true;
    ServerGrabStartLocation = Start;
    ServerGrabDirection = Direction;
    ServerGrabMaxDistance = ActualAimDistance;
    ServerGrabCurrentDistance = 0.f;

    // 타이머 한번 제거하고 가기
    World->GetTimerManager().ClearTimer(GrabFinishTimer);
    World->GetTimerManager().ClearTimer(TryGrabTimer);

    // 클라이언트는 연출 시작
    Multicast_PlayGrab(Start, End);

    // 서버도 그랩 판정 위해 Tick 켜기
    RefreshGrabTick();
}

bool UCartGrabComponent::PerformGrabTrace(FVector_NetQuantizeNormal AimDirection, float AimDistance, FHitResult& Hit)
{
    UWorld* World = GetWorld();
    if (!IsValid(World)) return false;

    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor)) return false;

    FVector Start = OwnerActor->GetActorLocation();
    Start.Z = 10.f; // 카트가 중심점보다 아래있어서 강제로 보정, 상품들이 10.f에 위치함
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

void UCartGrabComponent::RefreshGrabTick()
{
    // 서버에서 틱을 필요로 하는지 체크
    bool bServerTick = GetOwner() && GetOwner()->HasAuthority() && bServerGrab;

    // 클라이언트에서 틱을 필요로 하는지 체크
    bool bVisualTick = VisualState != EGrabVisualState::None;

    // 둘중에 하나라도 필요로하면 틱 활성화
    SetComponentTickEnabled(bServerTick || bVisualTick);
}

void UCartGrabComponent::TickGrab(float DeltaTime)
{
    // 이전값과 현재값 세팅
    float PreviousDistance = ServerGrabCurrentDistance;
    ServerGrabCurrentDistance = FMath::Min(ServerGrabCurrentDistance + GrabSpeed * DeltaTime, ServerGrabMaxDistance);

    FVector GrabLocation;
    if (TrySweepGrab(PreviousDistance, ServerGrabCurrentDistance, GrabLocation))
    {
        bServerGrab = false;

        // 서버에서 회수 처리
        StartGrabReturn(ServerGrabCurrentDistance);

        // 모든 클라이언트에 회수 처리 및 회수 위치 설정
        Multicast_StartGrabReturn(GrabLocation, GrabbedProduct.Get());

        RefreshGrabTick();
        return;
    }

    // 끝까지 검사했는데 못잡으면 최종 지점에서 회수 시작
    if (ServerGrabCurrentDistance >= ServerGrabMaxDistance - KINDA_SMALL_NUMBER)
    {
        bServerGrab = false;

        StartGrabReturn(ServerGrabMaxDistance);

        RefreshGrabTick();
    }
}

bool UCartGrabComponent::TrySweepGrab(float FromDistance, float ToDistance, FVector& OutLocation)
{
    UWorld* World = GetWorld();
    if (!IsValid(World)) return false;

    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor)) return false;

    // 직전 위치, 현재 위치를 Sweep 할 것
    FVector Start = ServerGrabStartLocation + ServerGrabDirection * FromDistance;
    FVector End = ServerGrabStartLocation + ServerGrabDirection * ToDistance;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerActor);

    FHitResult Hit;
    bool bHit = World->SweepSingleByChannel(
        Hit,
        Start,
        End,
        FQuat::Identity,
        BCCollisionChannel::RobotHandGrabTrace,
        FCollisionShape::MakeSphere(GrabRadius),
        Params
    );

    if (!bHit) return false;

    AActor* HitActor = Hit.GetActor();
    AProductBase* Product = Cast<AProductBase>(HitActor);

    if (IsValid(Product) && Product->TrySetGrabbed())
    {
        GrabbedProduct = Product;

        float HitDistance = FVector::DotProduct(Hit.ImpactPoint - ServerGrabStartLocation, ServerGrabDirection);
        float ActualDistance = FMath::Clamp(HitDistance, FromDistance, ToDistance);

        OutLocation = ServerGrabStartLocation + ServerGrabDirection * ActualDistance;

        return true;
    }

    // 상품이 아닌 다른것과 충돌했다면 벽이나 카트에 충돌한거라 회수해야 함
    if (IsValid(HitActor))
    {
        // 회수 처리 추가 필요
        return false;
    }

    return false;
}

void UCartGrabComponent::StartGrabReturn(float ReturnDistance)
{
    UWorld* World = GetWorld();
    if (!IsValid(World)) return;

    float ReturnDuration = ReturnDistance / FMath::Max(GrabSpeed, 1.f);

    World->GetTimerManager().SetTimer(
        GrabFinishTimer,
        this,
        &ThisClass::HandleFinishGrab,
        ReturnDuration,
        false
    );
}

void UCartGrabComponent::Multicast_StartGrabReturn_Implementation(FVector_NetQuantize ReturnLocation, AProductBase* Product)
{
    if (GetNetMode() == NM_DedicatedServer) return;

    EnsureVisualComponents();
    if (!ArmSpline || !Hand) return;

    // 붙잡기 성공했다면 메시 보여주기
    if (IsValid(Product))
    {
        ShowVisualProductMesh(Product);
    }

    // 회수로 변경하고 위치 조정하기
    VisualTargetLocation = ReturnLocation;
    VisualElapsedTime = 0.f;
    VisualReturnDuration = FVector::Distance(GetGrabStartLocation(), ReturnLocation) / FMath::Max(GrabSpeed, 1.f);

    VisualState = EGrabVisualState::Returning;

    UpdateGrabVisual(ReturnLocation);

    RefreshGrabTick();
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

    // 내 Pawn 이라면 조준선 끄기
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (IsValid(OwnerPawn) && OwnerPawn->IsLocallyControlled())
    {
        SetAimVisual(false);
    }

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

    RefreshGrabTick();
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

    RefreshGrabTick();

    // 내 Pawn 이라면 조준선 켜기
    // 마우스 위치 기준으로 조준선 한번 갱신하고 Visual 켜야함
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (IsValid(OwnerPawn) && OwnerPawn->IsLocallyControlled())
    {
        UpdateGrabAim();
        SetAimVisual(true);
    }
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
    // 데디케이트 서버는 연출 필요없음
    if (GetNetMode() != NM_DedicatedServer)
    {
        ShowVisualProductMesh(Product);
    }
}

FVector UCartGrabComponent::GetGrabStartLocation() const
{
    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor)) return {};

    return OwnerActor->GetActorLocation();
}

void UCartGrabComponent::EnsureAimNiagara()
{
    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor)) return;

    if (!AimNiagaraSystem) return;

    if (!AimNiagaraComponent)
    {
        AimNiagaraComponent = NewObject<UNiagaraComponent>(OwnerActor, TEXT("AimNiagaraComponent"));
        AimNiagaraComponent->SetAsset(AimNiagaraSystem);
        AimNiagaraComponent->SetupAttachment(OwnerActor->GetRootComponent());
        AimNiagaraComponent->SetAutoActivate(false);

        OwnerActor->AddInstanceComponent(AimNiagaraComponent);
        AimNiagaraComponent->RegisterComponent();
    }
}

void UCartGrabComponent::UpdateAimNiagaraVisual(const FVector& Start, const FVector& End)
{
    if (VisualState != EGrabVisualState::None) return;
    if (!IsValid(AimNiagaraComponent)) return;

    FVector NiagaraOffset(0.f, 0.f, NiagaraHeightOffset);

    FVector NiagaraStart = Start + NiagaraOffset;
    FVector NiagaraEnd = End + NiagaraOffset;

    // static const를 이용하는게 임시 FName 대입보다 조금 더 빠름
    AimNiagaraComponent->SetVariablePosition(NiagaraParamName::NAME_StartPosition, NiagaraStart);
    AimNiagaraComponent->SetVariablePosition(NiagaraParamName::NAME_EndPosition, NiagaraEnd);
    AimNiagaraComponent->SetVariableFloat(NiagaraParamName::NAME_ActiveDistance, CachedAimDistance);
    AimNiagaraComponent->SetVariableFloat(NiagaraParamName::NAME_DotSpacing, AimDotSpacing);
    AimNiagaraComponent->SetVariableFloat(NiagaraParamName::NAME_DotSize, AimDotSize);
}

void UCartGrabComponent::SetAimVisual(bool bVisibility)
{
    // 나이아가라 설정
    if (IsValid(AimNiagaraComponent))
    {
        AimNiagaraComponent->SetVisibility(bVisibility, true);

        // 활성화 안되어있으면 키기
        if (bVisibility && !AimNiagaraComponent->IsActive())
        {
            AimNiagaraComponent->Activate(true);
        }
    }

    // 데칼 설정
    if (IsValid(AimDecal))
    {
        AimDecal->SetVisibility(bVisibility);
    }
}

void UCartGrabComponent::EnsureAimDecal()
{
    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor)) return;

    if (!AimDecal)
    {
        AimDecal = NewObject<UDecalComponent>(OwnerActor, TEXT("AimDecal"));
        AimDecal->SetupAttachment(OwnerActor->GetRootComponent());

        AimDecal->SetUsingAbsoluteLocation(true);
        AimDecal->SetUsingAbsoluteRotation(true);
        AimDecal->SetWorldRotation(FRotator(-90.f, 0.f, 0.f));
        AimDecal->SetVisibility(false);

        if (AimDecalMaterial)
        {
            AimDecal->SetDecalMaterial(AimDecalMaterial);
        }

        AimDecal->DecalSize = FVector(32.f, 32.f, 32.f);

        OwnerActor->AddInstanceComponent(AimDecal);
        AimDecal->RegisterComponent();
    }
}

void UCartGrabComponent::UpdateAimDecal(const FVector& Target)
{
    if (VisualState != EGrabVisualState::None) return;
    if (!IsValid(AimDecal)) return;

    AimDecal->SetWorldLocation(Target);
}
