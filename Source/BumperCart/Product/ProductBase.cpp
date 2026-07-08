// Fill out your copyright notice in the Description page of Project Settings.


#include "Product/ProductBase.h"

#include "ProductDataAsset.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
#include "ProductShelfSubsystem/ProductShelfSubsystem.h"
#include "GameFramework/GameState.h"
#include "NiagaraComponent.h"
#include "Util/Utility.h"

AProductBase::AProductBase()
{
 	PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    bReplicates = true;
    SetReplicateMovement(false);
    SetNetUpdateFrequency(1.f);

    // 컴포넌트 설정
    ProductCollision = CreateDefaultSubobject<USphereComponent>(TEXT("Root"));
    ProductCollision->SetSphereRadius(40.f);
    ProductCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ProductCollision->SetGenerateOverlapEvents(false);
    ProductCollision->SetCollisionProfileName(TEXT("ProductCollision"));
    SetRootComponent(ProductCollision);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    Mesh->SetupAttachment(ProductCollision);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetSimulatePhysics(false);
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetReceivesDecals(false);

    AuraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("AuraComponent"));
    AuraComponent->SetupAttachment(Mesh);
    AuraComponent->SetAutoActivate(false);
    AuraComponent->SetRelativeLocation(FVector::ZeroVector);

    bOnSale = false;

    ElapsedTime = 0.f;
    HeightOffset = 120.f;
    BobbingAmplitude = 30.f;
    BobbingSpeed = 1.5f;
    RotationSpeed = 60.f;

    FallingMinHeight = 120.f;
    FallingMaxHeight = 200.f;
    FallingHorizontalOffset = 200.f;
    FallingDuration = 1.f;

    SpawningDuration = 1.1f;
    SpawningHeight = 250.f;
}

void AProductBase::Destroyed()
{
    UWorld* World = GetWorld();
    if (IsValid(World))
    {
        if (UProductShelfSubsystem* ShelfSubsystem = World->GetSubsystem<UProductShelfSubsystem>())
        {
            ShelfSubsystem->OnProductDestroyed();
        }
    }

    Super::Destroyed();
}

void AProductBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    switch (EndPlayReason)
    {
    case EEndPlayReason::Destroyed:
        UE_LOG(LogTemp, Warning, TEXT("%s 상품 파괴"), *GetName());
        break;

    case EEndPlayReason::RemovedFromWorld:
        UE_LOG(LogTemp, Warning, TEXT("%s 상품 월드에서 제거"), *GetName());
        break;
    }
}

void AProductBase::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    BaseMeshLocation = Mesh->GetRelativeLocation();
    BaseMeshRotation = Mesh->GetRelativeRotation();
}

void AProductBase::BeginPlay()
{
    Super::BeginPlay();

    ApplyDataAsset();

    FVector BobbingLocation = GetActorLocation();
    BobbingLocation.Z = HeightOffset;
    SetActorLocation(BobbingLocation, false, nullptr, ETeleportType::TeleportPhysics);

    // 랜덤한 위아래 움직임을 위해 시작 시간을 다르게 함
    // 연출이라 모든 클라가 같을 필요는 없음
    ElapsedTime = FMath::RandRange(0.f, 2.f);

    SetProductState(EProductState::None);
}

void AProductBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (ProductState.State == EProductState::Display)
    {
        TickDisplay(DeltaTime);
    }
    else if (IsLaunchState())
    {
        TickLaunch(DeltaTime);
    }
}

void AProductBase::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    ApplyDataAsset();
}

void AProductBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, ProductState);
}

void AProductBase::ApplyDataAsset()
{
    if (!IsValid(ProductDataAsset)) return;

    if (IsValid(ProductDataAsset->ProductMesh))
    {
        Mesh->SetStaticMesh(ProductDataAsset->ProductMesh);
    }

    ApplyValueOverlay();
    ApplyValueAura();
}

void AProductBase::ApplyProductState()
{
    switch (ProductState.State)
    {
    case EProductState::Display:
        SetActorHiddenInGame(false);

        ProductCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

        // 클라이언트면 둥둥 떠다니는 연출위해 Tick 켜기, 데이케이트 서버는 끄기
        SetActorTickEnabled(GetNetMode() != NM_DedicatedServer);
        break;

    case EProductState::Spawning:   // Fall Through
    case EProductState::Falling:
        SetActorHiddenInGame(false);

        ProductCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

        SetActorTickEnabled(true);
        break;

    case EProductState::Grabbed:    // Fall Through
    case EProductState::Loaded:
    case EProductState::Paid:   
    case EProductState::None: 
    default:
        SetActorHiddenInGame(true);

        ProductCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        SetActorTickEnabled(false);
        break;
    }

    if (GetNetMode() != NM_DedicatedServer)
    {
        RefreshAuraActive();
    }
}

