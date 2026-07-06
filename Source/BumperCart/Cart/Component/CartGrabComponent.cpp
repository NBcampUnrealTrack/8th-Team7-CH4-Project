// Fill out your copyright notice in the Description page of Project Settings.


#include "Cart/Component/CartGrabComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Product/ProductBase.h"
#include "CartLoadComponent.h"
#include "TimerManager.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Util/BCCollisionChannels.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"


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

    // 로봇손 연출 관련 변수
    VisualStartLocation = FVector::ZeroVector;
    VisualTargetLocation = FVector::ZeroVector;
    VisualReachDuration = 0.f;
    VisualReturnDuration = 0.f;
    VisualElapsedTime = 0.f;
    VisualState = EGrabVisualState::None;

    // 상품 획득시 연출 관련 변수
    ProductPopDuration = 0.5f;
    ProductPopMaxScale = 2.f;
    ProductPopMinScale = 0.5f;
    bProductPopPlaying = false;
    ProductPopElapsedTime = 0.f;
    ProductPopBaseScale = FVector::OneVector;
    ProductPopIncreaseDuration = 0.35f;

    // 실패 연출 변수
    GrabMissEffectOffset = 20.f;

    // 조준선 갱신 관련
    CachedAimDirection = FVector::ZeroVector;
    CachedAimTargetLocation = FVector::ZeroVector;
    CachedAimDistance = 0.f;

    // 조준선 나이아가라 변수
    AimDotSpacing = 45.f;
    AimDotSize = 20.f;
    NiagaraHeightOffset = 10.f;

    // 로봇손 판정
    bServerGrab = false;
    ServerGrabStartLocation = FVector::ZeroVector;
    ServerGrabDirection = FVector::ZeroVector;
    ServerGrabMaxDistance = 0.f;
    ServerGrabCurrentDistance = 0.f;
    AimPlaneZ = 20.f;

    // 로봇손 관련
    GrabSpeed = 1400.f;
    GrabRange = 500.f;
    GrabRadius = 10.f;
    bCanGrab = true;
    SocketName = TEXT("ProductSocket");

    static ConstructorHelpers::FObjectFinder<USoundBase> GrabSuccessSoundFinder(
        TEXT("/Game/Developers/dongh/Audio/GrabSuccess.GrabSuccess")
    );
    if (GrabSuccessSoundFinder.Succeeded())
    {
        GrabSuccessSound = GrabSuccessSoundFinder.Object;
    }

    static ConstructorHelpers::FObjectFinder<USoundBase> GrabMissSoundFinder(
        TEXT("/Game/Developers/dongh/Audio/GrabMiss.GrabMiss")
    );
    if (GrabMissSoundFinder.Succeeded())
    {
        GrabMissSound = GrabMissSoundFinder.Object;
    }

    static ConstructorHelpers::FObjectFinder<USoundBase> GrabLaunchSoundFinder(
        TEXT("/Game/Developers/dongh/Audio/SimpleWhoosh.SimpleWhoosh")
    );
    if (GrabLaunchSoundFinder.Succeeded())
    {
        GrabLaunchSound = GrabLaunchSoundFinder.Object;
    }

    static ConstructorHelpers::FObjectFinder<USoundBase> GrabBlockedSoundFinder(
        TEXT("/Game/Developers/dongh/Audio/GrabBlocked.GrabBlocked")
    );
    if (GrabBlockedSoundFinder.Succeeded())
    {
        GrabBlockedSound = GrabBlockedSoundFinder.Object;
    }
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
    StartAimVisual();
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
    StopAimVisual();

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
        ArmSpline->SetReceivesDecals(false);

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
        Hand->SetReceivesDecals(false);

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
        VisualProductMesh->SetUsingAbsoluteRotation(true);
        VisualProductMesh->SetReceivesDecals(false);

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
        TickGrabSweep(DeltaTime);
    }

    // 클라이언트만 연출
    if (GetNetMode() == NM_DedicatedServer) return;

    // 로컬에서 그랩 상태가 아니면 조준선 갱신
    if (IsLocallyControlled() && VisualState == EGrabVisualState::None)
    {
        UpdateGrabAim();
    }

    // 로봇손 늘어나는 위치 계산
    if (VisualState != EGrabVisualState::None)
    {
        TickGrabVisual(DeltaTime);
    }

    // 상품 Pop 연출
    if (bProductPopPlaying)
    {
        TickProductPopVisual(DeltaTime);
    }
}

