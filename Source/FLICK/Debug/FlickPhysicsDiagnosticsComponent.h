#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlickPhysicsDiagnosticsComponent.generated.h"

class UPrimitiveComponent;

/** Local, read-only developer instrumentation. No physics settings are modified. */
UCLASS()
class FLICK_API UFlickPhysicsDiagnosticsComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UFlickPhysicsDiagnosticsComponent();
	static bool IsEnabled();
	const FString& GetSummary() const { return Summary; }
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UFUNCTION()
	void RecordContact(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent,
		FVector Impulse, const FHitResult& Hit);
	void ClearBindings();
	struct FContact
	{
		FVector Point, Normal;
		double Time = 0;
		FString Description;
	};
	TArray<FContact> Contacts;
	TArray<TWeakObjectPtr<UPrimitiveComponent>> Observed;
	FString Summary;
};