void AProductBase::StartSpawn(const FVector& StartLocation, const FVector& EndLocation, AActor* IgnoreActor)
{
    if (!HasAuthority()) return;
    StartLaunch(
        EProductState::Spawning,
        StartLocation,
        EndLocation,
        SpawningHeight,
        SpawningDuration,
        IgnoreActor
    );
}

void AProductBase::SetProductState(EProductState NewState)
{
    if (!HasAuthority()) return;

    if (ProductState.State == NewState) return;

    ProductState.State = NewState;

    // 서버 또한 State에 따른 변화를 적용해야 함
    ApplyProductState();
}

bool AProductBase::TrySetLoaded()
{
    if (!HasAuthority()) return false;

    if (!CanLoad()) return false;

    SetProductState(EProductState::Loaded);
    ForceNetUpdate();
    return true;
}

bool AProductBase::TrySetGrabbed()
{
    if (!HasAuthority()) return false;

    if (!CanGrab()) return false;

    SetProductState(EProductState::Grabbed);
    ForceNetUpdate();
    return true;
}

void AProductBase::DropFromCart(AActor* CartActor)
{
    if (!HasAuthority()) return;
    if (!IsValid(CartActor)) return;

    // Falling 오프셋 결정
    FVector Offset = FVector(
        FMath::RandRange(-FallingHorizontalOffset, FallingHorizontalOffset),
        FMath::RandRange(-FallingHorizontalOffset, FallingHorizontalOffset),
        0.f);

    float RandomHeight = FMath::RandRange(FallingMinHeight, FallingMaxHeight);

    // Falling 시작 위치 잡기
    FVector StartLocation = CartActor->GetActorLocation();
    StartLocation.Z = HeightOffset;

    // Falling 목표 위치 잡기, 시간 0 으로 설정
    FVector EndLocation = StartLocation + Offset;

    StartLaunch(
        EProductState::Falling,
        StartLocation,
        EndLocation,
        RandomHeight,
        FallingDuration,
        CartActor
    );
}

void AProductBase::OnRep_ProductState()
{
    if (IsLaunchState())
    {
        // Falling or Spawn으로 변할땐 포물선 운동 값 설정
        // 서버로부터 경과 시간을 받아서 시작 위치 지정
        float Alpha = GetLaunchAlpha();

        // 이미 연출이 끝났다면
        //  -> 네트워크 딜레이가 길어 연출이 끝났을때 상태를 전송받으면 즉시 종료처리
        if (Alpha >= 1.f)
        {
            FVector EndLocation = ProductState.LaunchEndLocation;
            EndLocation.Z = HeightOffset;

            SetActorLocation(EndLocation, false, nullptr, ETeleportType::TeleportPhysics);
            ResetBaseMeshTransform();
        }
        else
        {
            const FVector CurrentLocation = GetLaunchLocation(Alpha);
            SetActorLocation(CurrentLocation, false, nullptr, ETeleportType::TeleportPhysics);

            // 메시를 포물선 운동 진행도에 맞게 위로 올리기
            if (IsValid(Mesh))
            {
                float CurrentZ = FMath::Sin(Alpha * PI) * ProductState.LaunchHeight;
                Mesh->SetRelativeLocation(BaseMeshLocation + FVector(0.f, 0.f, CurrentZ));
            }
        }
    }
    else if (ProductState.State == EProductState::Display)
    {
        // Falling/Spawning -> Display로 변할땐 위치 지정
        FVector ServerLocation = ProductState.DisplayLocation;
        ServerLocation.Z = HeightOffset;

        SetActorLocation(ServerLocation, false, nullptr, ETeleportType::TeleportPhysics);
        ResetBaseMeshTransform();
    }

    ApplyProductState();
}

int32 AProductBase::GetWeight() const
{
    if (!ProductDataAsset) return 0;

    return ProductDataAsset->ProductData.Weight;
}

int32 AProductBase::GetValue() const
{
    if (!ProductDataAsset) return 0;

    return ProductDataAsset->ProductData.Value;
}

EProductState AProductBase::GetProductState() const
{
    return ProductState.State;
}

FLoadedProductInfo AProductBase::GetLoadedProductInfo() const
{
    FLoadedProductInfo Info;

    if (IsValid(ProductDataAsset))
    {
        Info.ProductId = ProductDataAsset->ProductId;
        Info.Value = ProductDataAsset->ProductData.Value;
    }
    Info.bOnSale = bOnSale;

    return Info;
}

