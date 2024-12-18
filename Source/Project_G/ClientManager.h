// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#define WIN32_LEAN_AND_MEAN

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include "ClientManager.generated.h"

USTRUCT(BlueprintType)
struct FPlayerScoreEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite)
	int32 PlayerScore;
};

UCLASS()
class PROJECT_G_API AClientManager : public AActor
{
	GENERATED_BODY()

private:
	SOCKET ClientSocket;
	
public:	
	// Sets default values for this actor's properties
	AClientManager();

	UFUNCTION(BlueprintCallable, Category = "Client")
	bool ConnectToServer(const FString& IP, int Port);

	UFUNCTION(BlueprintCallable, Category = "Client")
	FString ReceiveIsMyTurnResponse();

	UFUNCTION(BlueprintCallable, Category = "Client")
	FString ReceiveResponse();

	UFUNCTION(BlueprintCallable, Category = "Client")
	void SendRequest(const FString& Request);

	UFUNCTION(BlueprintCallable, Category = "Client")
	void SendIsMyTurnRequest();

	UFUNCTION(BlueprintCallable, Category = "Client")
	void SendDrawMessage();

	UFUNCTION(BlueprintCallable, Category = "Client")
	void SendStandMessage();

	UFUNCTION(BlueprintCallable, Category = "Client")
	void RequestScore();

	UFUNCTION(BlueprintCallable, Category = "Client")
	void RequestGameTable();

	UFUNCTION(BlueprintCallable, Category = "Client")
	int32 ReceiveScore();

	UFUNCTION(BlueprintCallable, Category = "Client")
	void RequestIsStanding();

	UFUNCTION(BlueprintCallable, Category = "Client")
	FString ReceiveIsStanding();

	UFUNCTION(BlueprintCallable, Category = "Client")
	TArray<FPlayerScoreEntry> ReceiveGameTable();

	UFUNCTION(BlueprintCallable, Category = "Client")
	bool CheckAllStanding();

	UFUNCTION(BlueprintCallable, Category = "Client")
	void DisconnectFromServer();

	UFUNCTION(BlueprintCallable, Category = "Client")
	void RequestPlayerName();
	
	UFUNCTION(BlueprintCallable, Category = "Client")
	FString ReceivePlayerName();


	void SendDisconnectMessage();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
