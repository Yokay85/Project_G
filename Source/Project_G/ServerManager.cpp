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

        UE_LOG(LogTemp, Log, TEXT("Client connected with socket %d."), ClientSocket);
        AddPlayer(ClientSocket, FString::Printf(TEXT("Player%d"), ClientSocket));

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
    FString ClientID = FString::Printf(TEXT("Socket_%d"), static_cast<int32>(ClientSocket));

    AddPlayer(ClientSocket, ClientID);

    while (true)
    {
        char Buffer[1024];
        int BytesReceived = recv(ClientSocket, Buffer, sizeof(Buffer), 0);
        if (BytesReceived <= 0)
        {
            UE_LOG(LogTemp, Log, TEXT("Connection closed by client or error occurred."));
            break;
        }

        Buffer[BytesReceived] = '\0';
        FString Message = FString(ANSI_TO_TCHAR(Buffer));
        UE_LOG(LogTemp, Log, TEXT("Received message from %s: %s"), *ClientID, *Message);

        if (Message == "Draw")
        {
            FString RowName = DrawCard();
            if (!RowName.IsEmpty())
            {
                AddCardToPlayer(ClientSocket, RowName);

                send(ClientSocket, TCHAR_TO_ANSI(*RowName), RowName.Len(), 0);
                UE_LOG(LogTemp, Log, TEXT("Sent card row name: %s to player %s."), *RowName, *ClientID);
            }
            else
            {
                FString ErrorMessage = TEXT("Deck is empty.");
                send(ClientSocket, TCHAR_TO_ANSI(*ErrorMessage), ErrorMessage.Len(), 0);
                UE_LOG(LogTemp, Warning, TEXT("Deck is empty."));
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

int32 AServerManager::CalculateScore(const TArray<FString>& PlayerCards)
{
    int32 Score = 0;
    int32 Aces = 0;

    for (const FString& Card : PlayerCards)
    {
        FString Suit;
        FString CardValue;

        for (int32 i = Card.Len() - 1; i >= 0; --i)
        {
            if (FChar::IsDigit(Card[i]) || Card[i] == 'A' || Card[i] == 'K' || Card[i] == 'Q' || Card[i] == 'J')
            {
                CardValue = Card.Mid(i);        
                Suit = Card.Left(i);           
                break;
            }
        }

        if (CardValue == "A")
        {
            Aces++;
            Score += 11; 
        }
        else if (CardValue == "K" || CardValue == "Q" || CardValue == "J")
        {
            Score += 10;
        }
        else
        {
            Score += FCString::Atoi(*CardValue); 
        }
    }

    while (Score > 21 && Aces > 0)
    {
        Score -= 10;
        Aces--;
    }

    return Score;
}

void AServerManager::AddPlayer(SOCKET ClientSocket, const FString& PlayerName)
{
    AsyncTask(ENamedThreads::GameThread, [this, ClientSocket, PlayerName]()
        {
            FString ClientID = FString::Printf(TEXT("Socket_%d"), static_cast<int32>(ClientSocket));

            if (!Players.Contains(ClientID))
            {
                FPlayerInfo NewPlayer;
                NewPlayer.PlayerName = PlayerName;
                Players.Add(ClientID, NewPlayer);

                UE_LOG(LogTemp, Log, TEXT("Player %s added with ClientID %s."), *PlayerName, *ClientID);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Player with ClientID %s already exists."), *ClientID);
            }
        });
}



void AServerManager::AddCardToPlayer(SOCKET ClientSocket, const FString& CardRowName)
{
    AsyncTask(ENamedThreads::GameThread, [this, ClientSocket, CardRowName]()
        {
            FString ClientID = FString::Printf(TEXT("Socket_%d"), static_cast<int32>(ClientSocket));

            if (Players.Contains(ClientID))
            {
                FPlayerInfo& Player = Players[ClientID];
                Player.PlayerCards.Add(CardRowName);

                // Оновлення рахунку
                Player.PlayerScore = CalculateScore(Player.PlayerCards);

                UE_LOG(LogTemp, Log, TEXT("Added card %s to player %s. Current score: %d"),
                    *CardRowName, *Player.PlayerName, Player.PlayerScore);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Player with ClientID %s not found."), *ClientID);
            }
        });
}




