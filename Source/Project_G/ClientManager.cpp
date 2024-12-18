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
        Buffer[BytesReceived] = '\0';
        FString Response = FString(ANSI_TO_TCHAR(Buffer));
        return Response;
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

void AClientManager::SendIsMyTurnRequest()
{
    FString Message = "IsMyTurn";
    send(ClientSocket, TCHAR_TO_ANSI(*Message), Message.Len(), 0);
    UE_LOG(LogTemp, Log, TEXT("Sent message: %s"), *Message);
}

FString AClientManager::ReceiveIsMyTurnResponse()
{
    char Buffer[1024];
    int BytesReceived = recv(ClientSocket, Buffer, sizeof(Buffer), 0);
    if (BytesReceived > 0)
    {
        Buffer[BytesReceived] = '\0';
        FString Response = FString(ANSI_TO_TCHAR(Buffer));
        UE_LOG(LogTemp, Log, TEXT("Received response: %s"), *Response);

        return Response;
    }
    UE_LOG(LogTemp, Log, TEXT("Sent message: %s"), "Failed to recive response");
    return "false";
}

void AClientManager::SendStandMessage()
{
    FString Message = "Stand";
    send(ClientSocket, TCHAR_TO_ANSI(*Message), Message.Len(), 0);

    UE_LOG(LogTemp, Log, TEXT("Sent message: %s"), *Message);
}

void AClientManager::RequestScore()
{
    if (ClientSocket == INVALID_SOCKET)
    {
        UE_LOG(LogTemp, Warning, TEXT("Client is not connected to the server."));
        return;
    }

    FString Message = "GetMyScore";
    send(ClientSocket, TCHAR_TO_ANSI(*Message), Message.Len(), 0);

    UE_LOG(LogTemp, Log, TEXT("Sent request for player's own score."));
}

int32 AClientManager::ReceiveScore()
{
    if (ClientSocket == INVALID_SOCKET)
    {
        UE_LOG(LogTemp, Warning, TEXT("Client is not connected to the server."));
        return -1;
    }

    char Buffer[1024];
    int BytesReceived = recv(ClientSocket, Buffer, sizeof(Buffer), 0);

    if (BytesReceived > 0)
    {
        Buffer[BytesReceived] = '\0';
        FString Response = FString(ANSI_TO_TCHAR(Buffer));
        UE_LOG(LogTemp, Log, TEXT("Received score response: %s"), *Response);


        return FCString::Atoi(*Response);
    }

    UE_LOG(LogTemp, Warning, TEXT("Failed to receive score response."));
    return -1;
}

void AClientManager::RequestIsStanding()
{
    if (ClientSocket == INVALID_SOCKET) {
        UE_LOG(LogTemp, Warning, TEXT("Client is not connected to the server."));
        return;
    }

    FString Message = "GetIsStanding";
    send(ClientSocket, TCHAR_TO_ANSI(*Message), Message.Len(), 0);

    UE_LOG(LogTemp, Log, TEXT("Sent request for player's standing status."));
}

FString AClientManager::ReceiveIsStanding()
{
    if (ClientSocket == INVALID_SOCKET) {
        UE_LOG(LogTemp, Warning, TEXT("Client is not connected to the server."));
        return "";
    }

    char Buffer[1024];
    int BytesReceived = recv(ClientSocket, Buffer, sizeof(Buffer), 0);

    if (BytesReceived > 0) {
        Buffer[BytesReceived] = '\0';
        FString Response = FString(ANSI_TO_TCHAR(Buffer));
        UE_LOG(LogTemp, Log, TEXT("Received IsStanding response: %s"), *Response);
        return Response;
    }

    UE_LOG(LogTemp, Warning, TEXT("Failed to receive IsStanding response."));
    return "";
}

bool AClientManager::CheckAllStanding()
{
    FString Message = "CheckAllStanding";
    send(ClientSocket, TCHAR_TO_ANSI(*Message), Message.Len(), 0);

    char Buffer[1024];
    int BytesReceived = recv(ClientSocket, Buffer, sizeof(Buffer), 0);
    if (BytesReceived > 0)
    {
        Buffer[BytesReceived] = '\0';
        FString Response = FString(ANSI_TO_TCHAR(Buffer));
        UE_LOG(LogTemp, Log, TEXT("Received CheckAllStanding response: %s"), *Response);

        return Response == "true";
    }

    UE_LOG(LogTemp, Warning, TEXT("Failed to receive CheckAllStanding response."));
    return false;
}

