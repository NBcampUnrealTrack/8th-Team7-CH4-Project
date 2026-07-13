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
 	PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;

    BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
    SetRootComponent(BoxCollision);

    CartMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CartMesh"));
    CartMesh->SetupAttachment(RootComponent);
    //CartMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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
}

void ANPCRushGimmick::BeginPlay()
{
	Super::BeginPlay();

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

void ANPCRushGimmick::OnCartOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority()) return;

    if (!OtherActor || OtherActor == this) return;

    if (OtherComp && OtherComp->GetCollisionObjectType() == ECC_WorldStatic)
    {
        UE_LOG(LogTemp, Log, TEXT("[맵 기믹 매니저] 거대 카트가 마트 벽에 부딪혀 박살났습니다!"));

        Destroy();
    }

    if (ACartPawn* HitPlayer = Cast<ACartPawn>(OtherActor))
    {
        UE_LOG(LogTemp, Log, TEXT("[NPCRush] 플레이어 오버랩 감지"));

        // 중복 실행 방지
        float CurrentTime = GetWorld()->GetTimeSeconds();
        if (CurrentTime - LastHitTime < 1.5f)    return;

        LastHitTime = CurrentTime;

        Knockback(HitPlayer);
    }
}

void ANPCRushGimmick::Knockback(ACartPawn* PlayerCart)
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

    EDropCollisionRole DropRole = EDropCollisionRole::Normal;

    UCartLoadComponent* CartLoadComp = PlayerCart->FindComponentByClass<UCartLoadComponent>();

    if (IsValid(CartLoadComp))
    {
        float ClosingSpeed = FinalKnockbackDir.Size2D();

        CartLoadComp->DropProducts(ClosingSpeed * 3000.0f, DropRole);

        PlayerCart->ApplyExternalKnockback(FinalKnockbackDir, Strength);

        UE_LOG(LogTemp, Log, TEXT("[거대 카트] 아이템 드롭, 밀치기 완료"));
    }

    PlayerCart->ClientPlayCameraShake(nullptr, 1.0f);
}

