// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#define WIN32_LEAN_AND_MEAN

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include "ClientManager.generated.h"

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
	int32 ReceiveScore();




protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
