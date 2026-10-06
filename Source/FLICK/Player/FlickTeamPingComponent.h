#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/FlickTypes.h"
#include "FlickTeamPingComponent.generated.h"

class AFlickPiece;

// Messages are delivered to teammate controllers only, never replicated globally.
UCLASS()
class FLICK_API UFlickTeamPingComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UFlickTeamPingComponent();
	void TryPing(AFlickPiece* Piece, int32 SwitchIndex = INDEX_NONE);
	FString GetMessage(int32 Row) const;
	float GetOpacity(int32 Row) const;
	EFlickTeam GetMessageTeam(int32 Row) const;
	bool HasMessages() const;
	UPROPERTY(EditAnywhere, Category="FLICK|Team Ping", meta=(ClampMin="0.5"))
	float CooldownSeconds = 2.0f;
	UPROPERTY(EditAnywhere, Category="FLICK|Team Ping", meta=(ClampMin="2.0"))
	float MessageLifetime = 7.0f;
	UPROPERTY(EditAnywhere, Category="FLICK|Team Ping", meta=(ClampMin="0.1"))
	float FadeSeconds = 2.0f;
	static constexpr int32 MaximumMessages = 4;
private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FFlickTeamPingTest;
#endif
	UFUNCTION(Server, Reliable)
	void ServerPingTarget(int32 PieceId, int32 SwitchIndex);
	UFUNCTION(Client, Reliable)
	void ClientReceivePing(const FString& MatchId, EFlickTeam Team, const FString& Message);
	struct FMessage { FString MatchId; EFlickTeam Team; FString Text; double ReceivedAt; };
	TArray<FMessage> Messages;
	double LastLocalPing = -1000;
	double LastServerPing = -1000;
};
