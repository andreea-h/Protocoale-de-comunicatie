Horovei Andreea-Georgiana
325CC

Tema #2 PC - Aplicatie client-server TCP su UDP pentru gestionarea mesajelor


->RULAREA TEMEI
	Makefile-ul are optiunile 'build', 'run_server' si 'run_subscriber'. 
Regula run_server ruleaza cu argmentul dat la rulare in felul urmator:
	make run_subscriber id_client="<client_id>"

	Regulile run_server si build nu trebuie rulare cu argumente; argumentele 
sunt scrise in fisierul Makefile (port-8080 si ip-127.0.0.1).



	Incep prin a preciza ca, din verificarile pe care le-am facut cu ajutorul 
fisierului 'udp_client.py', aplicatia functioneaza ok in ceea ce priveste 
trimiterea de catre server a mesajelor catre clientii TCP in situatia in care 
acestia sunt conectati. Am facut verificari atat pentru rate mici ale mesajelor
(valoarea 2000 pentru delay) cat si pentru viteze mari (ruland scriptul pus la 
dispozitie fara delay si cu optiunea --mode=random). Nu am observat mesaje
eronate sau pachete lipsa in nicio situatie.

->Structuri pentru reprezentarea datelor/Functii auxiliare folosite in prelucrarea 
mesajelor

	Pentru gestionarea si transmiterea mesajelor intre server, clientii UDP si 
cei TCP am definit structuri pentru reprezentarea datelor astfel incat acestea 
sa poata fi accesate mai usor. Pentru a reprezenta un mesaj UPD primit de catre 
server am defint structura 'publisher_message'. Acesta are drept campuri cele 
trei componente prezente in Tabel 1 din enunt si a fost definita pentru a 
putea accesa fiecare componenta din mesajul trimis de clientul UDP,
topic_name, data_type si message fiind ulterior convertite in formatul prezentat
in Tabelul 2 din enunt astfel incat sa fie incluse intr-o variabila de tipul 
'subscriber_message', aceasta din urma ingloband informatiile care trebuie 
afisate clientului la primirea notificarii. 
	
	Dupa primirea mesajului de la clientiul UDP, am copiat intr-o variabila
de tipul 'publisher_message' continutul din bufferul de receptie (folosind memmove)
si am definit functia 'extract_paylod' care primeste ca argumente 2 pointeri, 
unul la o variabila de tipul publisher_message (am pasat pointe la variabila in care
tocmai am copiat continutul bufferului) si unul la o variabila de tipul
'subscriber_message' (pentru aceasta am pasat pointer la variabila in care se vor 
retine campurile informatiei care trebuie afisate in client la primirea unei 
notificari de la un publisher la care acesta este abonat). 
	
	Functia 'extract_payload' va prelua din pub_message acei octeti care reprezinta 
payload-ul si va face conversia dupa formatul prezentat in Tabel 2, in functie de 
tipul de date, punand la adresa corespunzatoare din sub_message payload-ul in forma 
in care trebuie afisat pentru client; functia va intoarce numarul de octeti necesari 
pentru reprezentarea mesajului. Numarul de octeti necesari in reprezentarea mesajului
este important pentru a putea aloca pentru campul message dintr-o variabila de 
tipul 'subscriber_message' numarul de bytes strict necesari in reprezentarea 
datelor.
	
	In continuare voi detalia abordarea pe care am ales-o pentru a stoca eficient 
mesajele si pentru a trimite catre clientii TCP un numar de octeti cat mai 
apropiat de dimensiunea necesara pentru a reprezenta informatia utila.

	Pentru a trimite mesaje de la server catre clientii TCP am ales sa trimit
