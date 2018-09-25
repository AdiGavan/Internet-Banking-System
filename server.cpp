/* "Copyright [2018] Gavan Adrian-George, 324CA" */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <unordered_map>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <vector>

#define MAX_CLIENTS	25
#define BUFLEN 256

using namespace std;

// Clasa ce retine informatiile unui client.
class User {
	public:
		string nume;
		string prenume;
		string numar_card;
		string pin;
		string parola_secreta;
		double sold;
		int sock = -1;
		bool blocat = false;
		bool deblocare_inceputa = false;


		User() {};
		User(string nume, string prenume, string numar_card, string pin,
					string parola_secreta, double sold) {
			this->nume = nume;
			this->prenume = prenume;
			this->numar_card = numar_card;
			this->pin = pin;
			this->parola_secreta = parola_secreta;
			this->sold = sold;
		}

};

// Clasa utilizata pentru contorizarea de pinuri gresite pentru client.
typedef struct {
	string numar_card;
	int contor_blocare = 0;
} Incercari_card;

// Clasa utilizata pentru transfer.
typedef struct {
	string numar_card;
	double suma;
} Cerere_transfer;

// Functie pentru incarcarea datelor din fisier in memorie.
void incarcare_informatii(string fisier,int &nr_useri,
													unordered_map<string, User> &data_users) {

	ifstream fin(fisier);
	if (!fin.is_open()) {
		printf("-10 : Eroare la apelul de deschidere a fisierului users_data_file\n");
    	exit(4);
	}

	fin >> nr_useri;
	string nume, prenume, parola_secreta, numar_card, pin;
	double sold;

	for (int i = 0; i < nr_useri; i++) {
		fin >> nume;
		fin >> prenume;
		fin >> numar_card;
		fin >> pin;
		fin >> parola_secreta;
		fin >> sold;
		User aux(nume, prenume, numar_card, pin, parola_secreta, sold);
		data_users[numar_card] = aux;
	}
	fin.close();
}