void AClientManager::RequestGameTable()
{
    if (ClientSocket == INVALID_SOCKET)
    {
        UE_LOG(LogTemp, Warning, TEXT("Client is not connected to the server."));
        return;
    }

    FString Message = "RequestGameTable";
    send(ClientSocket, TCHAR_TO_ANSI(*Message), Message.Len(), 0);

    UE_LOG(LogTemp, Log, TEXT("Sent request for game table."));
}

TArray<FPlayerScoreEntry> AClientManager::ReceiveGameTable()
{
    TArray<FPlayerScoreEntry> GameTable;

    if (ClientSocket == INVALID_SOCKET)
    {
        UE_LOG(LogTemp, Warning, TEXT("Client is not connected to the server."));
        return GameTable;
    }

    char Buffer[2048];
    int BytesReceived = recv(ClientSocket, Buffer, sizeof(Buffer), 0);

    if (BytesReceived > 0)
    {
        Buffer[BytesReceived] = '\0';
        FString Response = FString(ANSI_TO_TCHAR(Buffer));
        UE_LOG(LogTemp, Log, TEXT("Received game table response: %s"), *Response);

        if (Response.StartsWith("GameTable::"))
        {
            FString TableData = Response.Mid(11); // Remove "GameTable::"
            TArray<FString> Entries;
            TableData.ParseIntoArray(Entries, TEXT(";"), true);

            for (const FString& Entry : Entries)
            {
                TArray<FString> Pair;
                Entry.ParseIntoArray(Pair, TEXT(":"), true);
                if (Pair.Num() == 2)
                {
                    FPlayerScoreEntry PlayerEntry;
                    PlayerEntry.PlayerName = Pair[0];
                    PlayerEntry.PlayerScore = FCString::Atoi(*Pair[1]);
                    GameTable.Add(PlayerEntry);
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid game table response: %s"), *Response);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to receive game table response."));
    }

    return GameTable;
}

void AClientManager::SendDisconnectMessage()
{
    if (ClientSocket == INVALID_SOCKET)
    {
        UE_LOG(LogTemp, Warning, TEXT("Client is not connected to the server."));
        return;
    }

    FString Message = "Disconnect";
    send(ClientSocket, TCHAR_TO_ANSI(*Message), Message.Len(), 0);

    UE_LOG(LogTemp, Log, TEXT("Sent disconnect message to server."));
}

void AClientManager::DisconnectFromServer()
{
    SendDisconnectMessage();

    if (ClientSocket != INVALID_SOCKET)
    {
        closesocket(ClientSocket);
        ClientSocket = INVALID_SOCKET;

        UE_LOG(LogTemp, Log, TEXT("Client disconnected from the server."));
    }
}

void AClientManager::RequestPlayerName()
{
    if (ClientSocket == INVALID_SOCKET)
    {
        UE_LOG(LogTemp, Warning, TEXT("Client is not connected to the server."));
        return;
    }

    FString Message = "GetMyName";
    send(ClientSocket, TCHAR_TO_ANSI(*Message), Message.Len(), 0);
    UE_LOG(LogTemp, Log, TEXT("Sent request for player's name."));
}

FString AClientManager::ReceivePlayerName()
{
    if (ClientSocket == INVALID_SOCKET)
    {
        UE_LOG(LogTemp, Warning, TEXT("Client is not connected to the server."));
        return "Unknown";
    }

    char Buffer[1024];
    int BytesReceived = recv(ClientSocket, Buffer, sizeof(Buffer), 0);

    if (BytesReceived > 0)
    {
        Buffer[BytesReceived] = '\0';
        FString PlayerName = FString(ANSI_TO_TCHAR(Buffer));
        UE_LOG(LogTemp, Log, TEXT("Received player name: %s"), *PlayerName);
        return PlayerName;
    }

    UE_LOG(LogTemp, Warning, TEXT("Failed to receive player name response."));
    return "Unknown";
}










