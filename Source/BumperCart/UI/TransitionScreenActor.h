#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TransitionScreenActor.generated.h"

class UUserWidget;

UCLASS()
class BUMPERCART_API ATransitionScreenActor : public AActor
{
	GENERATED_BODY()
public:
    ATransitionScreenActor();

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> LoadingWidgetClass;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    TObjectPtr<UUserWidget> LoadingWidget;
};
