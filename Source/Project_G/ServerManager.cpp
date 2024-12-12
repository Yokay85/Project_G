// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerManager.h"
#include "Engine/Engine.h"
#include <Misc/OutputDeviceNull.h>
#include "Async/Async.h"
#include "Kismet/KismetSystemLibrary.h"

FCriticalSection Mutex;


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

        if (Message == "IsMyTurn")
        {
            if (CurrentPlayerID.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("CurrentPlayerID is not set!"));
                FString Response = "false";
                send(ClientSocket, TCHAR_TO_ANSI(*Response), Response.Len(), 0);
            }
            else
            {
                FString Response = (IsPlayerTurn(ClientID)) ? "true" : "false";
                send(ClientSocket, TCHAR_TO_ANSI(*Response), Response.Len(), 0);
                UE_LOG(LogTemp, Log, TEXT("Sent IsMyTurn response to %s: %s (CurrentPlayerID: %s)"), *ClientID, *Response, *CurrentPlayerID);
            }
        }
        else if (Message == "Draw")
        {
            if (ClientID == CurrentPlayerID)
            {
                FString RowName = DrawCard();
                if (!RowName.IsEmpty())
                {
                    Mutex.Lock();
                    AddCardToPlayer(ClientSocket, RowName);
                    Mutex.Unlock();

                    send(ClientSocket, TCHAR_TO_ANSI(*RowName), RowName.Len(), 0);
                    UE_LOG(LogTemp, Log, TEXT("Sent card row name: %s to player %s."), *RowName, *ClientID);

                    PassTurnToNextPlayer();
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Deck is empty."));
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Player %s tried to act out of turn."), *ClientID);
            }
        }
        else if (Message == "Stand")
        {
            if (Players.Contains(ClientID))
            {
                FPlayerInfo& Player = Players[ClientID];
                Player.bIsStanding = true;

                UE_LOG(LogTemp, Log, TEXT("Player %s has chosen to stand. They will no longer take turns."), *ClientID);

                PassTurnToNextPlayer();

            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Player %s not found when sending Stand signal."), *ClientID);
            }
        }
        else if (Message == "GetMyScore")
        {
            if (Players.Contains(ClientID))
            {
                const FPlayerInfo& Player = Players[ClientID];
                int32 PlayerScore = Player.PlayerScore;

                FString Response = FString::Printf(TEXT("%d"), PlayerScore);
                send(ClientSocket, TCHAR_TO_ANSI(*Response), Response.Len(), 0);

                UE_LOG(LogTemp, Log, TEXT("Sent score %d to client %s."), PlayerScore, *ClientID);
            }
            else
            {
                FString Response = "-1";
                send(ClientSocket, TCHAR_TO_ANSI(*Response), Response.Len(), 0);

                UE_LOG(LogTemp, Warning, TEXT("Player with ClientID %s not found."), *ClientID);
            }
        }
        else if (Message == "GetIsStanding") 
        {
            if (Players.Contains(ClientID)) {
                const FPlayerInfo& Player = Players[ClientID];

                FString Response = Player.bIsStanding ? "true" : "false";
                send(ClientSocket, TCHAR_TO_ANSI(*Response), Response.Len(), 0);

                UE_LOG(LogTemp, Log, TEXT("Sent IsStanding response to %s: %s (CurrentPlayerID: %s)"), *ClientID, *Response, *CurrentPlayerID);
            } else {
                FString Response = "false";
                send(ClientSocket, TCHAR_TO_ANSI(*Response), Response.Len(), 0);

                UE_LOG(LogTemp, Warning, TEXT("Player with ClientID %s not found."), *ClientID);
            }
        }
        else if (Message == "CheckAllStanding")
        {
            bool AllStanding = AreAllPlayersStanding();
            FString Response = AllStanding ? "true" : "false";
            send(ClientSocket, TCHAR_TO_ANSI(*Response), Response.Len(), 0);

            UE_LOG(LogTemp, Log, TEXT("Sent CheckAllStanding response to %s: %s"), *ClientID, *Response);
        }
        else if (Message == "RequestGameTable")
        {
            FString GameTable = GenerateGameTable();
            send(ClientSocket, TCHAR_TO_ANSI(*GameTable), GameTable.Len(), 0);
            UE_LOG(LogTemp, Log, TEXT("Sent game table to client %s: %s"), *ClientID, *GameTable);
        }
        else if (Message == "Disconnect")
        {
            UE_LOG(LogTemp, Log, TEXT("Player %s disconnected."), *ClientID);

            Mutex.Lock();
            RemovePlayer(ClientID);
            Mutex.Unlock();
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
                if (i > 0 && Card.Mid(i - 1, 2) == "10")
                {
                    CardValue = "10";
                    Suit = Card.Left(i - 1);
                    break;
                }

                CardValue = Card.Mid(i, 1); 
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
        else if (CardValue.IsNumeric())
        {
            Score += FCString::Atoi(*CardValue); 
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid CardValue detected: %s"), *CardValue);
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

    FString ClientID = FString::Printf(TEXT("Socket_%d"), static_cast<int32>(ClientSocket));

    AsyncTask(ENamedThreads::GameThread, [this, ClientSocket, ClientID, PlayerName]()
        {
            if (!Players.Contains(ClientID))
            {
                FPlayerInfo NewPlayer;
                NewPlayer.PlayerName = PlayerName;
                NewPlayer.ClientSocket = ClientSocket;
                Players.Add(ClientID, NewPlayer);
                PlayerOrder.Add(ClientID);

                if (PlayerOrder.Num() == 1)
                {
                    CurrentPlayerID = ClientID;
                    CurrentPlayerIndex = 0;
                    UE_LOG(LogTemp, Log, TEXT("First player added. CurrentPlayerID set to %s"), *CurrentPlayerID);
                }

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
    FString ClientID = FString::Printf(TEXT("Socket_%d"), static_cast<int32>(ClientSocket));

    if (Players.Contains(ClientID))
    {
        FPlayerInfo& Player = Players[ClientID];
        Player.PlayerCards.Add(CardRowName);

        Player.PlayerScore = CalculateScore(Player.PlayerCards);

        UE_LOG(LogTemp, Log, TEXT("Added card %s to player %s. Current score: %d"),
            *CardRowName, *Player.PlayerName, Player.PlayerScore);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Player with ClientID %s not found."), *ClientID);
    }
}


void AServerManager::PassTurnToNextPlayer()
{
    if (PlayerOrder.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("No players left in the game."));
        return;
    }

    int32 InitialIndex = CurrentPlayerIndex;
    do
    {
        CurrentPlayerIndex = (CurrentPlayerIndex + 1) % PlayerOrder.Num();
        CurrentPlayerID = PlayerOrder[CurrentPlayerIndex];

        if (Players.Contains(CurrentPlayerID) && !Players[CurrentPlayerID].bIsStanding)
        {
            UE_LOG(LogTemp, Log, TEXT("It's now %s's turn."), *CurrentPlayerID);
            return; 
        }

    } while (CurrentPlayerIndex != InitialIndex);

    UE_LOG(LogTemp, Log, TEXT("All players are standing or no active players left. Game over."));
    CurrentPlayerID = "";
}




bool AServerManager::IsPlayerTurn(const FString& ClientID)
{
    return ClientID == CurrentPlayerID;
}

bool AServerManager::AreAllPlayersStanding() const
{
    for (const auto& Pair : Players)
    {
        if (!Pair.Value.bIsStanding)
        {
            return false;
        }
    }
    return true;
}

FString AServerManager::GenerateGameTable()
{
    FString Table = "GameTable::";
    for (const auto& Pair : Players)
    {
        const FPlayerInfo& Player = Pair.Value;
        Table += FString::Printf(TEXT("%s:%d;"), *Player.PlayerName, Player.PlayerScore);
    }

    UE_LOG(LogTemp, Log, TEXT("Generated Game Table: %s"), *Table);
    return Table;
}

void AServerManager::RemovePlayer(const FString& ClientID)
{
    if (Players.Contains(ClientID))
    {
        Players.Remove(ClientID);
        PlayerOrder.Remove(ClientID);

        UE_LOG(LogTemp, Log, TEXT("Removed player with ClientID: %s"), *ClientID);

        if (CurrentPlayerID == ClientID)
        {
            UE_LOG(LogTemp, Log, TEXT("Current player disconnected. Passing turn to the next player."));
            CurrentPlayerID = ""; 
            PassTurnToNextPlayer(); 
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Player with ClientID %s not found for removal."), *ClientID);
    }
}

void AServerManager::ShutdownServer()
{
    UE_LOG(LogTemp, Log, TEXT("Shutting down the server..."));

    for (auto& Pair : Players)
    {
        FPlayerInfo& Player = Pair.Value;
        if (Player.ClientSocket != INVALID_SOCKET)
        {
            closesocket(Player.ClientSocket);
            Player.ClientSocket = INVALID_SOCKET;
            UE_LOG(LogTemp, Log, TEXT("Closed connection for player: %s"), *Pair.Key);
        }
    }

    Players.Empty();
    PlayerOrder.Empty();
    CurrentPlayerID = "";

    if (ListenSocket != INVALID_SOCKET)
    {
        closesocket(ListenSocket);
        ListenSocket = INVALID_SOCKET;
        UE_LOG(LogTemp, Log, TEXT("Server socket closed."));
    }

    WSACleanup();
    UE_LOG(LogTemp, Log, TEXT("WinSock cleaned up. Server shutdown complete."));
}








