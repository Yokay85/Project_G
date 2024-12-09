// Fill out your copyright notice in the Description page of Project Settings.


#include "ClientManager.h"

// Sets default values
AClientManager::AClientManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AClientManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AClientManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool AClientManager::ConnectToServer(const FString& IP, int Port)
{
    WSADATA WsaData;
    sockaddr_in ServerAddr;

    if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
        return false;

    ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ClientSocket == INVALID_SOCKET)
        return false;

    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(Port);
    inet_pton(AF_INET, TCHAR_TO_ANSI(*IP), &ServerAddr.sin_addr);

    return connect(ClientSocket, (sockaddr*)&ServerAddr, sizeof(ServerAddr)) != SOCKET_ERROR;
}

void AClientManager::SendRequest(const FString& Request)
{
    send(ClientSocket, TCHAR_TO_ANSI(*Request), Request.Len(), 0);
}

FString AClientManager::ReceiveResponse()
{
    char Buffer[1024];
    int BytesReceived = recv(ClientSocket, Buffer, sizeof(Buffer), 0);
    if (BytesReceived > 0)
    {
        return FString(ANSI_TO_TCHAR(Buffer));
    }
    return TEXT("");
}

void AClientManager::SendDrawMessage()
{
    FString Message = "Draw";
    int32 BytesSent = send(ClientSocket, TCHAR_TO_ANSI(*Message), Message.Len(), 0);

    if (BytesSent == SOCKET_ERROR) {
        int32 ErrorCode = WSAGetLastError();
        UE_LOG(LogTemp, Warning, TEXT("Send failed with error: %d"), ErrorCode);
    } else {
        UE_LOG(LogTemp, Log, TEXT("Sent message: %s"), *Message);
    }
}