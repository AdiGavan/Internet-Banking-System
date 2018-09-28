"Copyright [2018] Gavan Adrian-George, 324CA" 

Nume, prenume: Gavan, Adrian-George

Grupa, seria: 324CA

Tema 2 PC - Sistem monetar de tip Internet Banking

Compilare si rulare:
====================

- Tema a fost realizata in C++ (se folosesc si cateva comenzi din C, cum ar fi "printf").
- Fisierul Makefile contine regulile de build si clean conform enuntului temei.
- Executabilele se ruleaza conform cerintei, iar ip-ul dat ca parametru executabilelor client este
127.0.0.1 (hostul local).

Prezentarea implementarii:
==========================

A. client.cpp
=============

- Clientul contine doar functia main, nu mai sunt alte functii auxiliare utilizate.
- Se verifica daca numarul de argumente este corect, iar apoi se creeaza fisierul de log.
- Se deschid conexiunile TCP si UDP.
- Se adauga si stdin si socket-ul UDP in multimea de descriptori.
- Acum avem 2 cazuri mari: cand se primesc comenzi de la stdin si cand se primeste raspuns de
la server.

a. Primesc comenzi de la stdin
1. Se primeste comanda "login".
- Se verifica daca exista deja o sesiune deschisa (daca este deschisa se intoarce direct eroare).
- Daca nu exista o sesiune deschisa, se retine numarul carudului (trebuie pentru unlock), iar
apoi se trimite mesajul catre server.
2. Se primeste comanda "logout".
- Se verifica daca clientul este autentificat (daca nu este se intoarce direct eroare).
- Daca este logat, atunci se reseteaza contorul care retine daca clientul este sau nu logat si
se trimite mesajul la server.
3. Se primeste comanda "quit".
- Se reseteaza "logged".
- Se trimite mesajul la server, se inchid socketii si fisierul si se termina executia clientului.
4. Se primeste comanda "listsold".
- Se verifica daca clientul este autentificat (daca nu este se intoarce direct eroare).
- Daca este autentificat se trimite mesajul catre server.
5. Se primeste comanda "transfer".
- Se verifica daca clientul este autentificat (daca nu este se intoarce direct eroare).
- Se retine faptul ca clientul este in procesul de transfer si se trimite mesajul catre server.
6. Se primeste comanda "unlock".
- Se retine faptul ca clientul este in procesul de unlock.
- Se formeaza mesajul cu "unlock <numar card>" si se trimite prin UDP catre server.
- Se primeste raspunsul de la server tot prin UDP.
- Daca a primit cerere de confirmare se afiseaza si se scrie in fisier fara rand liber.
- Daca a primit altceva ca si raspuns inseamna ca este o eroare, se reseteaza variabila "unlock"
si se scrie mesajul in fisier si la consola cu randuri libere.
7. Cazul cand sunt in procesul de "unlock" si primesc parola secreta de la stdin.
- Se formeaza mesajul "<numar card> parola".
- Se trimite mesajul prin UDP.
- Se primeste mesajul de la server prin UDP si indiferent de raspuns se afiseaza la consola si se
scrie in fisier cu rand liber. De asemenea, se reseteaza si variabila pentru unlock.
8. Cazul cand sunt in procesul de "transfer" si primesc raspunsul pentru confirmare de la stdin.
- Se trimite direct raspunsul catre server.

b. Primesc raspunsuri de la server
- Daca se primeste "quit", inseamna ca serverul se inchide, deci se inchide fisierul si se termina
executia.
- Daca transferul a fost realizat cu succes, se reseteaza variabila pentru transfer si se scrie in
fisier cu rand liber. Altfel, daca comanda incepe cu "Transfer", dar nu urmeaza cuvantul "realizat"
inseamna ca s-a primit o cerere de confirmare, deci se scrie in fisier fara spatiu.
- Daca se primesc alte raspunsuri, se verifica daca se primeste o eroare de tipul "-8" sau "-9",
pentru a stii daca trebuie resetata variabila transfer.
- Se scriu raspunsurile in fisier si la consola.
- Daca mesajul este unu de bun venit, inseamna ca trebuie sa se retina ca clientul este logat.

B. server.cpp
=============

- In server, pe langa functia main mai avem definite o functie si niste structuri auxiliare:
a. User - retine datele unul client + socket-ul clientului care este conectat la card,
          daca cardul este blocat si daca cineva a inceput procesul de deblocare;
b. Incercari_card - folosit pentru a retine cate pinuri au fost introduse gresit pentru un card.
c. Cerere_transfer - se retine suma si cardul catre care trebuie facut transferul.
d. functia "incarcare_informatii" - incarca datele din fisier intr-un unordered_map numit
data_users.

- In main sunt declarate mai multe variabile, dar voi explica utilitatea unora destul de importante.
a. unordered_map<string, User> data_users: este map-ul in care se retin toate datele despre clienti.
Cheia va fi numarul unui card.
b. unordered_map<int, Incercari_card> verificare_login: map-ul in care se va retine pentru fiecare
client cardul si numarul de incercari gresite pentru cardul respectiv in incercarea de a se loga.
Cheia este socket-ul clientului.
c. unordered_map<int, string> sock_nrCard: map-ul in care se retin asocierile socket-numar-card
pentru fiecare client conectat. Cheia este socket-ul.
d. unordered_map<int, Cerere_transfer> cereri_transfer: Map utilizat pentru a se stii ce cereri de
transfer sunt in progres. Cheia este socket-ul, iar pentru fiecare socket se retine contul catre
care trebuie sa trimita banii si suma corespunzatoare.
e. vector<int> conexiuni_clienti: se retin socketii corespunzatori clientilor inca conectati
la server;

