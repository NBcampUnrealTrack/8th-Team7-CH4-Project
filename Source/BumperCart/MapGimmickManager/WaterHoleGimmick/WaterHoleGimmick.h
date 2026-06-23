#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaterHoleGimmick.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UBoxComponent;

UCLASS()
class BUMPERCART_API AWaterHoleGimmick : public AActor
{
	GENERATED_BODY()
	
public:	
    AWaterHoleGimmick();

#pragma region Component
protected:
    
    UPROPERTY(EditAnywhere, Category = "Component")
    TObjectPtr<USceneComponent> SceneRoot;

    // 믈 웅덩이 메쉬
    UPROPERTY(EditAnywhere, Category = "Component")
    TObjectPtr<UStaticMeshComponent> WaterHoleMesh;

    // 오버랩용 콜리전
    UPROPERTY(EditAnywhere, Category = "Component")
    TObjectPtr<UBoxComponent> BoxCollision;

#pragma endregion

#pragma region Override
protected:
    virtual void BeginPlay() override;

#pragma endregion

#pragma region Overlap
protected:
    UFUNCTION()
    void OnWaterHoleBeginOverlap(UPrimitiveComponent* OverlappedComp,
                                 AActor* OtherActor,
                                 UPrimitiveComponent* OtherComp,
                                 int32 OtherBodyIndex,
                                 bool bFromSweep,
                                 const FHitResult& SweepResult);

    UFUNCTION()
    void OnWaterHoleEndOverlap(UPrimitiveComponent* OverlappedComp,
                               AActor* OtherActor,
                               UPrimitiveComponent* OtherComp,
                               int32 OtherBodyIndex);
#pragma endregion

#pragma region Spin
private:
    // 회전 중인 캐릭터 기억
    TObjectPtr<ACharacter> SpinningCharacter;

    // 회전 지속시간
    UPROPERTY(EditAnywhere, Category = "Spin")
    float SpinDuration = 0.25f;

    // 회전각
    UPROPERTY(EditAnywhere, Category = "Spin")
    float EffectDuration = 0.8f;

    float TimerInterval = 0.01f;

    // 지속 시간 체크용
    float SpinElapsed = 0.0f;

    FTimerHandle SpinTimerHandle;

    // 플레이어의 원래 마찰력 값 저장 (복구용)
    float OriginalGroundFriction;
    float OriginalBrakingDeceleration;
    FRotator TargetRotation;

    // 타이머에 의해 주기적으로 회전 실행
    void UpdateCharacterSpin();

    // 회전이 끝났을 때 정지
    void StopCharacterSpin();
#pragma endregion


};
