#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSpawn/ProductShelf/ProductShelf.h"
#include "ProductShelfManager.generated.h"

UCLASS()
class BUMPERCART_API AProductShelfManager : public AActor
{
	GENERATED_BODY()
	
public:	
    AProductShelfManager();

#pragma region Override
protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

#pragma endregion

#pragma region ProductShelf
private:
    // 모든 가판대
    UPROPERTY(VisibleAnywhere)
    TArray<AProductShelf*> AllProductShelfs;

    // 가판대 온오프
    UPROPERTY(EditAnywhere)
    bool bToggleOn = false;

public:
    // 모든 가판대 온오프 세팅
    void SetAllShelvesOpen(bool bToggle);

#pragma endregion

};
