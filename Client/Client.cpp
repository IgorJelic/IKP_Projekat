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

struct igracInfo
{
	char ime[15];
	bool prvi = false;
};


// TCP client that use non-blocking sockets
int main()
{
	// Socket used to communicate with server
	SOCKET connectSocket = INVALID_SOCKET;

	// Variable used to store function return value
	int iResult;

	// Buffer we will use to store message
	char dataBuffer[BUFFER_SIZE];

	// WSADATA data structure that is to receive details of the Windows Sockets implementation
	WSADATA wsaData;

	// Initialize windows sockets library for this process
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return 1;
	}

	// create a socket
	connectSocket = socket(AF_INET,
		SOCK_STREAM,
		IPPROTO_TCP);

	if (connectSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	// Create and initialize address structure
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;								// IPv4 protocol
	serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);	// ip address of server
	serverAddress.sin_port = htons(SERVER_PORT);					// server port

	// Connect to server specified in serverAddress and socket connectSocket
	iResult = connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
	if (iResult == SOCKET_ERROR)
	{
		printf("Unable to connect to server.\n");
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}
	
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
		char poruka[256];
		igracInfo igrac;

		while (true)
		{
			//// Unos potrebnih podataka koji ce se poslati serveru
			printf("Unesite ime igraca: ");
			gets_s(igrac.ime, 15);

			iResult = send(connectSocket, (char*)&igrac, (int)sizeof(igracInfo), 0);

			//// Check result of send function
			if (iResult == SOCKET_ERROR)
			{
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(connectSocket);
				WSACleanup();
				return 1;
			}



			//slanje poruke serveru
			while (true)
			{

				iResult = recv(connectSocket, dataBuffer, BUFFER_SIZE, 0);

				if (iResult > 0)	// Check if message is successfully received
				{
					dataBuffer[iResult] = '\0';

					// Log message text
					printf("Server sent: %s.\n", dataBuffer);


					/*printf("\nOpseg broja: ");

					gets_s(poruka);
					iResult = send(connectSocket, (char*)&poruka, strlen(poruka), 0);

					if (iResult == SOCKET_ERROR)
					{
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(connectSocket);
						WSACleanup();
						return 1;
					}*/


				}
				else if (iResult == 0)	// Check if shutdown command is received
				{
					// Connection was closed successfully
					printf("Connection with server closed.\n");
					closesocket(connectSocket);
					break;
					//WSACleanup();
					//return 0;
				}
				else	// There was an error during recv
				{

					printf("recv failed with error: %d\n", WSAGetLastError());
					closesocket(connectSocket);
					break;
					//WSACleanup();
					//return 1;
				}



				/*printf("\nPress 'x' to exit or any other key to continue: ");

				gets_s(poruka);
				iResult = send(connectSocket, (char*) &poruka, strlen(poruka), 0);

				if (iResult == SOCKET_ERROR)
				{
					printf("send failed with error: %d\n", WSAGetLastError());
					closesocket(connectSocket);
					WSACleanup();
					return 1;
				}

				printf("\nMessage successfully sent. Total bytes: %ld\n", iResult);
				if (_getch() == 'x')
				{
					break;
				}*/



			}


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