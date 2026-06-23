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


};