- Se verifica ca numarul de parametrii sa fie corect.
- Se realizeaza conexiunile TCP si UDP.
- Cand se incepe multiplexarea din loop-ul infinit vom avea mai multe cazuri:

a. Cazul cand vine ceva pe socket-ul cu listen.
- Se face accept si se adauca noul socket. De asemenea, se adauga si in vectorul "conexiuni_clienti".

b. Cazul cand se citesc date de la stdin.
- Daca se primeste comanda "quit", se trimite un mesaj catre toti clientii care inca sunt conectati
spunand ca serverul se va inchide, apoi sunt eliminate toate socket-urile, se inchid si se termina
executia serverului.

c. Cazul cand se primesc date pe UDP.
1. Se primeste comanda "unlock" pe UDP.
- Se verifica daca cardul exista sau nu (daca nu exista se trimite eroare pe socketul UDP).
- Se verifica daca cardul e blocat sau nu (daca nu e blocat se trimite eroare pe socketul UDP).
- Se verifica daca cineva incearca sa deblocheze cardul (daca deja incearca, se trimite eroare pe
socketul UDP).
- Daca totul este ok, se seteaza in data_users faptul ca pentru acest card a inceput o deblocare si
se trimite mesajul de cerere parola secreta prin UDP catre client.
- Pentru toate verificarile s-a folosit doar "data_users".

2. Cazul cand se primesc numarul cardului si parola secreta.
- Se verifica daca exista cardul (mereu exista daca s-a ajuns pana aici, dar pentru orice
eventualitate).
- Daca exista cardul si parola este corecta, se reseteaza variabilele "blocat" si "deblocare_inceputa"
cu ajutorul map-ului data_users.
- Se trimite mesajul de deblocare prin UDP catre client.
- Daca exista cardul si parola este incorecta, se reseteaza doar "deblocare_inceputa" si se trimite
eroare catre client.

d. Cazul cand se primesc comenzi prin TCP.
- Se verifica daca conexiunea cu un anume client s-a inchis. Daca s-a inchis, se elimina socketul din
multimea de descriptori si din "conexiuni_clienti".
- Daca se primeste o comanda:
1. Se primeste comanda "login".
- Daca se primeste comanda de login de pe acelasi client, dar pentru alt card, se reseteaza contorul
pentru pin gresit.
- Se verifica daca cardul exista, daca exista deja o sesiune deschisa pe acel card sau daca este
blocat. Daca unul dintre aceste cazuri este adevarat, se intoarce eroarea corespunzatoare.
- Daca totul este ok dar pinul este gresit, se verifica daca a mai fost gresit pinul de acest client
pentru acest card sau nu. Daca nu a mai fost gresit se adauga in verificare_login o noua intrare.
- Daca a mai fost gresit, se cauta valoarea dupa cheie (socket) si se incrementeaza numarul de greseli.
- Daca numarul de greseli a ajuns la 3, se blocheaza cardul si se sterge din verificare_login valoarea
de la acest socket, pentru ca daca cardul s-a blocat, dupa ce se deblocheaza trebuie inceputa
contorizarea de la 0;
- Daca pinul este introdus corect, se sterge valoarea de la cheia respectiva din verificare_login,
se seteaza socket-ul in data_users pentru a se stii ca cineva este conectat la acest card, se adauga
in sock_nrCard cardul la cheia socket pentru a stii la ce card este conectat clientul si se trimite
mesajul de bunvenit.

2. Se primeste comanda "logout".
- Daca cardul se gaseste, se reseteaza valoarea sock din data_users pentru cardul respectiv, se sterge
perechea din sock_nrCard si se trimite mesajul corespunzator clientului.

3. Se primeste comanda "quit".
- Se sterge perechea corespunzatoare din verificare_login (daca exista).
- Se reseteaza sock din data_users pentru cardul respectiv.
- Se sterge perechea din sock_nrCard.
- Se elimina socketul din conexiuni_clienti.
- Se inchide socketul si se elimina din multimea descriptorilor.

4. Se primeste comanda "listsold".
- Se retine sold-ul intr-o variabila (este luat din data_users).
- Se formeaza mesajul (se transforma double-ul cu 2 zecimale in string si se concateneaza).
- Se trimite mesajul la client.

5. Se primeste comanda "transfer".
- Se verifica daca exista cardul si daca exista destui bani in cont. Daca ceva nu este ok, se
trimite codul de eroare corespunzator catre client.
- Daca totul este ok, se adauga in cereri transfer perechea socket-<numar card, suma ce trebuie
transferata> pentru ca serverul poate primii si alte comenzi si se trimite mesajul de cerere
confirmare catre client.

6. Se primeste raspunsul pentru confirmarea unui transfer.
- Se verifica daca exista cerere de transfer.
- Se verifica daca prima litera este "y".
- Daca este "y", atunci (folosindu-ne de data_users si cereri_transfer) modificam sold-urile
cardului corespunzator clientului (scadem suma) si cardului catre care se realizeaza transferul
(se aduna suma).
- Se elimina cerearea de transfer din map.
- Se trimite mesajul de confirmare al transferului catre client.
- Daca nu este "y", atunci se trimite codul de eroare -9 catre client.

Observatii:
===========
- La login se retine numarul cardului doar daca clientul nu este deja conectat. Daca este conectat
nu se mai retine cardul cand se apeleaza login (pentru unlock).
- Contorul pentru blocarea unui card nu se reseteaza daca un alt client introduce pinul gresit
pentru acelasi card.
- Contorul se reseteaza (pentru un client) doar daca introduc o alta comanda de login pentru alt
card (daca dau unlock sau listsold sau altceva nu se reseteaza). Se reseteaza si in caz de logare
cu succes, evident.