UStaticMesh* AProductBase::GetProductMesh() const
{
    return IsValid(Mesh) ? Mesh->GetStaticMesh() : nullptr;
}

void AProductBase::SetOnSale(bool NewValue)
{
    bOnSale = NewValue;
}

bool AProductBase::IsOnSale() const
{
    return bOnSale;
}

void AProductBase::ResetBaseMeshTransform()
{
    if (IsValid(Mesh))
    {
        Mesh->SetRelativeLocation(BaseMeshLocation);
        Mesh->SetRelativeRotation(BaseMeshRotation);
    }
}

bool AProductBase::CanLoad() const
{
    return ProductState.State == EProductState::Grabbed;
}

bool AProductBase::CanGrab() const
{
    return ProductState.State == EProductState::Display;
}

void AProductBase::TickDisplay(float DeltaTime)
{
    if (!IsValid(Mesh)) return;

    ElapsedTime += DeltaTime;

    // 메시만 SIn 함수 따라 상대 좌표 위아래로 이동
    float BobZ = FMath::Sin(ElapsedTime * BobbingSpeed) * BobbingAmplitude;
    Mesh->SetRelativeLocation(BaseMeshLocation + FVector(0.f, 0.f, BobZ));

    // 회전값 적용
    FRotator Rotation(0.f, RotationSpeed * ElapsedTime, 0.f);
    Mesh->SetRelativeRotation(BaseMeshRotation + Rotation);
}

void AProductBase::StartLaunch(EProductState State, const FVector& StartLocation, const FVector& EndLocation, float InHeight, float InDuration, AActor* IgnoreActor)
{
    if (!HasAuthority()) return;

    FVector FixedStart = StartLocation;
    FixedStart.Z = HeightOffset;

    FVector FixedEnd = EndLocation;
    FixedEnd.Z = HeightOffset;

    // 목표 지점을 벽/가판대 안으로 들어가지 않게 조정
    FVector SafeEnd = GetSafeLocation(FixedStart, FixedEnd, IgnoreActor);

    LaunchIgnoredActor = IgnoreActor;

    // 충돌체와 무시할 액터가 유효하면 해당 액터는 무시하고 이동하게 함
    //  -> 가판대 or 카트에 껴서 못나오는 문제 해결하기 위함
    if (IsValid(ProductCollision) && IsValid(IgnoreActor))
    {
        ProductCollision->IgnoreActorWhenMoving(IgnoreActor, true);
    }

    // 복제되는 구조체 한번에 설정하고 대입
    FProductRepState NewRepState;
    NewRepState.State = State;
    NewRepState.LaunchStartLocation = FixedStart;
    NewRepState.LaunchEndLocation = SafeEnd;
    NewRepState.LaunchHeight = InHeight;
    NewRepState.LaunchDuration = InDuration;
    NewRepState.LaunchServerStartTime = GetServerTimeSeconds();

    ProductState = NewRepState;

    // 우선 시작지점으로 이동
    SetActorLocation(ProductState.LaunchStartLocation, false, nullptr, ETeleportType::TeleportPhysics);

    ApplyProductState();
    ForceNetUpdate();
}

void AProductBase::TickLaunch(float DeltaTime)
{
    float Alpha = GetLaunchAlpha();
    FVector BaseLocation = GetLaunchLocation(Alpha);

    if (HasAuthority())
    {
        SetActorLocation(BaseLocation, true);
    }
    else
    {
        SetActorLocation(BaseLocation, false, nullptr, ETeleportType::TeleportPhysics);
    }
    

    if (IsValid(Mesh))
    {
        // 보여지는 메시만 위로 튀는 연출
        // Sin함수 PI까지만하면 0 ~ 1, 1 ~ 0 처리
        float CurrentZ = FMath::Sin(Alpha * PI) * ProductState.LaunchHeight;
        Mesh->SetRelativeLocation(BaseMeshLocation + FVector(0.f, 0.f, CurrentZ));
    }

    if (Alpha >= 1.f && HasAuthority())
    {
        SetActorLocation(ProductState.LaunchEndLocation, true);

        // 서버에서 상품이 실제로 위치한 좌표 저장
        ProductState.DisplayLocation = GetActorLocation();
        ProductState.DisplayLocation.Z = HeightOffset;

        // 시작할때 무시하도록 설정했던 값 되돌리기
        if (IsValid(ProductCollision) && LaunchIgnoredActor.IsValid())
        {
            ProductCollision->IgnoreActorWhenMoving(LaunchIgnoredActor.Get(), false);
        }
        LaunchIgnoredActor.Reset();

        ResetBaseMeshTransform();

        SetProductState(EProductState::Display);
        ForceNetUpdate();
    }
}