int main(int argc, char *argv[]) {
	int sockfd, newsockfd, portno;
	struct sockaddr_in serv_addr, cli_addr;
	struct sockaddr_in server_addr_udp, client_addr_udp;
	int sockudp;

	socklen_t clilen;
	char buffer[BUFLEN];
	int n, i, nr_useri;;
	string bufer;
	string mesaj;
	// Map utilizat pentru retinerea datelor clientilor (cheie - numarul de card).
	unordered_map<string, User> data_users;
	// Map utilizat pentru retinerea numarului de pinuri gresite introduse de
	// un client pentru un card (cheie - socket-ul clientului).
	unordered_map<int, Incercari_card> verificare_login;
	// Map utilizat pentru a tine evidenta care socketi corespund unor carduri.
	unordered_map<int, string> sock_nrCard;
	// Map utilizat pentru a tine evidenta cererilor de transfer (cheie - socket).
	unordered_map<int, Cerere_transfer> cereri_transfer;
	// Vector in care se retin clientii activi (socketii conectati).
	vector<int> conexiuni_clienti;
	fd_set read_fds;	//multimea de citire folosita in select()
	fd_set tmp_fds;	//multime folosita temporar
	int fdmax;		//valoare maxima file descriptor din multimea read_fds

	if (argc < 3) {
		printf("-10 : Eroare la apelul executabilului(prea putine argumente)\n");
		return 0;
	} else if (argc > 3) {
		printf("-10 : Eroare la apelul executabilului(prea multe argumente)\n");
		return 0;
	}

	// Se goleste multimea de descriptori de citire (read_fds) si multimea tmp_fds
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("-10 : Eroare la apelul socket(TCP)\n");
		exit(2);
	}

	portno = atoi(argv[1]);
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;	// Se foloseste adresa IP a masinii.
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) {
		printf("-10 : Eroare la apelul bind(TCP)\n");
		exit(3);
	}

	listen(sockfd, MAX_CLIENTS);
	// Se adauga noul file descriptor (socketul pe care se asculta conexiuni) in
	// multimea read_fds.
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	// Mai sus s-a realizat conexiunea TCP. Acum se realizeaza conexiunea UDP.
	sockudp = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockudp < 0) {
			printf("-10 : Eroare la apelul socket(UDP)\n");
			exit(2);
    }

	memset((char *) &server_addr_udp, 0, sizeof(server_addr_udp));
	/* Se seteaza familia de adrese socket-ului. */
	server_addr_udp.sin_family = AF_INET;
	/* Se asculta pe toate adresele locale (ce am pe placa de retea). */
	server_addr_udp.sin_addr.s_addr = htonl(INADDR_ANY);
	/* Se seteaza portul socket-ului. */
	server_addr_udp.sin_port = htons((unsigned short) atoi(argv[1]));

	/* Legare proprietati de socket. */
	/* Parametrii: socket, adresa socket-ului meu, marimea socket-ului. */
	if (bind(sockudp, (struct sockaddr *) &server_addr_udp,
											sizeof(server_addr_udp)) < 0) {
		printf("-10 : Eroare la apelul bind(UDP)\n");
		exit(3);
	}

	// Se adauga socket-ul pentru UDP si STDIN in multimea descriptorilor.
	FD_SET(sockudp, &read_fds);
	FD_SET(0, &read_fds);
	if(sockudp > fdmax) {
		fdmax = sockudp;
	}
	// Se incarca informatiile.
	incarcare_informatii(argv[2], nr_useri, data_users);

	while (1) {
		tmp_fds = read_fds;
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) {
			printf("-10 : Eroare la apelul select\n");
			exit(5);
		}

		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd) {
					// A venit ceva pe socketul inactiv(cel cu listen) = o noua conexiune
					// Actiunea serverului: accept()
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
																											&clilen)) == -1) {
						printf("-10 : Eroare la apelul accept\n");
						exit(5);
					} else {
						// Se adauga noul socket intors de accept la multimea descriptorilor.
						FD_SET(newsockfd, &read_fds);
						conexiuni_clienti.push_back(newsockfd);
						if (newsockfd > fdmax) {
							fdmax = newsockfd;
						}
					}
					printf("Noua conexiune de la %s, port %d, socket_client %d\n",
					inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);

					// Cazul cand se citesc date de la STDIN.
        } else if (i == 0) {
					cin >> bufer;
					if (bufer != "quit") {
						printf("Comanda incorecta. Comanda valida: \"quit\"\n");
					} else {
						memset(buffer, 0, BUFLEN);
						strcpy(buffer, bufer.c_str());

						// Se trimite tuturor clientilor faptul ca o sa se inchida serverul.
						for (unsigned int j = 0; j < conexiuni_clienti.size(); j++) {
	           	n = send(conexiuni_clienti[j], buffer, BUFLEN, 0);
            	if (n < 0) {
              	cout << "-10 : Eroare la apel send" << endl;
								exit(6);
            	}
						}
						// Se inchid conexiunile.
						for (unsigned int j = 0; j < conexiuni_clienti.size(); j++) {
							close(conexiuni_clienti[j]);
							FD_CLR(conexiuni_clienti[j], &read_fds);
						}
						close(sockfd);
						close(sockudp);
						FD_CLR(sockfd, &read_fds);
						FD_CLR(sockudp, &read_fds);
						return 0;
					}

					// Cazul cand se primesc date pe UDP.
				} else if (i == sockudp) {
					socklen_t slen = sizeof(client_addr_udp);
					n = recvfrom(sockudp, buffer, BUFLEN, 0,
											(struct sockaddr *) &client_addr_udp, &slen);
					if (n < 0) {
						printf("-10 : Eroare la apelul recvfrom\n");
						exit(6);
					}
					string bufer(buffer);

					// Cazul cand se primeste "unlock".
					if (bufer.substr(0, 6) == "unlock") {
						string numar_card = bufer.substr(7, 6);
						auto exista_card = data_users.find(numar_card);

						// Cazul cand cardul nu exista.
						if (exista_card == data_users.end()) {
							n = sendto(i, "UNLOCK> -4 : Numar card inexistent", BUFLEN, 0,
																		(struct sockaddr *) &client_addr_udp, slen);
							if (n < 0) {
								cout << "-10 : Eroare la apel sendto" << endl;
								exit(6);
							}
							// Cazul cand cardul nu este blocat.
						} else if (!exista_card->second.blocat) {
							n = sendto(i, "UNLOCK> -6 : Operatie esuata", BUFLEN, 0,
																		(struct sockaddr *) &client_addr_udp, slen);
							if (n < 0) {
								cout << "-10 : Eroare la apel sendto" << endl;
								exit(6);
							}
							// Cazul cand cineva deja incearca sa deblocheze cardul.
						} else if (exista_card->second.deblocare_inceputa) {
							n = sendto(i, "UNLOCK> -7 : Deblocare esuata", BUFLEN, 0,
																		(struct sockaddr *) &client_addr_udp, slen);
							if (n < 0) {
								cout << "-10 : Eroare la apel sendto" << endl;
								exit(6);
							}
							// Cazul cand se incepe deblocarea si se cere parola.
						} else {
							exista_card->second.deblocare_inceputa = true;
							n = sendto(i, "UNLOCK> Trimite parola secreta", BUFLEN, 0,
																		(struct sockaddr *) &client_addr_udp, slen);
							if (n < 0) {
								cout << "-10 : Eroare la apel sendto" << endl;
								exit(6);
							}
						}
						// Cazul cand se primeste parola secreta (si numarul cardului).
					} else {
						string numar_card = bufer.substr(0,6);
						auto exista_card = data_users.find(numar_card);
						// In principiu in acest stadiu nu se poate intra in acest caz,
						// dar se verifica pentru orice eventualitate existenta cardului.
						if (exista_card == data_users.end()) {
							n = sendto(i, "UNLOCK> -4 : Numar card inexistent", BUFLEN, 0,
																		(struct sockaddr *)&client_addr_udp, slen);
							if (n < 0) {
								cout << "-10 : Eroare la apel sendto" << endl;
								exit(6);
							}
							// Cazul cand cardul exista si parola este corecta.
						} else if(exista_card->second.parola_secreta ==
																					bufer.substr(7, bufer.length() - 7)){

							exista_card->second.blocat = false;
							exista_card->second.deblocare_inceputa = false;
							n = sendto(i, "UNLOCK> Card deblocat", BUFLEN, 0,
																		(struct sockaddr *)&client_addr_udp, slen);
							if (n < 0) {
								cout << "-10 : Eroare la apel sendto" << endl;
								exit(6);
							}
							// Cazul cand parola este incorecta.
						} else {
							exista_card->second.deblocare_inceputa = false;
							n = sendto(i, "UNLOCK> -7 : Deblocare esuata", BUFLEN, 0,
																		(struct sockaddr *)&client_addr_udp, slen);
							if (n < 0) {
								cout << "-10 : Eroare la apel sendto" << endl;
								exit(6);
							}
						}
					}

					// Cazul cand se primesc date pe una din conexiunile TCP prin care
					// serverul comunica cu un anume client.
				} else {
					memset(buffer, 0, BUFLEN);
					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
						if (n == 0) {	// Conexiunea s-a inchis.
							printf("Clientul de pe socket-ul %d a iesit\n", i);
						} else {
							printf("-10 : Eroare la apelul recv\n");
							exit(6);
						}
						// Se elimina socket-ul din multimea socketilor "activi".
						conexiuni_clienti.erase(remove(conexiuni_clienti.begin(),
													conexiuni_clienti.end(), i), conexiuni_clienti.end());
						close(i);
						FD_CLR(i, &read_fds);

						// Cazul cand recv intoarce > 0.
					} else {
						string bufer(buffer);

						// Cazul cand se primeste comanda "login".
						if (bufer.substr(0,5) == "login") {
							string numar_card = bufer.substr(6,6);
							string pin = bufer.substr(13,4);

							// Daca se primeste alta comanda de login de pe acelasi client
							// dar pentru alt card se reseteaza contorul de blocare.
							auto resetare_contor_blocare = verificare_login.find(i);
							if (resetare_contor_blocare != verificare_login.end()) {
								if (resetare_contor_blocare->second.numar_card != numar_card){
									verificare_login.erase(i);
								}
							}

							auto exista_card = data_users.find(numar_card);
							// Cazul cand cardul nu exista.
							if (exista_card == data_users.end()) {
								n = send(i, "IBANK> -4 : Numar card inexistent", BUFLEN, 0);
								if (n < 0) {
                  cout << "-10 : Eroare la apel send" << endl;
									exit(6);
                }
								//Cardul exista dar e deja o sesiune pe el.
							} else if(exista_card->second.sock != -1) {
								n = send(i, "IBANK> -2 : Sesiune deja deschisa", BUFLEN, 0);
								if (n < 0) {
									cout << "-10 : Eroare la apel send" << endl;
									exit(6);
								}
								 // Se verifica daca cardul este blocat.
							} else if(exista_card->second.blocat) {
								n = send(i, "IBANK> -5 : Card blocat", BUFLEN, 0);
								if (n < 0) {
									cout << "-10 : Eroare la apel send" << endl;
									exit(6);
								}
								// Se verifica pinul.
							} else {
								// Cazul cand pinul este gresit.
								if (pin != exista_card->second.pin) {
									auto incercari_client = verificare_login.find(i);

									// Cand e prima oara cand se introduce pinul gresit pentru
									// acest card de pe acest client.
									if (incercari_client == verificare_login.end() ||
														incercari_client->second.numar_card != numar_card) {
										Incercari_card aux;
										aux.numar_card = numar_card;
										aux.contor_blocare = 1;
										verificare_login[i] = aux;

										// Cazul cand deja mai fusese introdus pinul gresit.
									} else {
										incercari_client->second.contor_blocare++;
										if(incercari_client->second.contor_blocare >= 3) {
											exista_card->second.blocat = true;
											verificare_login.erase(i);
											n = send(i, "IBANK> -5 : Card blocat", BUFLEN, 0);
											if (n < 0) {
												cout << "-10 : Eroare la apel send" << endl;
												exit(6);
											}
											continue;
										}
									}
									n = send(i, "IBANK> -3 : Pin gresit", BUFLEN, 0);
									if (n < 0) {
										cout << "-10 : Eroare la apel send" << endl;
										exit(6);
									}
									// Cazul cand pinul este corect si cardul nu e blocat.
								} else {
									auto incercari_client = verificare_login.find(i);
									if (incercari_client != verificare_login.end()) {
										verificare_login.erase(i);
									}
									exista_card->second.sock = i;
									sock_nrCard[i] = numar_card;

									mesaj = "IBANK> Welcome ";
									mesaj += exista_card->second.nume + " "
																				+ exista_card->second.prenume;

									memset(buffer, 0, BUFLEN);
									strcpy(buffer, mesaj.c_str());
									n = send(i, buffer, BUFLEN, 0);
									if (n < 0) {
										cout << "-10 : Eroare la apel send" << endl;
										exit(6);
									}
								}
							}

							// Cazul cand se primeste comanda "logout".
						} else if (bufer.substr(0,6) == "logout") {
							auto client_card = sock_nrCard.find(i);
							if (client_card != sock_nrCard.end()) {
								data_users[client_card->second].sock = -1;
								sock_nrCard.erase(i);
								n = send(i, "IBANK> Clientul a fost deconectat", BUFLEN, 0);
								if (n < 0) {
									cout << "-10 : Eroare la apel send" << endl;
									exit(6);
								}
								// Cazul asta nu ar trebui sa se intample niciodata, pentru
								// ca se verifica sa fie logat in client, dar pentru orice
								// eventualitate.
							} else {
								cout << "-10 : Eroare la apel logout(nu s-a gasit clientul pentru delogare)" << endl;
							}

							// Cazul cand se primeste comanda "quit".
						} else if (bufer.substr(0,4) == "quit") {
							auto incercari_client = verificare_login.find(i);
							if (incercari_client != verificare_login.end()) {
								verificare_login.erase(i);
							}
							auto client_card = sock_nrCard.find(i);
							if (client_card != sock_nrCard.end()) {
								data_users[client_card->second].sock = -1;
								sock_nrCard.erase(i);
							}
							// Se elimina socketul din socketii activi.
							conexiuni_clienti.erase(remove(conexiuni_clienti.begin(),
													conexiuni_clienti.end(), i), conexiuni_clienti.end());
							printf("Clientul de pe socket-ul %d a iesit\n", i);
							close(i);
							FD_CLR(i, &read_fds);

							// Cazul cand se primeste comanda "listsold".
						} else if (bufer.substr(0,8) == "listsold") {
							auto client_card = sock_nrCard.find(i);
							// Se verifica ca clientul sa fie conectat la un cont.
							// In principiu ar trebui ca mereu sa fie conectat (verificre
							// din client).
							if (client_card != sock_nrCard.end()) {
								double sold = data_users[client_card->second].sold;
								mesaj = "IBANK> ";
								stringstream stream;
								stream << fixed << setprecision(2) << sold;
								mesaj += stream.str();

								memset(buffer, 0, BUFLEN);
								strcpy(buffer, mesaj.c_str());
								n = send(i, buffer, BUFLEN, 0);
								if (n < 0) {
									cout << "-10 : Eroare la apel send" << endl;
									exit(6);
								}
							}

							// Cazul cand se primeste comanda "transfer".
						} else if (bufer.substr(0,8) == "transfer") {
							string numar_card_transfer = bufer.substr(9,6);

							auto exista_card = data_users.find(numar_card_transfer);
							// Cazul cand cardul nu exista.
							if (exista_card == data_users.end()) {
								n = send(i, "IBANK> -4 : Numar card inexistent", BUFLEN, 0);
								if (n < 0) {
                	cout << "-10 : Eroare la apel send" << endl;
									exit(6);
                }
								// Cazul cand cardul exista.
							} else {
								double suma = stod(bufer.substr(16, bufer.length() - 16));
								stringstream stream;
								stream << fixed << setprecision(2) << suma;

								auto client_card = sock_nrCard.find(i);
								if (client_card != sock_nrCard.end()) {
									// Cazul cand nu sunt destule foduri in cont.
									if (suma > data_users[client_card->second].sold) {
										n = send(i, "IBANK> -8 : Fonduri insuficiente", BUFLEN, 0);
										if (n < 0) {
											cout << "-10 : Eroare la apel send" << endl;
											exit(6);
										}
										// Cazul cand cardul exita si sunt destule fonduri.
									} else {
										Cerere_transfer aux;
										aux.suma = suma;
										aux.numar_card = numar_card_transfer;
										cereri_transfer[i] = aux;

										mesaj = "IBANK> Transfer ";
										mesaj += stream.str() + " catre " + exista_card->second.nume
																+ " " + exista_card->second.prenume + "? [y/n]";
										memset(buffer, 0, BUFLEN);
										strcpy(buffer, mesaj.c_str());
										n = send(i, buffer, BUFLEN, 0);
										if (n < 0) {
											cout << "-10 : Eroare la apel send" << endl;
											exit(6);
										}
									}
								}
							}

							// Cazul cand se primeste raspunsul pentru confirmarea unui
							// transfer.
						} else {
							auto cerere_tr = cereri_transfer.find(i);
							if (cerere_tr != cereri_transfer.end()) {
								// Daca raspunsul este pozitiv.
								// Conform cerintei se ia in considerare doar prima litera.
								if (bufer.substr(0,1) == "y") {

									data_users[cerere_tr->second.numar_card].sold += cerere_tr->second.suma;
									data_users[sock_nrCard.at(i)].sold -= cerere_tr->second.suma;
									cereri_transfer.erase(i);
									n = send(i, "IBANK> Transfer realizat cu succes", BUFLEN, 0);
									if (n < 0) {
										cout << "-10 : Eroare la apel send" << endl;
										exit(6);
									}
									// Daca raspunsul este orice altceva.
								} else {
									n = send(i, "IBANK> -9 : Operatie anulata", BUFLEN, 0);
									if (n < 0) {
										cout << "-10 : Eroare la apel send" << endl;
										exit(6);
									}
								}
							}
						}
          }
				}
			}
		}
  }
	close(sockfd);
	return 0;
}
