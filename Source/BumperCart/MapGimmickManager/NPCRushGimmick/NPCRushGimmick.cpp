#include "MapGimmickManager/NPCRushGimmick/NPCRushGimmick.h"

#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/KismetStringLibrary.h"
#include "Cart/CartPawn.h"
#include "Cart/CartLoadTypes.h"
#include "Cart/Component/CartLoadComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

ANPCRushGimmick::ANPCRushGimmick()
{
 	PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;

    BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
    SetRootComponent(BoxCollision);

    CartMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CartMesh"));
    CartMesh->SetupAttachment(RootComponent);

    FrontLeftWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontLeftWheel"));
    FrontLeftWheel->SetupAttachment(CartMesh);

    FrontRightWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontRightWheel"));
    FrontRightWheel->SetupAttachment(CartMesh);

    BackLeftWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackLeftWheel"));
    BackLeftWheel->SetupAttachment(CartMesh);

    BackRightWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackRightWheel"));
    BackRightWheel->SetupAttachment(CartMesh);

    NPCRushFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
    NPCRushFX->SetupAttachment(CartMesh);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));

    NPCRushAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("NPCRushAudio"));
    NPCRushAudioComponent->SetupAttachment(RootComponent);

    if (IsValid(ProjectileMovement))
    {
        ProjectileMovement->InitialSpeed =1000.0f;
        ProjectileMovement->MaxSpeed = 1000.0f;
        ProjectileMovement->ProjectileGravityScale = 0.0f;
    }

    NPCRushAudioComponent->bAutoActivate = true;

    CurrentWheelRotationPitch = 0.0f;
    WheelRotationSpeed = -300.0f;
}

void ANPCRushGimmick::BeginPlay()
{
	Super::BeginPlay();

    FrontLeftInitRot = FrontLeftWheel->GetRelativeRotation();
    FrontRightInitRot = FrontRightWheel->GetRelativeRotation();
    BackLeftInitRot = BackLeftWheel->GetRelativeRotation();
    BackRightInitRot = BackRightWheel->GetRelativeRotation();

    if (SpawnSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), SpawnSound, GetActorLocation());
    }

    if (NPCRushAudioComponent && NPCRushAudioComponent->IsActive())
    {

    }

    if (HasAuthority() && BoxCollision)
    {
        BoxCollision->OnComponentBeginOverlap.AddDynamic(this, &ANPCRushGimmick::OnCartOverlap);
    }
}

void ANPCRushGimmick::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    CurrentWheelRotationPitch += WheelRotationSpeed * DeltaTime;

    CurrentWheelRotationPitch = FMath::Fmod(CurrentWheelRotationPitch, 360.0f);

    FRotator NewWheelRotation = FRotator(CurrentWheelRotationPitch, 0.0f, 0.0f);

    FrontLeftWheel->SetRelativeRotation(FRotator(CurrentWheelRotationPitch, FrontLeftInitRot.Yaw, FrontLeftInitRot.Roll));
    FrontRightWheel->SetRelativeRotation(FRotator(CurrentWheelRotationPitch, FrontRightInitRot.Yaw, FrontRightInitRot.Roll));
    BackLeftWheel->SetRelativeRotation(FRotator(CurrentWheelRotationPitch, BackLeftInitRot.Yaw, BackLeftInitRot.Roll));
    BackRightWheel->SetRelativeRotation(FRotator(CurrentWheelRotationPitch, BackRightInitRot.Yaw, BackRightInitRot.Roll));
}

void ANPCRushGimmick::OnCartOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority()) return;

    if (!OtherActor || OtherActor == this) return;

    if (OtherComp && OtherComp->GetCollisionObjectType() == ECC_WorldStatic)
    {
        Destroy();

        return;
    }

    if (ACartPawn* HitPlayer = Cast<ACartPawn>(OtherActor))
    {
        // 중복 실행 방지
        float CurrentTime = GetWorld()->GetTimeSeconds();
        if (CurrentTime - LastHitTime < 1.5f)    return;

        LastHitTime = CurrentTime;

        Knockback(HitPlayer, SweepResult);
    }
}

void ANPCRushGimmick::Knockback(ACartPawn* PlayerCart, const FHitResult& SweepResult)
{
    if (!HasAuthority()) return;

    if (!PlayerCart) return;


    if (KnockbackSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), KnockbackSound, GetActorLocation());
    }

    FVector CartForward = GetActorForwardVector().GetSafeNormal2D();
    FVector CartRight = GetActorRightVector().GetSafeNormal2D();

    FVector ToPlayer = (PlayerCart->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();

    float SideDot = FVector::DotProduct(ToPlayer, CartRight);

    FVector FinalKnockbackDir = FVector::ZeroVector;

    if (SideDot >= 0.0f)
    {
        FinalKnockbackDir = (CartForward + CartRight).GetSafeNormal2D();
    }
    else
    {
        FinalKnockbackDir = (CartForward - CartRight).GetSafeNormal2D();
    }

    FVector SpawnLocation = PlayerCart->GetActorLocation();
    SpawnLocation.Z += 60.0f;

    FRotator SpawnRotation = FinalKnockbackDir.Rotation();

    Multicast_PlayerHitEffect(SpawnLocation, SpawnRotation);

    EDropCollisionRole DropRole = EDropCollisionRole::Normal;

    UCartLoadComponent* CartLoadComp = PlayerCart->FindComponentByClass<UCartLoadComponent>();

    if (IsValid(CartLoadComp))
    {
        float ClosingSpeed = FinalKnockbackDir.Size2D();

        CartLoadComp->DropProducts(ClosingSpeed * 3000.0f, DropRole);

        PlayerCart->ApplyExternalKnockback(FinalKnockbackDir, Strength);
    }

    PlayerCart->ClientPlayCameraShake(nullptr, 1.0f);
}

void ANPCRushGimmick::Multicast_PlayerHitEffect_Implementation(FVector SpawnLocation, FRotator SpawnRotation)
{
    if (KnockbackFX)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            KnockbackFX,
            SpawnLocation,
            SpawnRotation,
            FVector(1.0f),
            true,
            true,
            ENCPoolMethod::None,
            true
        );
    }
}