bool AProductBase::IsLaunchState() const
{
    return ProductState.State == EProductState::Spawning ||
        ProductState.State == EProductState::Falling;
}

FVector AProductBase::GetSafeLocation(const FVector& Start, const FVector& End, AActor* IgnoreActor)
{
    if (!IsValid(ProductCollision)) return End;

    UWorld* World = GetWorld();
    if (!IsValid(World)) return End;

    FVector SafeStart = Start;
    SafeStart.Z = HeightOffset;

    FVector SafeEnd = End;
    SafeEnd.Z = HeightOffset;

    // ProductSafeLocation 이란 이름으로 쿼리 설정, 복잡한 충돌체는 false (Simple Collision만)
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ProductSafeLocation), false);
    Params.AddIgnoredActor(this);
    if (IsValid(IgnoreActor))
    {
        Params.AddIgnoredActor(IgnoreActor);
    }

    FCollisionShape Shape = FCollisionShape::MakeSphere(ProductCollision->GetScaledSphereRadius());

    FHitResult Hit;
    bool bHit = World->SweepSingleByProfile(
        Hit,
        SafeStart,
        SafeEnd,
        FQuat::Identity,
        ProductCollision->GetCollisionProfileName(),
        Shape,
        Params
    );

    // Sweep 으로 충돌 안했으면 안전한 위치임
    if (!bHit)
    {
        return SafeEnd;
    }

    // 시작지점부터 겹쳐있었다면 시작지점이 타겟
    if (Hit.bStartPenetrating)
    {
        return SafeStart;
    }

    // 충돌 지점을 구해서 반환
    FVector SafeLocation = Hit.Location;
    SafeLocation.Z = HeightOffset;
    return SafeLocation;
}

float AProductBase::GetServerTimeSeconds() const
{
    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        return 0.f;
    }

    if (AGameStateBase* GameState = World->GetGameState())
    {
        return GameState->GetServerWorldTimeSeconds();
    }

    return World->GetTimeSeconds();
}

float AProductBase::GetLaunchElapsedTime() const
{
    return FMath::Max(0.f, GetServerTimeSeconds() - ProductState.LaunchServerStartTime);
}

float AProductBase::GetLaunchAlpha() const
{
    float Duration = FMath::Max(ProductState.LaunchDuration, KINDA_SMALL_NUMBER);
    return FMath::Clamp(GetLaunchElapsedTime() / Duration, 0.f, 1.f);
}

FVector AProductBase::GetLaunchLocation(float Alpha) const
{
    FVector Location = FMath::Lerp(
        FVector(ProductState.LaunchStartLocation),
        FVector(ProductState.LaunchEndLocation),
        Alpha
    );
    Location.Z = HeightOffset;

    return Location;
}

void AProductBase::ApplyValueOverlay()
{
    if (!IsValid(Mesh)) return;

    int32 Value = GetValue();

    // 동적 머티리얼 생성
    ValueOverlayMID = UMaterialInstanceDynamic::Create(ValueOverlayMaterial, this);
    if (!IsValid(ValueOverlayMID))
    {
        Mesh->SetOverlayMaterial(nullptr);
        return;
    }

    ValueOverlayMID->SetVectorParameterValue(TEXT("OverlayColor"), GetValueOverlayColor());

    Mesh->SetOverlayMaterial(ValueOverlayMID);
}

FLinearColor AProductBase::GetValueOverlayColor() const
{
    int32 Value = GetValue();

    if (Value >= 80) return BCColor::Gold; // 가장 가치 있음, 황금색or빨강
    if (Value >= 60) return BCColor::Purple; // 높은 가치, 보라
    if (Value >= 40) return BCColor::Blue; // 중간 가치, 파랑
    if (Value >= 20) return BCColor::Green; // 낮은 가치 초록

    return BCColor::White; // 제일 저렴함, 흰색
}

void AProductBase::ApplyValueAura()
{
    if (!IsValid(AuraComponent) || !AuraSystem) return;

    int32 Value = GetValue();

    AuraComponent->SetAsset(AuraSystem);

    FLinearColor Color = GetValueOverlayColor();
    Color.A = 0.08f;

    AuraComponent->SetVariableLinearColor(TEXT("User.AuraColor"), Color);

    RefreshAuraActive();
}

void AProductBase::RefreshAuraActive()
{
    if (!IsValid(AuraComponent)) return;

    bool bShowAura = ProductState.State == EProductState::Display && AuraSystem;

    if (bShowAura)
    {
        AuraComponent->Activate();
    }
    else
    {
        AuraComponent->DeactivateImmediate();
    }
}
