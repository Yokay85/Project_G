// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#define WIN32_LEAN_AND_MEAN

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include "ServerManager.generated.h"

UCLASS()
class PROJECT_G_API AServerManager : public AActor
{
	GENERATED_BODY()

private:
	SOCKET ListenSocket;
	TArray<FString> Deck;
	bool bStopServer = false;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck")
	AActor* DeckActor; // Посилання на BP_Deck

	// Sets default values for this actor's properties
	AServerManager();

	UFUNCTION(BlueprintCallable, Category = "Server")
	void StartServer();

	void RunServer();
	void HandleClient(SOCKET ClientSocket);
	void BeginDestroy();
	void CloseSocket();
	FString DrawCard();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