mai intai numarul de octeti care reprezinta dimensiunea mesajului si apoi sa trimit 
efectiv mesajul. Am ales aceasta varianta pentru ca atunci cand este apelat 
'recv' in subscriber, acesta sa stie cat spatiu trebuie alocat pentru 
bufferul de receptie. Numarul de octeti ce reprezinta lungimea mesajului este 
reprezentat ca 'int'. Aceasta abordare permite folosirea eficienta a memoriei 
pentru stocarea datelor trimise catre clienti. Initial, nu am abordat situatia 
in acest fel, ci am trimis catre client intotdeauna accelasi numar de octeti
(am trimis continutul de la adresa unei variabile de tipul 'subscriber_message');
aceasta abordare s-a dovedit a fi ineficienta atunci cand se afisau mesajele 
unei client reconectat avand SF=1. O alta abordare pe care am incercat-o inainte 
de a rezolva situatia in modul in care este acum implementata a fost sa 
folosesc in structura campuri char * pentru mesaj si numele topicului (cele 2 
campuri pe care le-am considerat ca avand lungimea variabila) insa trimiterea 
unor pointeri prin socket nu are sens intrucat, la nivelul receiverului, acestia 
nu vor avea aceeasi semnificatie(se poate ajunge la seg fault). Astfel, am ales 
abordarea pe care am prezentat-o initial, accea a trimiterii un mesajului
direct in forma care trebuie afisata, trimitand anterior si dimensiunea sa.

	Pentru a construi stringul care trebuie afisat am folosit strcat, pasand 
ca argumente campurile din variabila de tipul 'subcriber_message'(acestea din urma
au fost extrase dintr-o variabila de tipul 'publisher_message' care retine mesajul
udp primit de server).

->Implementarea functionalitatii store&forward

	Inainte de prezenta detalii despre abordarea pe care am ales-o, precizez ca 
la reconectarea unui client avand SF=1 si care trebuie sa primeasca mesaje 
trimise cat a fost offline nu am obtinut o solutie care sa aduca 100% corectitudine
in ceea ce priveste mesajele afisate in client. Trimitand cu valori ale lui delay 
de 0, 1, 2 un numar destul de mare de mesaje (pana la 9000), deci un flux destul de mare, 
se intampla ocazional ca mesajele trimise sa fie interpretate eronat si sa 
lipseasca mesaje. Nu am observat ca acest lucru sa se intample atunci cand trimit 
un numar mai mic de mesaje(doar cateva sute). Totusi, cred ca problema este 
cauzata de concatenarea mesajelor realizata la nivelul TCP (observand asta prin 
afisarea valorii din bufferul de receptie al dimensiunii mesajului).

	Atunci cand serverul primeste un mesaj upd si are in lista de clienti
cel putin un client care este abonat la topicul din mesaj, am memorat in vectorul
de cienti, pentru clientul care trebuie sa primeasca mesajul la reconectare, 
mesajul efectiv care trebuie trimis impreuna cu dimensiunea sa (sub forma unei 
variabile de tipul to_forward_msg). Cand clientul se reconecteaza, ii voi trimite
mesajele stocate. Mesajele vor fi sterse din acel vector de mesaje imediat ce 
se face afisarea lor.


->Situatiile in care se afiseaza un mesaj pentru input incorect:
	-Atunci cand se incearca conectarea simultana a doi clienti cu acelasi id 
se afiseaza un mesaj pentru client si este inschis socketul asociat acelui client.
	-In situatia in care comanda de input din user este invalida: se afiseaza 
feedback pentru user, indicand care sunt comenzile valide; daca se incearca 
sa se dea subscribe din nou cu aceeasi valoarea pentru SF, se afiseaza un mesaj 
aferent; daca se da subscribe la un topic la care userul este deja abonat, se 
afiseaza un mesaj care sa arate ca SF a fost actualizat; daca se sa unsubscribe 
la un topic la care userul nu s-a abonat, se afiseaza un mesaj care sa il 
atentioneze asupra acestui fapt. (In cazurile de input invalid de la user, 
dupa afisarea mesajului de eroare aferent, rularea serverului nu este in nicio
situatie afectata).
	-Pentru situatiile de eroare semnalate de API-ul socket.h (apelui de send, recv, 
listen, select) am considerat ca cel mai sigur pentru ca erorile sa nu se propage
este sa apelez 'exit(1)' (asta dupa afisarea unui mesaj care sa duca la contextul 
in care s-a produs eroarea). De asemenea, in server este afisat un mesaj care sa
il atentioneze pe user in situatia in care a introdus de la tastatura ceva 
diferit de "exit\n".


