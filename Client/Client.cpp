#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "conio.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#pragma pack(1)

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 27016
#define BUFFER_SIZE 256


// varijable koje oznacavaju koja je uloga running klijenta
//bool admin = false;
//bool player1 = false;
//bool player2 = false;

enum Role { admin, player1, player2 };
enum Search { binarna, linearna_od_napred, linearna_od_nazad };
enum Znak { manje, vece, jednako }; // mozda ovako

bool readyForGame = false;


void WSAInitialization();

// TCP client that use non-blocking sockets
int main()
{
	SOCKET connectSocket = INVALID_SOCKET;
	int iResult;
	char dataBuffer[BUFFER_SIZE];

	Role role;
	Search searchP1;
	Search searchP2;

	// ADMIN PROMENLJIVE
	int adminZamisljenBroj = -1;
	int adminIntervalPocetak = -1;
	int adminIntervalKraj = -1;

	// IGRACI PROMENLJIVE
	char P1odabirPretrage[BUFFER_SIZE]; 
	char P2odabirPretrage[BUFFER_SIZE];
	int P1Pokusaj;
	int P2Pokusaj;


#pragma region CONNECTING

	WSAInitialization();

	connectSocket = socket(AF_INET,
		SOCK_STREAM,
		IPPROTO_TCP);

	if (connectSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;								// IPv4 protocol
	serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);	// ip address of server
	serverAddress.sin_port = htons(SERVER_PORT);					// server port

	iResult = connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
	if (iResult == SOCKET_ERROR)
	{
		printf("Unable to connect to server.\n");
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

#pragma endregion
	
	//""Welcome!You are admin!""
	//""Welcome, Player %d!""
	iResult = recv(connectSocket, dataBuffer, BUFFER_SIZE, 0);
	if (iResult > 0)
	{
		dataBuffer[iResult] = '\0';
		printf("Message received from server:\n");

		printf(">>\t%s\n", dataBuffer);

		printf("_______________________________  \n");


	}
	else if (iResult == 0)
	{
		// connection was closed gracefully
		printf("Connection with server closed.\n");
		closesocket(connectSocket);
	}
	else
	{
		// there was an error during recv
		printf("recv failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
	}

	// proverava substring od indexa 0 pa na dalje
	std::string s = dataBuffer;

	// Slucaj da je primljena poruka : No room!, zatvori klijent aplikaciju i konekciju
	if (s.rfind("No room!", 0) == 0)
	{
		printf("\n\n>> Client shuting down... Press any key...");

		// Shutdown the connection since we're done
		iResult = shutdown(connectSocket, SD_BOTH);

		// Check if connection is succesfully shut down.
		if (iResult == SOCKET_ERROR)
		{
			printf("Shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(connectSocket);
			WSACleanup();
			return 1;
		}

		Sleep(1000);

		// Close connected socket
		closesocket(connectSocket);

		// Deinitialize WSA library
		WSACleanup();

		_getch();
	}
	else
	{
		if (s.rfind("Welcome! You are admin!", 0) == 0)
		{
			role = admin;
			//admin = true;
		}
		if (s.rfind("Welcome, Player 1!", 0) == 0)
		{
			role = player1;
			//player1 = true;
		}
		if (s.rfind("Welcome, Player 2!", 0) == 0)
		{
			role = player2;
			//player2 = true;
		}
		

		char poruka[256];

		printf("Unesite ime igraca: ");
		gets_s(poruka, 15);

		iResult = send(connectSocket, poruka, (int)strlen(poruka), 0);

		if (iResult == SOCKET_ERROR)
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(connectSocket);
			WSACleanup();
			return 1;
		}

		switch (role)
		{
		case admin:
		{
			// SLANJE INTERVALA
			int retVal = 0;

			while (!retVal || (adminZamisljenBroj < 0))
			{
				printf("ADMIN>> Unesite zamisljeni broj: ");
				retVal = scanf_s("%d", &adminZamisljenBroj);
				getchar();
			}

			retVal = 0;

			bool valid = false;

			while (!valid)
			{
				printf("ADMIN>> Unesite pocetak intervala: ");
				retVal = scanf_s("%d", &adminIntervalPocetak);
				getchar();

				if (retVal == 1)
				{
					if (adminIntervalPocetak < adminZamisljenBroj)
					{
						valid = true;
					}
				}
			}

			retVal = 0;
			valid = false;


			while (!valid)
			{
				printf("ADMIN>> Unesite kraj intervala: ");
				retVal = scanf_s("%d", &adminIntervalKraj);
				getchar();

				if (retVal == 1)
				{
					if (adminIntervalKraj > adminZamisljenBroj)
					{
						valid = true;
					}
				}
			}

			char msg[BUFFER_SIZE];
			sprintf_s(msg, "%d:%d", adminIntervalPocetak, adminIntervalKraj);

			iResult = send(connectSocket, msg, (int)strlen(msg), 0);

			if (iResult == SOCKET_ERROR)
			{
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(connectSocket);
				WSACleanup();
				return 1;
			}
			
			break;
		}
		case player1:
		{
			// URADI VALIDACIJU UNOSA
			do
			{
				printf("PLAYER1>> Pretraga: Binarna (1) / Linearna od prvog(2) / Linearna od poslednjeg(3)\nPLAYER1>> ");
				gets_s(P1odabirPretrage, BUFFER_SIZE);
			} while ((strcmp(P1odabirPretrage, "1") != 0) && (strcmp(P1odabirPretrage, "2") != 0) && (strcmp(P1odabirPretrage, "3") != 0));
			

			if (strcmp(P1odabirPretrage, "1") == 0)
			{
				searchP1 = binarna;
			}
			else if (strcmp(P1odabirPretrage, "2") == 0)
			{
				searchP1 = linearna_od_napred;
			}
			else
			{
				searchP1 = linearna_od_nazad;
			}

			iResult = send(connectSocket, P1odabirPretrage, (int)strlen(P1odabirPretrage), 0);

			if (iResult == SOCKET_ERROR)
			{
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(connectSocket);
				WSACleanup();
				return 1;
			}
			break;
		}
		case player2:
		{

			break;
		}
		}

		// STAVLJANJE SOKETA U NEBLOKIRAJUCE STANJE
		fd_set readfds;

		while (true)
		{
			FD_ZERO(&readfds);
			FD_SET(connectSocket, &readfds);

			int selectResult = select(0, &readfds, NULL, NULL, NULL);

			if (selectResult == SOCKET_ERROR)
			{
				printf("Select failed with error: %d\n", WSAGetLastError());
				closesocket(connectSocket);
				WSACleanup();
				return 1;
			}
			else if (selectResult == 0) // timeout expired
			{

				continue;
			}
			else if (FD_ISSET(connectSocket, &readfds))
			{
				
			}
		}

		// odgovor na poslat username
		iResult = recv(connectSocket, dataBuffer, BUFFER_SIZE, 0);
		if (iResult > 0)
		{
			dataBuffer[iResult] = '\0';
			printf("Message received from server:\n");

			printf(">>\t%s\n", dataBuffer);

			printf("_______________________________  \n");
		}
		else if (iResult == 0)
		{
			// connection was closed gracefully
			printf("Connection with server closed.\n");
			closesocket(connectSocket);
		}
		else
		{
			// there was an error during recv
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(connectSocket);
		}		

		

		/*while (!readyForGame)
		{
			Sleep(1000);
			printf("Waiting for players...");
		}*/

		// SLANJE ODABRANE PRETRAGE
		if (player1)
		{
			
		}

		while (true)
		{
			
		
		}

		// Shutdown the connection since we're done
		iResult = shutdown(connectSocket, SD_BOTH);

		// Check if connection is succesfully shut down.
		if (iResult == SOCKET_ERROR)
		{
			printf("Shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(connectSocket);
			WSACleanup();
			return 1;
		}

		Sleep(1000);

		// Close connected socket
		closesocket(connectSocket);

		// Deinitialize WSA library
		WSACleanup();
	}

	

	return 0;
}

#pragma region FUNKCIJE
void WSAInitialization()
{
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return;
	}
}

#pragma endregion