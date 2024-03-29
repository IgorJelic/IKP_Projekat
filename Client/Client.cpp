#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <chrono>
#include "conio.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#pragma pack(1)

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 27016
#define BUFFER_SIZE 256


struct Interval
{
	int min;
	int max;
};

enum Role { admin, player1, player2 };
enum Search { binarna, linearna_od_napred, linearna_od_nazad };
enum Znak { manje, vece, jednako }; // mozda ovako

bool readyForGame = false;

#pragma region FUNKCIJE
void WSAInitialization();
int Binary(int intervalMin, int intervalMax);
int LinearFront(int intervalMin);
int LinearBack(int intervalMax);
#pragma endregion

// TCP client that use non-blocking sockets
int main()
{
	bool GAME_OVER = false;

	SOCKET connectSocket = INVALID_SOCKET;
	int iResult;
	char dataBuffer[BUFFER_SIZE];

	Role role;
	Search searchP1;
	Search searchP2;
	Interval *interval = { 0 };

	// ADMIN PROMENLJIVE
	int adminZamisljenBroj = -1;
	int adminIntervalPocetak = -1;
	int adminIntervalKraj = -1;
	int tacnoCntr = 0;

	int intervalPocetak = -1;
	int intervalKraj = -1;

	// IGRACI PROMENLJIVE
	char P1odabirPretrage[BUFFER_SIZE]; 
	char P2odabirPretrage[BUFFER_SIZE];
	int P1Pokusaj;
	int P2Pokusaj;
	int P1Vreme = 0;
	int P2Vreme = 0;


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
		}
		else if (s.rfind("Welcome, Player 1!", 0) == 0)
		{
			role = player1;
		}
		else if (s.rfind("Welcome, Player 2!", 0) == 0)
		{
			role = player2;
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

		#pragma region PRIPREME ZA START IGRE
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
					iResult = recv(connectSocket, dataBuffer, BUFFER_SIZE, 0);
					if (iResult > 0)
					{
						dataBuffer[iResult] = '\0';
						printf("Message received from server:\n");
						printf(">>\t%s\n", dataBuffer);
						printf("_______________________________  \n");

						// ako je PLAYERU2 dodeljena linearna pretraga, ima opciju da pocne s kraja ili s pocetka intervala
						if (strcmp(dataBuffer, "Linearna") == 0)
						{
							do
							{
								printf("PLAYER2>> Linearna pretraga: Od prvog(1) / Od poslednjeg(2)\nPLAYER2>> ");
								gets_s(P2odabirPretrage, BUFFER_SIZE);
							} while ((strcmp(P2odabirPretrage, "1") != 0) && (strcmp(P2odabirPretrage, "2") != 0));

							if (strcmp(P2odabirPretrage, "1") == 0)
							{
								searchP2 = linearna_od_napred;
							}
							else
							{
								searchP2 = linearna_od_nazad;
							}

						}
						else
						{
							searchP2 = binarna;
							strcpy(P2odabirPretrage, "0");
						}

						// potvrda serveru o odabiru pretrage PLAYERA2
						int tempResult = send(connectSocket, P2odabirPretrage, (int)strlen(P2odabirPretrage), 0);
						//// Check result of send function
						if (tempResult == SOCKET_ERROR)
						{
							printf("send failed with error: %d\n", WSAGetLastError());
							closesocket(connectSocket);
							WSACleanup();
							return 1;
						}


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

					break;
				}
				}
		#pragma endregion


		// Cekanje na prijem poruke za pocetak igre
		iResult = recv(connectSocket, dataBuffer, BUFFER_SIZE, 0);
		if (iResult > 0)
		{
			printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
			dataBuffer[iResult] = '\0';
			printf("%s", dataBuffer);
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

		// prijem intervala od strane igraca
		if (role != admin)
		{
			iResult = recv(connectSocket, dataBuffer, BUFFER_SIZE, 0);
			if (iResult > 0)
			{
				dataBuffer[iResult] = '\0';
				interval = (Interval*)dataBuffer;

				intervalPocetak = interval->min;
				intervalKraj = interval->max;
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
		}

		printf("\n\n>> Game starts in 5 seconds...");
		Sleep(5000);

		#pragma region GAME_START
		// palim tajmer
		auto t1 = std::chrono::high_resolution_clock::now();
		while (GAME_OVER == false)
		{
			switch (role)
			{
				// prima broj, vraca manje vece ili jednako
				case admin:
				{
					#pragma region PRIJEM POKUSAJA I VRACANJE ODGOVORA u skladu sa rezultatom
					iResult = recv(connectSocket, dataBuffer, BUFFER_SIZE, 0);
					if (iResult> 0)
					{
						// moras skontati kom klijentu treba da se vrati obavestenje
						// neka format bude val:key => val = pogadjanje, key = koji igrac

						dataBuffer[iResult] = '\0';

						char delimiter = ':';
						std::string sValKey = dataBuffer;
						std::string sVal = sValKey.substr(0, sValKey.find(delimiter));
						std::string sKey = sValKey.substr(sValKey.find(delimiter) + 1);

						int val = std::stoi(sVal);

						char sendMsg[BUFFER_SIZE];
						if (val == adminZamisljenBroj)
						{
							if (strcmp(sKey.c_str(), "1"))
							{
								auto t11 = std::chrono::high_resolution_clock::now();
								auto durationP1 = std::chrono::duration_cast<std::chrono::milliseconds>(t11 - t1).count();

								P1Vreme = (int)durationP1;
							}
							else
							{
								auto t21 = std::chrono::high_resolution_clock::now();
								auto durationP2 = std::chrono::duration_cast<std::chrono::milliseconds>(t21 - t1).count();

								P2Vreme = (int)durationP2;
							}
							strcpy(sendMsg, "TACNO");
							tacnoCntr++;
							if (tacnoCntr > 1)
							{
								GAME_OVER = true;
							}							
						}
						else if (val < adminZamisljenBroj)
						{
							strcpy(sendMsg, "VECE");
						}
						else
						{
							strcpy(sendMsg, "MANJE");
						}

						sValKey = sendMsg;
						sValKey += ":" + sKey;

						strcpy(sendMsg, sValKey.c_str());

						int tempResult = send(connectSocket, sendMsg, (int)strlen(sendMsg), 0);
						//// Check result of send function
						if (tempResult == SOCKET_ERROR)
						{
							printf("send failed with error: %d\n", WSAGetLastError());
							closesocket(connectSocket);
							WSACleanup();
							return 1;
						}
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
					#pragma endregion

					break;
				}
				// igrac 1 prvo salje pa prima odgovor sve dok ne primi TACNO ili GAMEOVER
				case player1:
				{
					switch (searchP1)
					{
						// BINARY
					case binarna:
					{
						#pragma region SLANJE_POKUSAJA
						std::string key = ":1";
						int sendVal = Binary(intervalPocetak, intervalKraj);
						std::string val = std::to_string(sendVal);
						val += key;

						char msg[BUFFER_SIZE];
						strcpy(msg, val.c_str());

						int tempResult = send(connectSocket, msg, (int)strlen(msg), 0);
						//// Check result of send function
						if (tempResult == SOCKET_ERROR)
						{
							printf("send failed with error: %d\n", WSAGetLastError());
							closesocket(connectSocket);
							WSACleanup();
							return 1;
						}
#pragma endregion

						#pragma region PRIJEM_ODGOVORA
						iResult = recv(connectSocket, dataBuffer, BUFFER_SIZE, 0);
						if (iResult > 0)
						{
							dataBuffer[iResult] = '\0'; // MANJE VECE TACNO
							printf("\nPRIJEM>> %s", dataBuffer);
							if (strcmp(dataBuffer, "MANJE") == 0)
							{
								intervalKraj = sendVal - 1;
							}
							else if (strcmp(dataBuffer, "VECE") == 0)
							{
								intervalPocetak = sendVal + 1;
							}
							else if (strcmp(dataBuffer, "TACNO") == 0)
							{
								//POBEDNIK = true;
								GAME_OVER = true;
							}
							else if (strcmp(dataBuffer, "GAMEOVER") == 0)
							{
								GAME_OVER = true;
							}
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
						#pragma endregion

						break;
					}
					// LINEAR FRONT
					case linearna_od_napred:
					{
						#pragma region SLANJE_POKUSAJA
						std::string key = ":1";
						int sendVal = LinearFront(intervalPocetak);
						std::string val = std::to_string(sendVal);
						val += key;

						char msg[BUFFER_SIZE];
						strcpy(msg, val.c_str());

						int tempResult = send(connectSocket, msg, (int)strlen(msg), 0);
						//// Check result of send function
						if (tempResult == SOCKET_ERROR)
						{
							printf("send failed with error: %d\n", WSAGetLastError());
							closesocket(connectSocket);
							WSACleanup();
							return 1;
						}
#pragma endregion

						#pragma region PRIJEM_ODGOVORA
						iResult = recv(connectSocket, dataBuffer, BUFFER_SIZE, 0);
						if (iResult > 0)
						{
							dataBuffer[iResult] = '\0'; // VECE TACNO, zato sto ne moze biti manje jer krece od pocetka intervala redom
							printf("\nPRIJEM>> %s", dataBuffer);
							/*if (strcmp(dataBuffer, "MANJE") == 0)
							{
								interval->max = sendVal - 1;
							}*/
							if (strcmp(dataBuffer, "VECE") == 0)
							{
								intervalPocetak = sendVal + 1;
							}
							else if (strcmp(dataBuffer, "TACNO") == 0)
							{
								// POBEDNIK = true;
								GAME_OVER = true;
							}
							else if (strcmp(dataBuffer, "GAMEOVER") == 0)
							{
								GAME_OVER = true;
							}
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
						#pragma endregion

						break;
					}
					// LINEAR BACK
					case linearna_od_nazad:
					{
						#pragma region SLANJE_POKUSAJA
						std::string key = ":1";
						int sendVal = LinearBack(intervalKraj);
						std::string val = std::to_string(sendVal);
						val += key;

						char msg[BUFFER_SIZE];
						strcpy(msg, val.c_str());

						int tempResult = send(connectSocket, msg, (int)strlen(msg), 0);
						//// Check result of send function
						if (tempResult == SOCKET_ERROR)
						{
							printf("send failed with error: %d\n", WSAGetLastError());
							closesocket(connectSocket);
							WSACleanup();
							return 1;
						}
						#pragma endregion

						#pragma region PRIJEM_ODGOVORA
						iResult = recv(connectSocket, dataBuffer, BUFFER_SIZE, 0);
						if (iResult > 0)
						{
							dataBuffer[iResult] = '\0'; // MANJE TACNO, zato sto ne moze biti vece jer krece od kraja intervala redom
							printf("\nPRIJEM>> %s", dataBuffer);
							if (strcmp(dataBuffer, "MANJE") == 0)
							{
								intervalKraj = sendVal - 1;
							}
							/*if (strcmp(dataBuffer, "VECE") == 0)
							{
								interval->min = sendVal + 1;
							}*/
							else if (strcmp(dataBuffer, "TACNO") == 0)
							{
								//POBEDNIK = true;
								GAME_OVER = true;
							}
							else if (strcmp(dataBuffer, "GAMEOVER") == 0)
							{
								GAME_OVER = true;
							}
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
						#pragma endregion

						break;
					}
					}

					break;
				}
				// igrac 2 prvo salje pa prima odgovor sve dok ne primi TACNO ili GAMEOVER
				case player2:
				{
					switch (searchP2)
					{
						// BINARY
						case binarna:
						{
							#pragma region SLANJE_POKUSAJA
							std::string key = ":2";
							int sendVal = Binary(intervalPocetak, intervalKraj);
							std::string val = std::to_string(sendVal);
							val += key;

							char msg[BUFFER_SIZE];
							strcpy(msg, val.c_str());

							int tempResult = send(connectSocket, msg, (int)strlen(msg), 0);
							//// Check result of send function
							if (tempResult == SOCKET_ERROR)
							{
								printf("send failed with error: %d\n", WSAGetLastError());
								closesocket(connectSocket);
								WSACleanup();
								return 1;
							}
							//recieved = false;
							#pragma endregion

							#pragma region PRIJEM_ODGOVORA
							iResult = recv(connectSocket, dataBuffer, BUFFER_SIZE, 0);
							if (iResult > 0)
							{
								dataBuffer[iResult] = '\0'; // MANJE VECE TACNO
								printf("\nPRIJEM>> %s", dataBuffer);
								if (strcmp(dataBuffer, "MANJE") == 0)
								{
									intervalKraj = sendVal - 1;
									//interval->max = sendVal - 1;
								}
								else if (strcmp(dataBuffer, "VECE") == 0)
								{
									intervalPocetak = sendVal + 1;
									//interval->min = sendVal + 1;
								}
								else if (strcmp(dataBuffer, "TACNO") == 0)
								{
									// POBEDNIK = true;
									GAME_OVER = true;
								}
								else if (strcmp(dataBuffer, "GAMEOVER") == 0)
								{
									GAME_OVER = true;
								}
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
							//Sleep(1000);
							//recieved = true;
							#pragma endregion

							break;
						}
						// LINEAR FRONT
						case linearna_od_napred:
						{
							#pragma region SLANJE_POKUSAJA
							std::string key = ":2";
							int sendVal = LinearFront(intervalPocetak);
							std::string val = std::to_string(sendVal);
							val += key;

							char msg[BUFFER_SIZE];
							strcpy(msg, val.c_str());

							int tempResult = send(connectSocket, msg, (int)strlen(msg), 0);
							//// Check result of send function
							if (tempResult == SOCKET_ERROR)
							{
								printf("send failed with error: %d\n", WSAGetLastError());
								closesocket(connectSocket);
								WSACleanup();
								return 1;
							}
							#pragma endregion

							#pragma region PRIJEM_ODGOVORA
							iResult = recv(connectSocket, dataBuffer, BUFFER_SIZE, 0);
							if (iResult > 0)
							{
								dataBuffer[iResult] = '\0'; // VECE TACNO, zato sto ne moze biti manje jer krece od pocetka intervala redom
								printf("\nPRIJEM>> %s", dataBuffer);
								/*if (strcmp(dataBuffer, "MANJE") == 0)
								{
									interval->max = sendVal - 1;
								}*/
								if (strcmp(dataBuffer, "VECE") == 0)
								{
									intervalPocetak = sendVal + 1;
								}
								else if (strcmp(dataBuffer, "TACNO") == 0)
								{
									// POBEDNIK = true;
									GAME_OVER = true;
								}
								else if (strcmp(dataBuffer, "GAMEOVER") == 0)
								{
									GAME_OVER = true;
								}
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
							#pragma endregion

							break;
						}
						// LINEAR BACK
						case linearna_od_nazad:
						{
							#pragma region SLANJE_POKUSAJA
							std::string key = ":2";
							int sendVal = LinearBack(intervalKraj);
							std::string val = std::to_string(sendVal);
							val += key;

							char msg[BUFFER_SIZE];
							strcpy(msg, val.c_str());

							int tempResult = send(connectSocket, msg, (int)strlen(msg), 0);
							//// Check result of send function
							if (tempResult == SOCKET_ERROR)
							{
								printf("send failed with error: %d\n", WSAGetLastError());
								closesocket(connectSocket);
								WSACleanup();
								return 1;
							}
							#pragma endregion

							#pragma region PRIJEM_ODGOVORA
							iResult = recv(connectSocket, dataBuffer, BUFFER_SIZE, 0);
							if (iResult > 0)
							{
								dataBuffer[iResult] = '\0'; // MANJE TACNO, zato sto ne moze biti vece jer krece od kraja intervala redom
								printf("\nPRIJEM>> %s", dataBuffer);
								if (strcmp(dataBuffer, "MANJE") == 0)
								{
									intervalKraj = sendVal - 1;
								}
								/*if (strcmp(dataBuffer, "VECE") == 0)
								{
									interval->min = sendVal + 1;
								}*/
								else if (strcmp(dataBuffer, "TACNO") == 0)
								{
									GAME_OVER = true;
									// POBEDNIK = true;
								}
								else if (strcmp(dataBuffer, "GAMEOVER") == 0)
								{
									GAME_OVER = true;
								}
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
							#pragma endregion

							break;
						}
					}

					break;
				}
			}
		}
		auto t2 = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
		#pragma endregion
		

		printf("\n\n>> GAME OVER...");
		if (role == admin)
		{
			printf("\n\n\n>> RESULTS =\tP1: %d milisekundi.\n\t\tP2: %d milisekundi.\n\n", P1Vreme, P2Vreme);
			if (P1Vreme < P2Vreme)
			{
				printf(">> P1 WINNER!\n");
			}
			else
			{
				printf(">> P2 WINNER!\n");
			}
		}


		printf("\n\nPress any key to finish...");
		getchar();

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

int Binary(int intervalMin, int intervalMax)
{
	float retVal = intervalMin + ((intervalMax - intervalMin) / 2);
	retVal = floor(retVal);

	return (int)retVal;
}

int LinearFront(int intervalMin)
{
	return intervalMin;
}

int LinearBack(int intervalMax)
{
	return intervalMax;
}

#pragma endregion