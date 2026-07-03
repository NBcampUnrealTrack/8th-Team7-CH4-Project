#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Cart/Bumpable.h"
#include "ObstacleGimmick.generated.h"

class UStaticMeshComponent;

UCLASS()
class BUMPERCART_API AObstacleGimmick : public AActor, public IBumpable
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
    // 장애물 매쉬
    UPROPERTY(EditAnywhere, Category = "Component")
    TObjectPtr<UStaticMeshComponent> ObstacleMesh;

#pragma endregion

};
