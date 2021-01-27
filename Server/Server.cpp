#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

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

#define SERVER_PORT 27016
#define BUFFER_SIZE 256
#define MAX_CLIENTS 3



struct igracInfo
{
	char ime[15];
	bool prvi = false;
	int broj_pokusaja = 0;
};

#pragma region FUNKCIJE
void WSAInitialization();
bool AllUsernamesRecieved(igracInfo* admin, igracInfo* p1, igracInfo* p2);

#pragma endregion

// TCP server that use non-blocking sockets
int main()
{
	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET deniedSocket = INVALID_SOCKET;
	SOCKET clientSockets[MAX_CLIENTS];
	// Index of last client
	int last = 0;

	int iResult;
	char dataBuffer[BUFFER_SIZE];

	// ADMIN PROMENLJIVE
	bool usernameRecievedFromAdmin = false;
	bool intervalRecievedFromAdmin = false;
	int intervalMin;
	int intervalMax;

	// PLAYERS PROMENLJIVE

	// klijenti
	igracInfo admin = { 0 };
	igracInfo player1 = { 0 };
	igracInfo player2 = { 0 };


	WSAInitialization();


	// Initialize serverAddress structure used by bind
	sockaddr_in serverAddress;
	memset((char*)&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;				// IPv4
	serverAddress.sin_addr.s_addr = INADDR_ANY;		// Use all available addresses
	serverAddress.sin_port = htons(SERVER_PORT);

	// Initialize all client_socket[] to 0 so not checked
	memset(clientSockets, 0, MAX_CLIENTS * sizeof(SOCKET));

	// Create a SOCKET for connecting to server 
	listenSocket = socket(AF_INET,      // IPv4 address family
		SOCK_STREAM,					// Stream socket
		IPPROTO_TCP);					// TCP protocol

	if (listenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket - bind port number and local address to socket
	iResult = bind(listenSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	//// All connections are by default accepted by protocol stek if socket is in listening mode.
	//// With SO_CONDITIONAL_ACCEPT parameter set to true, connections will not be accepted by default
	bool bOptVal = true;
	int bOptLen = sizeof(bool);
	iResult = setsockopt(listenSocket, SOL_SOCKET, SO_CONDITIONAL_ACCEPT, (char*)&bOptVal, bOptLen);
	if (iResult == SOCKET_ERROR) {
		printf("setsockopt for SO_CONDITIONAL_ACCEPT failed with error: %u\n", WSAGetLastError());
	}

	// Non blocking mode
	unsigned long  mode = 1;
	if (ioctlsocket(listenSocket, FIONBIO, &mode) != 0)
		printf("ioctlsocket failed with error.");

	// Set listenSocket in listening mode
	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	printf("Server socket is set to listening mode. Waiting for new connection requests.\n");

	// set of socket descriptors
	// added only set for reading, as a base of project
	fd_set readfds;
	//fd_set writefds;

	// timeout for select function
	timeval timeVal;
	timeVal.tv_sec = 1;
	timeVal.tv_usec = 0;

	igracInfo *igrac;
	int igraciCounter = 0;

	while (true)
	{
		// initialize socket set
		FD_ZERO(&readfds);

		FD_SET(listenSocket, &readfds);

		for (int i = 0; i < last; i++)
		{
			FD_SET(clientSockets[i], &readfds);
		}



		// wait for events on set						  &timeVal
		int selectResult = select(0, &readfds, NULL, NULL, NULL);

		if (selectResult == SOCKET_ERROR)
		{
			printf("Select failed with error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}
		else if (selectResult == 0) // timeout expired
		{

			continue;
		}
		else if (FD_ISSET(listenSocket, &readfds))
		{
			// Struct for information about connected client
			sockaddr_in clientAddr;
			int clientAddrSize = sizeof(struct sockaddr_in);

			if (last < MAX_CLIENTS)
			{
				// New connection request is received. Add new socket in array on first free position.
				clientSockets[last] = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);

				if (clientSockets[last] == INVALID_SOCKET)
				{
					if (WSAGetLastError() == WSAECONNRESET)
					{
						printf("accept failed, because timeout for client request has expired.\n");
					}
					else
					{
						printf("accept failed with error: %d\n", WSAGetLastError());
					}
				}
				else
				{
					// postavljanje klijent soketa u neblokirajuci rezim
					if (ioctlsocket(clientSockets[last], FIONBIO, &mode) != 0)
					{
						printf("ioctlsocket failed with error.");
						continue;
					}

					if (last == 0)
					{
						iResult = send(clientSockets[last], "Welcome! You are admin!", 23, 0);

						//// Check result of send function
						if (iResult == SOCKET_ERROR)
						{
							printf("send failed with error: %d\n", WSAGetLastError());
							closesocket(clientSockets[last]);
							WSACleanup();
							return 1;
						}
					}
					else
					{
						char msg[] = "";
						sprintf(msg, "Welcome, Player %d!", last);
						iResult = send(clientSockets[last], msg, 19, 0);

						//// Check result of send function
						if (iResult == SOCKET_ERROR)
						{
							printf("send failed with error: %d\n", WSAGetLastError());
							closesocket(clientSockets[last]);
							WSACleanup();
							return 1;
						}
					}

					
					if (last == 0)
					{
						printf("LOG>> New client request accepted (%d). Client address: %s : %d\tADMIN\n", last, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
					}
					else if (last == 1)
					{
						printf("LOG>> New client request accepted (%d). Client address: %s : %d\tPLAYER_1\n", last, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
					}
					else if (last == 2)
					{
						printf("LOG>> New client request accepted (%d). Client address: %s : %d\tPLAYER_2\n", last, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
					}

					last++;
				}
			}
			// u slucaju da pokusa da se poveze cetvrti klijent
			else
			{
				deniedSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
				if (deniedSocket == INVALID_SOCKET)
				{
					if (WSAGetLastError() == WSAECONNRESET)
					{
						printf("accept failed, because timeout for client request has expired.\n");
					}
					else
					{
						printf("accept failed with error: %d\n", WSAGetLastError());
					}
				}
				else
				{
					printf("LOG>> New client(%d) request denied. Client address: %s : %d\n", last+1, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

					send(deniedSocket, "No room!", 8, 0);

					Sleep(1000);

					//// Shutdown the connection since we're done
					//iResult = shutdown(deniedSocket, SD_BOTH);

					//// Check if connection is succesfully shut down.
					//if (iResult == SOCKET_ERROR)
					//{
					//	printf("Shutdown failed with error: %d\n", WSAGetLastError());
					//	closesocket(deniedSocket);
					//	WSACleanup();
					//	return 1;
					//}

					closesocket(deniedSocket);
				}
			}
		}
		else
		{
			// prijem poruke
			// prvo primamo poruke dokle god svi igraci ne unesu username!
			// posle toga primamo interval od ADMIN klijenta
			// za to vreme player1 bira kojom ce pretragom da se sluzi dok player2 dobija suprotan izbor pretrage
			// igrac koji bude imao izbor LINEARNA PRETRAGA, moze da bira da se linearna pretraga vrsi od prvog ili poslednjeg elementa u intervalu

			// Check if new message is received from connected clients
			for (int i = 0; i < last; i++)
			{
				// Check if new message is received from client on position "i"
				if (FD_ISSET(clientSockets[i], &readfds))
				{
					iResult = recv(clientSockets[i], dataBuffer, BUFFER_SIZE, 0);
					// poruka je primljena
					if (iResult > 0)
					{
						// AKO NISU SVI USERNAMEOVI PRIMLJENI
						if (!AllUsernamesRecieved(&admin, &player1, &player2))
						{
							if (i == 0)	// ADMIN
							{
								// ako nije primljen USERNAME admina
								if (!usernameRecievedFromAdmin)
								{
									dataBuffer[iResult] = '\0';
									printf("\nMessage received from ADMIN:\n");
									strcpy(admin.ime, dataBuffer);
									printf("Log>> Admin, username = %s\n", admin.ime);

									usernameRecievedFromAdmin = true;

									char message[] = "Send interval of numbers.";
									iResult = send(clientSockets[i], message, (int)strlen(message), 0);

									//// Check result of send function
									if (iResult == SOCKET_ERROR)
									{
										printf("send failed with error: %d\n", WSAGetLastError());
										closesocket(clientSockets[i]);
										WSACleanup();
										return 1;
									}

								}
								else
								{
									// ako interval nije primljen, primi interval i ispisi ga, kada se svi loguju, prosledices intervale igracima
									if (!intervalRecievedFromAdmin)
									{
										dataBuffer[iResult] = '\0';
										printf("\nMessage received from ADMIN:\n");

										char delimiter = ':';
										std::string s = dataBuffer;
										std::string minInterval = s.substr(0, s.find(delimiter));
										std::string maxInterval = s.substr(s.find(delimiter) + 1);

										intervalMin = std::stoi(minInterval);
										intervalMax = std::stoi(maxInterval);
										
										printf("Log>> Interval za pogadjanje = (%d - %d)\n", intervalMin, intervalMax);

										intervalRecievedFromAdmin = true;
									}
									// da li treba nesto u slucaju da je interval primljen a da svi igraci jos nisu ulogovani?

								}
								
							}
							else if (i == 1) // PLAYER1 
							{
								dataBuffer[iResult] = '\0';
								printf("\nMessage received from player1:\n");
								strcpy(player1.ime, dataBuffer);
								printf("Log>> player1, username = %s\n", player1.ime);

								if (!AllUsernamesRecieved(&admin, &player1, &player2))
								{
									char message[] = "Waiting for other clients to LogIn...";
									iResult = send(clientSockets[i], message, (int)strlen(message), 0);

									//// Check result of send function
									if (iResult == SOCKET_ERROR)
									{
										printf("send failed with error: %d\n", WSAGetLastError());
										closesocket(clientSockets[i]);
										WSACleanup();
										return 1;
									}
								}
								else
								{
									
								}
							}
							else if (i == 2) // PLAYER2 username unet
							{
								dataBuffer[iResult] = '\0';
								printf("\nMessage received from player2:\n");

								strcpy(player2.ime, dataBuffer);
								printf("Log>> player2, username = %s\n", player2.ime);

								if (!AllUsernamesRecieved(&admin, &player1, &player2))
								{
									char message[] = "Waiting for other clients to LogIn...";
									iResult = send(clientSockets[i], message, (int)strlen(message), 0);

									//// Check result of send function
									if (iResult == SOCKET_ERROR)
									{
										printf("send failed with error: %d\n", WSAGetLastError());
										closesocket(clientSockets[i]);
										WSACleanup();
										return 1;
									}
								}
								else
								{

								}
							}
						}
						

						

						//primljenoj poruci u memoriji pristupiti preko pokazivaca tipa (studentInfo *)
						//jer znamo format u kom je poruka poslata a to je struct studentInfo
						/*student = (studentInfo*)dataBuffer;

						printf("Ime i prezime: %s %s  \n", student->ime, student->prezime);

						printf("Poeni studenta: %d  \n", ntohs(student->poeni));
						printf("_______________________________  \n");*/

						/*igrac = (igracInfo*)dataBuffer;
						printf("Ime igraca: %s", igrac->ime);
						igraciCounter++;*/

					}
					else if (iResult == 0)
					{
						// connection was closed gracefully
						printf("Connection with client (%d) closed.\n", i + 1);
						closesocket(clientSockets[i]);

						// sort array and clean last place
						for (int j = i; j < last - 1; j++)
						{
							clientSockets[j] = clientSockets[j + 1];
						}
						clientSockets[last - 1] = 0;

						last--;
					}
					else
					{
						// there was an error during recv
						printf("recv failed with error: %d\n", WSAGetLastError());
						closesocket(clientSockets[i]);

						// sort array and clean last place
						for (int j = i; j < last - 1; j++)
						{
							clientSockets[j] = clientSockets[j + 1];
						}
						clientSockets[last - 1] = 0;

						last--;
					}
				}


			}

		}

			//dodati da kada se treci klijent uloguje igra moze da pocne
			
			if (igraciCounter == 3)
			{
				sprintf(dataBuffer, "Igra moze da pocne. Prvi igrac (ime) bira broj.");
				for (int i = 0; i < last; i++)
				{

					// Send message to clients using connected socket
					iResult = send(clientSockets[i], dataBuffer, (int)strlen(dataBuffer), 0);


					// Check result of send function
					if (iResult == SOCKET_ERROR)
					{
						printf("send failed with error: %d\n", WSAGetLastError());
						//shutdown(clientSockets[i], SD_BOTH);
						closesocket(clientSockets[i]);

						//break;
						WSACleanup();
						return 1;
					}

					memset(dataBuffer, 0, BUFFER_SIZE); //pokusala sam da dobijem prvog klijenta
					snprintf(dataBuffer, BUFFER_SIZE, "%d", i);
					iResult = send(clientSockets[i], dataBuffer, strlen(dataBuffer), 0);
				}


				/*int selectResult = select(0, NULL, &writefds, NULL, NULL);

				if (selectResult == SOCKET_ERROR)
				{
					printf("Select failed with error: %d\n", WSAGetLastError());
					closesocket(listenSocket);
					WSACleanup();
					return 1;
				}
				else if (selectResult == 0) // timeout expired
				{
					if (_kbhit()) //check if some key is pressed
					{
						_getch();
						printf("Primena racunarskih mreza u infrstrukturnim sistemima 2019/2020\n");
					}
					continue;
				}
				*/
			}
			
		
	

	}


	//Close listen and accepted sockets
	closesocket(listenSocket);

	// Deinitialize WSA library
	WSACleanup();

	return 0;
}

#pragma region DEFINICIJE_FJA
/// <summary>
/// Initcijalizacija WSA biblioteke
/// </summary>
void WSAInitialization()
{
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return;
	}
}

/// <summary>
/// Proverava da li su uneti svi usernameovi, if true, zavrsava se sa prijemom usernamova
/// </summary>
/// <param name="admin"></param>
/// <param name="p1"></param>
/// <param name="p2"></param>
/// <returns></returns>
bool AllUsernamesRecieved(igracInfo* admin, igracInfo* p1, igracInfo* p2)
{
	if (admin->ime[0] == '\0')
	{
		return false;
	}
	else if (p1->ime[0] == '\0')
	{
		return false;
	}
	else if (p2->ime[0] == '\0')
	{
		return false;
	}
	else
	{
		return true;
	}
}

#pragma endregion