void UCartGrabComponent::StartAimVisual()
{
    UpdateGrabAim();
    SetAimVisual(true);
    RefreshTickEnabled();
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

void UCartGrabComponent::StopAimVisual()
{
    CachedAimDirection = FVector::ZeroVector;
    CachedAimTargetLocation = FVector::ZeroVector;
    SetAimVisual(false);
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
}

void UCartGrabComponent::UpdateGrabAim()
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!IsValid(OwnerPawn))
    {
        StopAimVisual();
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(OwnerPawn->GetController());
    if (!IsValid(PlayerController))
    {
        StopAimVisual();
        return;
    }

    // WorldLocation : 마우스가 위치한 3D월드 시작점, 보통 카메라 위치
    // WorldDirection : 마우스 위치를 향해 뻗어나가는 방향 벡터, 카메라에서 마우스 커서 방향으로 나가는 방향
    // 마우스 방향 광선 식 : WorldLocation + WorldDirection * T (T는 임의의 값)
    FVector WorldLocation, WorldDirection;
    if (!PlayerController->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
    {
        StopAimVisual();
        return;
    }

    FVector Start = OwnerPawn->GetActorLocation();
    Start.Z = AimPlaneZ; // 강제로 상품 높이로 보정

    // 방향 Z가 0에 가까우면 T가 무한히 커질 수 있으니 빠져나오기
    if (FMath::IsNearlyZero(WorldDirection.Z)) return;

    // 상품높이 = WorldLocation + WorldDirection * T (광선위의 카트의 높이과 Z가 같은 임의의 점)
    // T = (상품높이 - WorldLocation) / WorldDirection
    //   -> T는 상품과 같은 높이가 되는데 필요한 값
    float T = (AimPlaneZ - WorldLocation.Z) / WorldDirection.Z;

    FVector AimPoint = WorldLocation + WorldDirection * T;
    AimPoint.Z = AimPlaneZ;

    FVector ToAimPoint = AimPoint - Start;
    ToAimPoint.Z = 0.f;

    // 마우스가 평면 위를 향하면 교차점이 뒤에 잡혀서 T가 음수가 나옴
    // 방향만 뒤집어주면 됨
    if (T < 0.f)
    {
        ToAimPoint *= -1.f;
    }

    float AimDistance = ToAimPoint.Size2D();
    if (AimDistance <= KINDA_SMALL_NUMBER) return;

    float ActualDistance = FMath::Clamp(AimDistance, 0.f, GrabRange);

    CachedAimDirection = ToAimPoint.GetSafeNormal();
    CachedAimTargetLocation = Start + CachedAimDirection * ActualDistance;
    CachedAimDistance = ActualDistance;

    SetAimVisual(true);
    UpdateAimNiagaraVisual(Start, CachedAimTargetLocation);
    UpdateAimDecal(CachedAimTargetLocation);
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
    Start.Z = AimPlaneZ;
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

    // 클라이언트는 연출 시작
    Multicast_PlayGrab(Start, End);

    // 서버도 그랩 판정 위해 Tick 켜기
    RefreshTickEnabled();
}

void UCartGrabComponent::RefreshTickEnabled()
{
    // 서버에서 틱을 필요로 하는지 체크
    bool bServerTick = GetOwner() && GetOwner()->HasAuthority() && bServerGrab;

    // 클라이언트에서 틱을 필요로 하는지 체크
    bool bVisualTick = VisualState != EGrabVisualState::None;

    // 본인 클라이언트에서 조준선을 위한 체크
    bool bAimTick = IsLocallyControlled() && VisualState == EGrabVisualState::None;

    bool bPopTick = bProductPopPlaying;

    // 둘중에 하나라도 필요로하면 틱 활성화
    SetComponentTickEnabled(bServerTick || bVisualTick || bAimTick || bPopTick);
}

void UCartGrabComponent::TickGrabSweep(float DeltaTime)
{
    // 이전값과 현재값 세팅
    float PreviousDistance = ServerGrabCurrentDistance;
    ServerGrabCurrentDistance = FMath::Min(ServerGrabCurrentDistance + GrabSpeed * DeltaTime, ServerGrabMaxDistance);

    float ActualDistance = 0.f;
    FVector HitLocation, HitNormal;
    EGrabResult Result = TrySweepGrab(PreviousDistance, ServerGrabCurrentDistance, ActualDistance, HitLocation, HitNormal);

    if (Result == EGrabResult::ProductGrabbed)
    {
        bServerGrab = false;

        float ReturnDuration = ActualDistance / FMath::Max(GrabSpeed, 1.f);

        // 모든 클라이언트에 회수 처리 및 회수 위치 설정
        Multicast_StartGrabReturn(ReturnDuration, GrabbedProduct.Get());

        // 서버에서 회수 처리
        StartGrabReturn(ReturnDuration);

        RefreshTickEnabled();
        return;
    }
    // 벽과 충돌하면
    else if (Result == EGrabResult::Blocked)
    {
        bServerGrab = false;

        float ReturnDuration = ActualDistance / FMath::Max(GrabSpeed, 1.f);

        Multicast_PlayGrabBlockedEffect(HitLocation, HitNormal);

        Multicast_StartGrabReturn(ReturnDuration, nullptr);
        StartGrabReturn(ReturnDuration);

        RefreshTickEnabled();
        return;
    }

    // 끝까지 검사했는데 못잡으면 최종 지점에서 회수 시작
    if (ServerGrabCurrentDistance >= ServerGrabMaxDistance - KINDA_SMALL_NUMBER)
    {
        bServerGrab = false;

        // 실패시 나이아가라 이펙트 Multicast
        FVector FailLocation = ServerGrabStartLocation + ServerGrabDirection * ServerGrabMaxDistance;
        FailLocation.Z = ServerGrabStartLocation.Z + GrabMissEffectOffset;
        Multicast_PlayGrabMissEffect(FailLocation);

        // 서버에서 회수 처리
        float ReturnDuration = ServerGrabMaxDistance / FMath::Max(GrabSpeed, 1.f);
        StartGrabReturn(ReturnDuration);

        RefreshTickEnabled();
    }
}

EGrabResult UCartGrabComponent::TrySweepGrab(float FromDistance, float ToDistance, float& OutDistance, FVector& OutHitLocation, FVector& OutHitNormal)
{
    UWorld* World = GetWorld();
    if (!IsValid(World)) return EGrabResult::None;

    AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor)) return EGrabResult::None;

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

    if (!bHit) return EGrabResult::None;

    AActor* HitActor = Hit.GetActor();

    // 충돌했다면
    if (IsValid(HitActor))
    {
        float HitDistance = FVector::DotProduct(Hit.Location - ServerGrabStartLocation, ServerGrabDirection);
        OutDistance = FMath::Clamp(HitDistance, FromDistance, ToDistance);
        OutHitLocation = Hit.ImpactPoint;
        OutHitNormal = Hit.ImpactNormal;

        AProductBase* Product = Cast<AProductBase>(HitActor);
        // 충돌한게 상품이면
        if (IsValid(Product) && Product->TrySetGrabbed())
        {
            GrabbedProduct = Product;
            return EGrabResult::ProductGrabbed;
        }

        // 충돌한게 상품이 아니면
        GrabbedProduct = nullptr;
        return EGrabResult::Blocked;
    }

    return EGrabResult::None;
}

