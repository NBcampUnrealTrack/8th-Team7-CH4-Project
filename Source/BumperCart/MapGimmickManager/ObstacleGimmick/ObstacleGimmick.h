#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ObstacleGimmick.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UBoxComponent;

UCLASS()
class BUMPERCART_API AObstacleGimmick : public AActor
{
	GENERATED_BODY()
	
public:	
	AObstacleGimmick();

#pragma region Override
protected:
    virtual void BeginPlay() override;

#pragma endregion

#pragma region Component
protected:
    UPROPERTY(EditAnywhere, Category = "Component")
    TObjectPtr<USceneComponent> SceneRoot;

    // 장애물 매쉬
    UPROPERTY(EditAnywhere, Category = "Component")
    TObjectPtr<UStaticMeshComponent> ObstacleMesh;

    UPROPERTY(EditAnywhere, Category = "Component")
    TObjectPtr<UBoxComponent> BoxCollision;

#pragma endregion

};
