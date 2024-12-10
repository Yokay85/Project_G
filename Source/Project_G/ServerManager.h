// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#define WIN32_LEAN_AND_MEAN

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include "ServerManager.generated.h"

USTRUCT(BlueprintType)
struct FPlayerInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite)
	TArray<FString> PlayerCards; 

	UPROPERTY(BlueprintReadWrite)
	int32 PlayerScore;

	FPlayerInfo()
		: PlayerName(TEXT("Unknown")), PlayerScore(0) {
	}
};

UCLASS()
class PROJECT_G_API AServerManager : public AActor
{
	GENERATED_BODY()

private:
	SOCKET ListenSocket;
	bool bStopServer = false;
	TMap<FString, FPlayerInfo> Players;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck")
	AActor* DeckActor;

	// Sets default values for this actor's properties
	AServerManager();

	UFUNCTION(BlueprintCallable, Category = "Server")
	void StartServer();

	void RunServer();
	void HandleClient(SOCKET ClientSocket);
	void BeginDestroy();
	void CloseSocket();
	void AddPlayer(SOCKET ClientSocket, const FString& PlayerName);
	void AddCardToPlayer(SOCKET ClientSocket, const FString& CardRowName);
	FString DrawCard();
	int32 CalculateScore(const TArray<FString>& PlayerCards);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