void UCartGrabComponent::StartGrabReturn(float ReturnDuration)
{
    UWorld* World = GetWorld();
    if (!IsValid(World)) return;

    // 너무 작은 시간이 들어오면 바로 종료
    if (ReturnDuration <= KINDA_SMALL_NUMBER)
    {
        HandleFinishGrab();
        return;
    }

    World->GetTimerManager().SetTimer(
        GrabFinishTimer,
        this,
        &ThisClass::HandleFinishGrab,
        ReturnDuration,
        false
    );
}

void UCartGrabComponent::Multicast_StartGrabReturn_Implementation(float ReturnDuration, AProductBase* Product)
{
    if (GetNetMode() == NM_DedicatedServer) return;

    EnsureVisualComponents();
    if (!ArmSpline || !Hand) return;

    // 붙잡기 성공했다면 메시 보여주기
    if (IsValid(Product))
    {
        ShowVisualProductMesh(Product);
    }

    // 현재 손 위치 구하기
    float Progress = VisualElapsedTime / VisualReachDuration;
    FVector CurrentLocation = FMath::Lerp(GetGrabStartLocation(), VisualTargetLocation, Progress);

    // 회수로 변경하고 위치 조정하기
    VisualTargetLocation = CurrentLocation;
    VisualElapsedTime = 0.f;
    VisualReturnDuration = ReturnDuration;

    VisualState = EGrabVisualState::Returning;

    UpdateGrabVisual(CurrentLocation);

    RefreshTickEnabled();
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

    SetHandOpen(true);

    // 내 Pawn 이라면 조준선 끄기
    if (IsLocallyControlled())
    {
        SetAimVisual(false);

        if (GrabLaunchSound)
        {
            UGameplayStatics::PlaySound2D(this, GrabLaunchSound);
        }
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

    RefreshTickEnabled();
}

void UCartGrabComponent::FinishGrabVisual()
{
    VisualState = EGrabVisualState::None;
    VisualElapsedTime = 0.f;

    SetHandOpen(true);

    if (ArmSpline)
    {
        ArmSpline->SetVisibility(false);
    }
    if (Hand)
    {
        Hand->SetVisibility(false);
    }
    HideVisualProductMesh();

    RefreshTickEnabled();

    // 내 Pawn 이라면 조준선 켜기
    // 마우스 위치 기준으로 조준선 한번 갱신하고 Visual 켜야함
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (IsValid(OwnerPawn) && OwnerPawn->IsLocallyControlled())
    {
        UpdateGrabAim();
        SetAimVisual(true);
    }
}

void UCartGrabComponent::SetHandOpen(bool bIsOpen)
{
    if (!IsValid(Hand)) return;

    if (bIsOpen)
    {
        if (HandMeshAsset)
        {
            Hand->SetStaticMesh(HandMeshAsset);
        }
    }
    else
    {
        if (CloseHandMeshAsset)
        {
            Hand->SetStaticMesh(CloseHandMeshAsset);
        }
    }
}

void UCartGrabComponent::ShowVisualProductMesh(AProductBase* Product)
{
    EnsureVisualComponents();

    if (!IsValid(Product) || !IsValid(VisualProductMesh)) return;

    UStaticMesh* ProductMesh = Product->GetProductMesh();
    if (!IsValid(ProductMesh)) return;

    SetHandOpen(false);

    VisualProductMesh->SetStaticMesh(ProductMesh);
    VisualProductMesh->SetVisibility(true);

    // 닫힌손 소켓에 부착해주기
    VisualProductMesh->AttachToComponent(Hand, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);

    // 원본 상품은 가리기
    Product->SetActorHiddenInGame(true);

    StartProductPopVisual();

    if (ProductGrabPickupEffect)
    {
        UWorld* World = GetWorld();
        if (!IsValid(World)) return;

        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            World,
            ProductGrabPickupEffect,
            VisualProductMesh->GetComponentLocation(),
            FRotator::ZeroRotator,
            FVector::OneVector,
            true
        );
    }

    if (IsLocallyControlled())
    {
        if (GrabSuccessSound)
        {
            UGameplayStatics::PlaySound2D(this, GrabSuccessSound);
        }
    }
}

