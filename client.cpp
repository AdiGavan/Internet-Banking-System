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
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#define BUFLEN 256

using namespace std;

int main(int argc, char *argv[]) {
  int sockfd, sockudp, n, rc;
  struct sockaddr_in serv_addr, server_addr_udp;
  string nume_fisier = "client-";
  char buffer[BUFLEN];
  string bufer, backup_nr_card;
  bool logged = false;
  bool transfer = false;
  bool unlock = false;

  if (argc < 3) {
      printf("-10 : Eroare la apelul executabilului(prea putine argumente)\n");
      return 0;
  } else if (argc > 3) {
      printf("-10 : Eroare la apelul executabilului(prea multe argumente)\n");
      return 0;
  }
  // Se creeaza numele fisierului de log.
  nume_fisier += to_string(getpid());
  nume_fisier += ".log";
  printf("ID-ul acestui client este: %d\n", getpid());
  ofstream fout(nume_fisier);

	if (!fout.is_open()) {
		printf("-10 : Eroare la apelul de deschidere a fisierului de log\n");
        exit(4);
	}
  // Conexiunea TCP.
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    printf("-10 : Eroare la apelul socket(TCP)\n");
    cout << endl;
    fout << "-10 : Eroare la apelul socket(TCP)" << endl;
    fout << endl;
    exit(2);
	}

  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(atoi(argv[2]));
  inet_aton(argv[1], &serv_addr.sin_addr);

  if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) {
    printf("-10 : Eroare la apelul connect\n");
    cout << endl;
    fout << "-10 : Eroare la apelul connect" << endl;
    fout << endl;
	exit(3);
  }

  fd_set read_fds;
  fd_set tmp_fds;
  FD_ZERO(&read_fds);
  FD_ZERO(&tmp_fds);
  FD_SET(sockfd, &read_fds);
  FD_SET(0, &read_fds);
  int fdmax = sockfd;

  // Se creeaza si conexiunea UDP
  sockudp = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockudp < 0) {
    printf("-10 : Eroare la apelul socket(UDP)\n");
    cout << endl;
    fout << "-10 : Eroare la apelul socket(UDP)" << endl;
    fout << endl;
    exit(2);
  }

  memset((char *) &server_addr_udp, 0, sizeof(server_addr_udp));
  server_addr_udp.sin_family = AF_INET;
  rc = inet_aton(argv[1], &server_addr_udp.sin_addr);
	if(rc < 0) {
		printf("-10 : Eroare la apelul inet_aton\n");
    cout << endl;
    fout << "-10 : Eroare la apelul inet_aton" << endl;
    fout << endl;
		exit(4);
	}
  server_addr_udp.sin_port = htons((unsigned short)atoi(argv[2]));

  FD_SET(sockudp, &read_fds);
	if(sockudp > fdmax) {
		fdmax = sockudp;
	}

  while(1){
    tmp_fds = read_fds;
    select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
    int i;
    for(i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &tmp_fds)) {
        // Cazul cand se citeste de la tastatura.
        if(i == 0) {
          getline(cin, bufer);
          fout << bufer << endl;

          // Cazul cand se citeste "login".
          if (bufer.substr(0,5) == "login") {
            // Cazul cand deja este logat.
            if (logged) {
              fout << "-2 : Sesiune deja deschisa" << endl;
              fout << endl;
              cout << "-2 : Sesiune deja deschisa" << endl;
              cout << endl;
              // Cazul cand nu este deja logat.
            } else {
              // Daca nu este deja logat, se retine numarul cardului
              // pentru ultimul login (pentru unlock).
              backup_nr_card = bufer.substr(6,6);
              memset(buffer, 0, BUFLEN);
              strcpy(buffer, bufer.c_str());
              n = send(sockfd, buffer, BUFLEN, 0);
              if (n < 0) {
                fout << "-10 : Eroare la apel send" << endl;
                fout << endl;
                cout << "-10 : Eroare la apel send" << endl;
                cout << endl;
                fout.close();
                exit(5);
              }
            }

            // Cazul cand se primeste comanda "logout".
          } else if (bufer.substr(0,6) == "logout") {
            // Cazul cand nu este logat.
            if (!logged) {
              fout << "-1 : Clientul nu este autentificat" << endl;
              fout << endl;
              cout << "-1 : Clientul nu este autentificat" << endl;
              cout << endl;
              // Cazul cand este logat.
            } else {
              memset(buffer, 0, BUFLEN);
              strcpy(buffer, bufer.c_str());
              logged = false; // Se retine ca acest client nu mai e logat.
              n = send(sockfd, buffer, BUFLEN, 0);
              if (n < 0) {
                fout << "-10 : Eroare la apel send" << endl;
                fout << endl;
                cout << "-10 : Eroare la apel send" << endl;
                cout << endl;
                fout.close();
                exit(5);
              }
            }

            // Cazul cand se primeste comanda "quit".
          } else if (bufer.substr(0,4) == "quit") {
            logged = false;
            fout << endl;
            cout << endl;
            memset(buffer, 0, BUFLEN);
            strcpy(buffer, bufer.c_str());
            n = send(sockfd, buffer, BUFLEN, 0);
            if (n < 0) {
              fout << "-10 : Eroare la apel send" << endl;
              fout << endl;
              cout << "-10 : Eroare la apel send" << endl;
              cout << endl;
              fout.close();
              exit(5);
            }
            close(sockudp);
            close(sockfd);
            fout.close();
            return 0;

            // Cazul cand se primeste comanda "listsold".
          } else if (bufer.substr(0,8) == "listsold") {
            // Cazul cand clientul nu este logat.
            if (!logged) {
              fout << "-1 : Clientul nu este autentificat" << endl;
              fout << endl;
              cout << "-1 : Clientul nu este autentificat" << endl;
              cout << endl;
              // Cazul cand clientul este logat.
            } else {
              memset(buffer, 0, BUFLEN);
              strcpy(buffer, bufer.c_str());
              n = send(sockfd, buffer, BUFLEN, 0);
              if (n < 0) {
                fout << "-10 : Eroare la apel send" << endl;
                fout << endl;
                cout << "-10 : Eroare la apel send" << endl;
                cout << endl;
                fout.close();
                exit(5);
              }
            }

            // Cazul cand se primeste comanda "transfer".
          } else if (bufer.substr(0,8) == "transfer") {
            // Cazul cand clientul nu este logat.
            if (!logged) {
              fout << "-1 : Clientul nu este autentificat" << endl;
              fout << endl;
              cout << "-1 : Clientul nu este autentificat" << endl;
              cout << endl;
              // Cazul cand clientul este logat.
            } else {
              transfer = true;
              memset(buffer, 0, BUFLEN);
              strcpy(buffer, bufer.c_str());
              n = send(sockfd, buffer, BUFLEN, 0);
              if (n < 0) {
                fout << "-10 : Eroare la apel send" << endl;
                fout << endl;
                cout << "-10 : Eroare la apel send" << endl;
                cout << endl;
                fout.close();
                exit(5);
              }
            }

            // Cazul cand se primeste comanda "unlock".
          } else if (bufer.substr(0,6) == "unlock") {
            unlock = true; // Se retine ca clientul a apelat comanda unlock.
            memset(buffer, 0, BUFLEN);
            bufer += " " + backup_nr_card;
            strcpy(buffer, bufer.c_str());
            socklen_t slen = sizeof(server_addr_udp);
            // Se trimite comanda de unlock + numarul cardului (pe UDP).
            n = sendto(sockudp, buffer, BUFLEN, 0,
                                    (struct sockaddr *) &server_addr_udp, slen);
            if (n < 0) {
              fout << "-10 : Eroare la apel sendto" << endl;
              fout << endl;
              cout << "-10 : Eroare la apel sendto" << endl;
              cout << endl;
              fout.close();
              exit(5);
            }
            // Se primeste raspunsul (pe UDP).
            memset(buffer, 0, BUFLEN);
            n = recvfrom(sockudp, buffer, BUFLEN, 0,
                                    (struct sockaddr *) &server_addr_udp, &slen);
            if (n < 0) {
              fout << "-10 : Eroare la apel recvfrom" << endl;;
              fout << endl;
              cout << "-10 : Eroare la apel recvfrom" << endl;
              cout << endl;
              fout.close();
              exit(5);
            }

            string bufer(buffer);
            // Cazul cand se cere trimiterea parolei.
            if (bufer.substr(8, 7) == "Trimite") {
              fout << bufer << endl;
              cout << bufer << endl;
              // Cazul cand ceva nu este in regula si primeste direct o eroare.
            } else {
              unlock = false; // Primind eroare, nu mai e in procesul de unlock.
              fout << bufer << endl;
              fout << endl;
              cout << bufer << endl;
              cout << endl;
            }

            // Cazul cand se primeste parola pentru unlock.
          } else if (unlock) {
            string mesaj = backup_nr_card;
            mesaj += " " + bufer;
            // Se trimit numarul cardului si parola (pe UDP).
            memset(buffer, 0, BUFLEN);
            strcpy(buffer, mesaj.c_str());
            socklen_t slen = sizeof(server_addr_udp);
            n = sendto(sockudp, buffer, BUFLEN, 0,
                              (struct sockaddr *) &server_addr_udp, slen);
            if (n < 0) {
              fout << "-10 : Eroare la apel sendto" << endl;
              fout << endl;
              cout << "-10 : Eroare la apel sendto" << endl;
              cout << endl;
              fout.close();
              exit(5);
            }
            // Se primeste raspunsul (pe UDP).
            n = recvfrom(sockudp, buffer, BUFLEN, 0,
                                  (struct sockaddr *) &server_addr_udp, &slen);
            if (n < 0) {
              fout << "-10 : Eroare la apel recvfrom" << endl;
              fout << endl;
              cout << "-10 : Eroare la apel recvfrom" << endl;
              cout << endl;
              fout.close();
              exit(5);
            }
            // Indiferent de raspuns, clientul nu o sa mai fie in procesul de
            // unlock pentru ca fie a reusit, fie a esuat.
            string bufer(buffer);
            unlock = false;
            fout << bufer << endl;
            fout << endl;
            cout << bufer << endl;
            cout << endl;

            // Cazul cand trebuie sa se trimita confirmarea pentru "transfer".
          } else if (transfer) {
            memset(buffer, 0, BUFLEN);
            strcpy(buffer, bufer.c_str());
            n = send(sockfd, buffer, BUFLEN, 0);
            if (n < 0) {
              fout << "-10 : Eroare la apel send" << endl;
              fout << endl;
              cout << "-10 : Eroare la apel send" << endl;
              cout << endl;
              fout.close();
              exit(5);
            }
          }

          // Cazul cand se primeste mesaj de la server.
        } else {
          recv(i, buffer, sizeof(buffer), BUFLEN);
          string bufer(buffer);
          // Cazul cand serverul a trimis comanda "quit".
          if (bufer == "quit") {
            cout << "Serverul se va inchide. Conexiune terminata." << endl;
            cout << endl;
            fout.close();
            return 0;
            // Cazul cand primim un raspuns pentru "transfer".
          } else if (bufer.substr(7, 8) == "Transfer") {
            // Daca transferul a fost realizat cu succes.
            if (bufer.substr(16, 8) == "realizat") {
              transfer = false;
              fout << bufer << endl;
              fout << endl;
              cout << bufer << endl;
              cout << endl;
              // Cand se primeste cererea de confirmare.
            } else {
              fout << bufer << endl;
              cout << bufer << endl;
            }

            // Cazul cand se primesc alte raspunsuri.
          } else {
            // Cazurile cand transferul esueaza => se retine ca acest client
            // nu mai este in procesul de transfer.
            if (bufer.substr(7, 2) == "-8" || bufer.substr(7, 2) == "-9") {
              transfer = false;
            }

            fout << bufer << endl;
            fout << endl;
            cout << bufer << endl;
            cout << endl;

            // Cand se primeste faptul ca clientul a fost logat, se retine acest
            // fapt.
            if (bufer.substr(0, 14) == "IBANK> Welcome") {
              logged = true;
            }
          }
        }
      }
    }
  }

  fout.close();
  return 0;
}
