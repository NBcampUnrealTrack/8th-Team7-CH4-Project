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

    // 믈 웅덩이 매쉬
    UPROPERTY(EditAnywhere, Category = "Component")
    TObjectPtr<UStaticMeshComponent> WaterHoleMesh;

    // 콜리전
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

#pragma region WaterHole
private:
    // 효과 지속시간
    UPROPERTY(EditAnywhere, Category = "Spin")
    float EffectDuration = 0.8f;

protected:
    UPROPERTY(EditAnywhere, Category = "WaterHole | Sound")
    USoundBase* WaterHoleSound;

#pragma endregion


};
