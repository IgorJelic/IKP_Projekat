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

struct Interval
{
	int min;
	int max;
};

#pragma region FUNKCIJE
void WSAInitialization();
bool AllUsernamesRecieved(igracInfo* admin, igracInfo* p1, igracInfo* p2);

#pragma endregion

// TCP server that use non-blocking sockets
int main()
{
	bool GAME_START = false;
	bool GAME_OVER = false;

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
	int tacnoCntr = 0;
	Interval interval = { 0 };

	// PLAYERS PROMENLJIVE
	bool usernameRecievedFromP1 = false;
	bool usernameRecievedFromP2 = false;
	bool pretragaRecievedFromP1 = false;
	bool pretragaRecievedFromP2 = false;
	char P1Pretraga[BUFFER_SIZE];
	char P2Pretraga[BUFFER_SIZE];

	// klijenti
	igracInfo admin = { 0 };
	igracInfo player1 = { 0 };
	igracInfo player2 = { 0 };

	#pragma region DIZANJE SERVERA
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
#pragma endregion


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
		//int selectResult = select(0, &readfds, NULL, NULL, NULL);
		int selectResult = select(0, &readfds, NULL, NULL, &timeVal); //na svakih sekund server proverava da li je game ready to start?

		if (selectResult == SOCKET_ERROR)
		{
			printf("Select failed with error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}
		// READY TO START
		// na svakih timeVal sekundi (1sec), proveri da li je game ready to play
		else if (selectResult == 0) // timeout expired
		{
			if (usernameRecievedFromAdmin && usernameRecievedFromP1 && usernameRecievedFromP2 && intervalRecievedFromAdmin && pretragaRecievedFromP1 && pretragaRecievedFromP2 && !GAME_START)
			{
				GAME_START = true;

				printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n>> GAME READY TO START!\n\n");

				char msg[BUFFER_SIZE];
				sprintf_s(msg, "ADMIN = %s\nPLAYER1 = %s\nPLAYER2 = %s\n\nINTERVAL (%d - %d)\nPRETRAGA P1 = %s\nPRETRAGA P2 = %s\n", 
										admin.ime, player1.ime, player2.ime, interval.min, interval.max, P1Pretraga, P2Pretraga);

				printf("%s", msg);
				// SLANJE SVIMA PORUKE KOJA SADRZI INFORMACIJE O IGRACIMA I ELEMENTIMA IGRE
				for (int i = 0; i < last; i++)
				{
					iResult = send(clientSockets[i], msg, (int)strlen(msg), 0);

					if (iResult == SOCKET_ERROR)
					{
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(clientSockets[i]);
						WSACleanup();
						return 1;
					}
				}

				//sprintf_s(msg, "%d:%d", interval.min, interval.max);  (int)strlen(msg)
				// 		iResult = send( connectSocket, (char*) &student, (int)sizeof(studentInfo), 0 );

				// SLANJE INTERVALA KLIJENTIMA
				for (int i = 1; i < last; i++)
				{
					iResult = send(clientSockets[i], (char*) &interval, (int)sizeof(interval), 0);

					if (iResult == SOCKET_ERROR)
					{
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(clientSockets[i]);
						WSACleanup();
						return 1;
					}
				}
				//_getch();
			}
			continue;
		}
		// prihvatanje klijenata
		else if (FD_ISSET(listenSocket, &readfds))
		{
			// Struct for information about connected client
			sockaddr_in clientAddr;
			int clientAddrSize = sizeof(struct sockaddr_in);

			// prihvatnje prva 3 klijenta
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
						char msg[] = "Welcome! You are admin!";
						iResult = send(clientSockets[last], msg, (int)strlen(msg), 0);

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
						iResult = send(clientSockets[last], msg, (int)strlen(msg), 0);

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
			// u slucaju da pokusa da se poveze 4. klijent
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
			// ako je GAME_START, vrti petlju sve dok nije GAME_OVER
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
					// PORUKA JE PRIMLJENA
					if (iResult > 0)
					{
						#pragma region GAME
						// DOK TRAJE IGRA
						if (GAME_START == true && GAME_OVER == false)
						{
							
							
							switch (i)
							{
								// ADMIN, prima odgovor za odgovarajuceg igraca(key), oduzmi KEY iz odgovora i prosledi odgovor do odgovarajuceg igraca
								case 0:
								{
									// VAL:KEY
									char delimiter = ':';
									dataBuffer[iResult] = '\0';

									std::string sValKey = dataBuffer;
									std::string sVal = sValKey.substr(0, sValKey.find(delimiter));	// VALUE koju saljem odredjenom klijentu
									std::string sKey = sValKey.substr(sValKey.find(delimiter) + 1);
									char csKey[BUFFER_SIZE];
									char csVal[BUFFER_SIZE];
									strcpy(csKey, sKey.c_str());
									strcpy(csVal, sVal.c_str());

									printf("\nGAME>> ADMIN [%s:%s]", csVal, csKey);

									// prosledi vrednost igracu 1
									if (strcmp(csKey, "1") == 0)
									{
										int tempResult = send(clientSockets[1], csVal, (int)strlen(csVal), 0);
										//// Check result of send function
										if (tempResult == SOCKET_ERROR)
										{
											printf("send failed with error: %d\n", WSAGetLastError());
											closesocket(clientSockets[1]);
											WSACleanup();
											return 1;
										}
									}
									// prosledi vrednost igracu 2
									else if (strcmp(csKey, "2") == 0)
									{
										int tempResult = send(clientSockets[2], csVal, (int)strlen(csVal), 0);
										//// Check result of send function
										if (tempResult == SOCKET_ERROR)
										{
											printf("send failed with error: %d\n", WSAGetLastError());
											closesocket(clientSockets[2]);
											WSACleanup();
											return 1;
										}
									}

									if (strcmp(csVal, "TACNO") == 0)
									{
										tacnoCntr++;
										if (tacnoCntr > 1)
										{
											GAME_OVER = true;
										}
									}

									break;
								}
								// PLAYER1, primljena poruka od P1 se samo prosledjuje ADMINU i povecava se broj pokusaja
								case 1:
								{
									dataBuffer[iResult] = '\0';

									int tempResult = send(clientSockets[0], dataBuffer, (int)strlen(dataBuffer), 0);
									//// Check result of send function
									if (tempResult == SOCKET_ERROR)
									{
										printf("send failed with error: %d\n", WSAGetLastError());
										closesocket(clientSockets[0]);
										WSACleanup();
										return 1;
									}

									player1.broj_pokusaja++;
									printf("\nGAME>> P1 [%d]", player1.broj_pokusaja);

									//Sleep(10000);

									break;
								}
								// PLAYER2, primljena poruka od P2 se samo prosledjuje ADMINU i povecava se broj pokusaja
								case 2:
								{
									char msg[BUFFER_SIZE];
									dataBuffer[iResult] = '\0';

									strcpy(msg, dataBuffer);

									int tempResult = send(clientSockets[0], msg, (int)strlen(msg), 0);
									//// Check result of send function
									if (tempResult == SOCKET_ERROR)
									{
										printf("send failed with error: %d\n", WSAGetLastError());
										closesocket(clientSockets[0]);
										WSACleanup();
										return 1;
									}

									player2.broj_pokusaja++;
									printf("\nGAME>> P2 [%d]", player2.broj_pokusaja);

									//Sleep(10000);

									break;
								}
							}	
						}
						#pragma endregion

						#pragma region PREPARATION
						if (!GAME_START)
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

										interval.min = std::stoi(minInterval);
										interval.max = std::stoi(maxInterval);

										printf("Log>> Interval za pogadjanje = (%d - %d)\n", interval.min, interval.max);

										intervalRecievedFromAdmin = true;
									}
								}

							}
							else if (i == 1) // PLAYER1 
							{
								// ako username jos nije primljen
								if (!usernameRecievedFromP1)
								{
									dataBuffer[iResult] = '\0';
									printf("\nMessage received from player1:\n");
									strcpy(player1.ime, dataBuffer);
									printf("Log>> player1, username = %s\n", player1.ime);

									usernameRecievedFromP1 = true;
								}
								// username primljen, na redu je odabir pretrage
								// obradi odabir pretrage, sacuvaj ga, pa ga salji kad bude GAMESTARTREADY
								else
								{
									dataBuffer[iResult] = '\0';


									// ako je PLAYER1 odabrao binarnu pretragu, PLAYER2 ima izbor da bira koju od dve linearne varijante zeli
									if (strcmp(dataBuffer, "1") == 0)
									{
										strcpy(P1Pretraga, "Binarna");
										strcpy(P2Pretraga, "Linearna");
									}
									// ako je PLAYER1 odabrao linearnu pretragu, znaci da PLAYER2 dobija binarnu pretragu
									else if (strcmp(dataBuffer, "2") == 0)
									{
										strcpy(P1Pretraga, "Linearna(Front)");
										strcpy(P2Pretraga, "Binarna");
									}
									else
									{
										strcpy(P1Pretraga, "Linearna(Back)");
										strcpy(P2Pretraga, "Binarna");
									}

									int tempResult = send(clientSockets[i + 1], P2Pretraga, (int)strlen(P2Pretraga), 0);
									//// Check result of send function
									if (tempResult == SOCKET_ERROR)
									{
										printf("send failed with error: %d\n", WSAGetLastError());
										closesocket(clientSockets[i]);
										WSACleanup();
										return 1;
									}

									printf("\nMessage received from player1: %s\n", P1Pretraga);

									pretragaRecievedFromP1 = true;
								}
							}
							else if (i == 2) // PLAYER2 
							{
								// ako nije primljen username
								if (!usernameRecievedFromP2)
								{
									dataBuffer[iResult] = '\0';
									printf("\nMessage received from player2:\n");

									strcpy(player2.ime, dataBuffer);
									printf("Log>> player2, username = %s\n", player2.ime);

									usernameRecievedFromP2 = true;
								}
								// ako jeste primljen, sledeca poruka koju prima je : ILI odabir koja linearna pretraga ILI potvrda binarne pretrage
								else
								{
									dataBuffer[iResult] = '\0';

									if (strcmp(dataBuffer, "0") == 0)
									{
										strcpy(P2Pretraga, "Binarna");
									}
									else if (strcmp(dataBuffer, "1") == 0)
									{
										strcpy(P2Pretraga, "Linearna(Front)");
									}
									else
									{
										strcpy(P2Pretraga, "Linearna(Back)");
									}

									pretragaRecievedFromP2 = true;

									printf("\nMessage received from player2: %s\n", P2Pretraga);


								}

							}

						}
						#pragma endregion
					}
					else if (iResult == 0)
					{
						// connection was closed gracefully
						printf("\nConnection with client (%d) closed.\n", i + 1);
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
