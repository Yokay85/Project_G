// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerManager.h"
#include "Engine/Engine.h"
#include <Misc/OutputDeviceNull.h>
#include "Async/Async.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values
AServerManager::AServerManager()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AServerManager::BeginPlay()
{
    Super::BeginPlay();
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("ServerManager is running!"));
    UE_LOG(LogTemp, Log, TEXT("ServerManager is running!"));

}

// Called every frame
void AServerManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

}

void AServerManager::RunServer()
{
    WSADATA WsaData;
    sockaddr_in ServerAddr;

    if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
    {
        UE_LOG(LogTemp, Error, TEXT("WSAStartup failed"));
        return;
    }

    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET)
    {
        UE_LOG(LogTemp, Error, TEXT("Socket creation failed"));
        WSACleanup();
        return;
    }

    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(7777);
    ServerAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(ListenSocket, (sockaddr*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
    {
        UE_LOG(LogTemp, Error, TEXT("Socket bind failed"));
        closesocket(ListenSocket);
        WSACleanup();
        return;
    }

    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        UE_LOG(LogTemp, Error, TEXT("Socket listen failed"));
        closesocket(ListenSocket);
        WSACleanup();
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Server is listening on port 7777"));

    while (!bStopServer)
    {
        SOCKET ClientSocket = accept(ListenSocket, nullptr, nullptr);
        if (ClientSocket == INVALID_SOCKET)
        {
            UE_LOG(LogTemp, Error, TEXT("Client connection failed"));
            continue;
        }

        UE_LOG(LogTemp, Log, TEXT("Client connected"));
        Async(EAsyncExecution::Thread, [this, ClientSocket]()
            {
                HandleClient(ClientSocket);
            });
    }

    closesocket(ListenSocket);
    WSACleanup();
}

void AServerManager::BeginDestroy()
{
    Super::BeginDestroy();
    bStopServer = true;
    UE_LOG(LogTemp, Log, TEXT("Server is shutting down"));
    CloseSocket();
}


void AServerManager::HandleClient(SOCKET ClientSocket)
{
    while (true)
    {
        char Buffer[1024];
        int BytesReceived = recv(ClientSocket, Buffer, sizeof(Buffer), 0);
        if (BytesReceived <= 0)
        {
            UE_LOG(LogTemp, Log, TEXT("Connection closed by client or error occurred"));
            break;
        }

        Buffer[BytesReceived] = '\0';
        FString Message = FString(ANSI_TO_TCHAR(Buffer));
        UE_LOG(LogTemp, Log, TEXT("Received message: %s"), *Message);

        if (Message == "Draw")
        {
            FString RowName = DrawCard();
            if (!RowName.IsEmpty())
            {
                send(ClientSocket, TCHAR_TO_ANSI(*RowName), RowName.Len(), 0);
                UE_LOG(LogTemp, Log, TEXT("Sent card row name: %s"), *RowName);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("No card available to draw!"));
            }
        }
    }
    closesocket(ClientSocket);
}

void AServerManager::StartServer()
{
    Async(EAsyncExecution::Thread, [this]()
        {
            RunServer();
        });
}

FString AServerManager::DrawCard()
{
    if (!DeckActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("DeckActor is not set!"));
        return TEXT("");
    }

    FString ResultRowName;

    // Прямий виклик ProcessEvent
    UFunction* Function = DeckActor->FindFunction(FName("DrawCard"));
    if (Function)
    {
        DeckActor->ProcessEvent(Function, &ResultRowName);
        UE_LOG(LogTemp, Log, TEXT("Card drawn: %s"), *ResultRowName);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("DrawCard function not found on DeckActor."));
    }

    return ResultRowName;
}



void AServerManager::CloseSocket()
{
    if (ListenSocket != INVALID_SOCKET)
    {
        closesocket(ListenSocket);
        ListenSocket = INVALID_SOCKET;
        UE_LOG(LogTemp, Log, TEXT("Socket successfully closed"));
    }
    WSACleanup();
}