void UCartGrabComponent::HideVisualProductMesh()
{
    bProductPopPlaying = false;

    if (VisualProductMesh)
    {
        VisualProductMesh->SetRelativeScale3D(ProductPopBaseScale);
        VisualProductMesh->SetVisibility(false);
        VisualProductMesh->SetStaticMesh(nullptr);
    }
}

void UCartGrabComponent::TickGrabVisual(float DeltaTime)
{
    VisualElapsedTime += DeltaTime;

    if (VisualState == EGrabVisualState::Extending)
    {
        // 진행 정도 확인
        float Progress = VisualElapsedTime / VisualReachDuration;

        // 연출 튀면 Clamp 필요
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
        float Progress = VisualElapsedTime / FMath::Max(VisualReturnDuration, KINDA_SMALL_NUMBER);

        // 연출 튀면 Clamp 필요
        FVector CurrentLocation = FMath::Lerp(GetGrabStartLocation(), VisualTargetLocation, 1.f - Progress);
        UpdateGrabVisual(CurrentLocation);

        if (Progress >= 1.f)
        {
            FinishGrabVisual();
        }
    }
}

void UCartGrabComponent::StartProductPopVisual()
{
    if (!IsValid(VisualProductMesh)) return;

    // 연출용 메시의 스케일값 가져오기, 되돌릴때 참고하는 용도
    ProductPopBaseScale = VisualProductMesh->GetRelativeScale3D();
    ProductPopElapsedTime = 0.f;
    bProductPopPlaying = true;

    VisualProductMesh->SetRelativeScale3D(ProductPopBaseScale * ProductPopMinScale);
}

void UCartGrabComponent::TickProductPopVisual(float DeltaTime)
{
    if (!IsValid(VisualProductMesh))
    {
        bProductPopPlaying = false;
        return;
    }

    ProductPopElapsedTime += DeltaTime;

    float Progress = FMath::Clamp(
        ProductPopElapsedTime / FMath::Max(ProductPopDuration, KINDA_SMALL_NUMBER),
        0.f,
        1.f
    );

    float ScaleValue = 0.f;

    if (Progress < ProductPopIncreaseDuration)
    {
        // min -> max 로 커지기
        ScaleValue = FMath::Lerp(
            ProductPopMinScale,
            ProductPopMaxScale,
            Progress / ProductPopIncreaseDuration
        );
    }
    else
    {
        // max -> 1.f 로 돌아오기
        ScaleValue = FMath::Lerp(
            ProductPopMaxScale,
            1.f,
            (Progress - ProductPopIncreaseDuration) / (1.f - ProductPopIncreaseDuration)
        );
    }

    VisualProductMesh->SetRelativeScale3D(ProductPopBaseScale * ScaleValue);

    // 로봇손 연출보다 먼저 끝날 수도 있으니 체크해서 연출 끄기
    if (Progress >= 1.f)
    {
        bProductPopPlaying = false;
        VisualProductMesh->SetRelativeScale3D(ProductPopBaseScale);
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

void UCartGrabComponent::Multicast_PlayGrabMissEffect_Implementation(FVector_NetQuantize EffectLocation)
{
    if (GetNetMode() == NM_DedicatedServer) return;
    if (!GrabMissEffect) return;

    UWorld* World = GetWorld();
    if (!IsValid(World)) return;

    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        World,
        GrabMissEffect,
        EffectLocation,
        FRotator::ZeroRotator,
        FVector::OneVector,
        true
    );

    if (IsLocallyControlled() && GrabMissSound)
    {
        UGameplayStatics::PlaySound2D(this, GrabMissSound);
    }
}

void UCartGrabComponent::Multicast_PlayGrabBlockedEffect_Implementation(FVector_NetQuantize EffectLocation,
    FVector_NetQuantizeNormal EffectNormal)
{
    if (GetNetMode() == NM_DedicatedServer) return;
    if (!GrabBlockedEffect) return;

    UWorld* World = GetWorld();
    if (!IsValid(World)) return;

    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        World,
        GrabBlockedEffect,
        EffectLocation,
        EffectNormal.Rotation(),
        FVector::OneVector,
        true
    );

    if (IsLocallyControlled() && GrabBlockedSound)
    {
        UGameplayStatics::PlaySound2D(this, GrabBlockedSound);
    }
}

bool UCartGrabComponent::IsLocallyControlled() const
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    return IsValid(OwnerPawn) && OwnerPawn->IsLocallyControlled();
}
