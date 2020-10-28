

	Tema3 PC - Client Web. Comunicatie cu REST API

->Folosirea bibliotecii 'parson'

In implementarea solutiei mele, am ales sa folosesc biblioteca 'parson', 
recomandata in enunt si preluata de la link-ul de github dat. Am ales sa folosesc 
aceasta biblioteca deoarece sunt de parere ca parsarea raspunsului serverului 
se poate face intr-o maniera mai eleganta cu functii care opereaza date de tipul 
'JSON_Object', 'JSON_Value' sau 'JSON_Array' (incluse in biblioteca de mai sus), 
decat folosind functii care lucreaza cu string-uri si care necesita o prelucrare
mai amanuntita a raspunsului dat de server.
	Am folosit ca schelet al temei o parte din laboratul 10, mai exact functiile
din requests.c (compute_get_request si compute_post_request),functiile 
pentru stabilirea respectiv inchiderea conexiunii cu serverul (open_connection, 
close_connection) si cele pentru trimiterea si primirea de mesaje catre/ de la 
server (send_to_server si receive_from_server).

Pentru a mentine conexiunea clientului cu serverul am stabilit ca, in
cadrul implementarii, sa deschid si sa inchid conexiunea pentru fiecare comanda
pe care o trimite clientul catre server pentru a abordare mai unitara a comenzilor
cat si pentru a evita erori care pot aparea legate de conexiunea cu serverul.
Am definit cate o functie pentru fiecare comanda in cadrul careia am implementat 
formatarea mesajului care trebuie trimis, precum si parsarea raspunsului dat 
de server.

->Format afisare

Am incercat sa printez mesaje catre user dupa fiecare comanda intr-un format 
cat mai prietenos. In parsarea raspunsului dat se server, am folosit o variabila
de tipul JSON_Value prin intermediul careia am identificat daca raspunsul contine 
un mesaj de eroare al serverului. In caz afirmativ, am folosit mesajul dat de 
server (extras cu functia 'json_object_dotget_string') pentru a atentiona userul 
(afisand direct ceea ce am extras de la server sau afisand un mesaj formatat in 
concordanta cu situatia in care s-a produs eroarea). In cazul in care a fost
executata cu succes comanda introdusa de user, se va afisa un mesaj care sa 
indice acest lucru. Intoducerea de la tastatura a oricarei alte comenzi, diferita 
de comenzile prezentate in enunt va afisa un mesaj care sa atentioneze userul
asupra comenzii invalide.


Pentru a retine informatii despre starea curenta a userului adica username, 
cookie, token jwt am definit structura client_info in care se va completa campul
username la logarea unei client, numele fiind folosit pentru mesaje care
sa creeze o interfata cat mai prietenoasa. In structura client_info am inclus si 
cookie_set si token_set, date de tip bool, care sa semnaleze daca userul curent
a obtinut la logare un cookie de sesiune, respectiv daca a primit un token JWT 
care sa ii garanteze accesul la biblioteca (ultimele 2 campuri prezentate sunt
actualizate in cadrul functiilor login respectiv enter_library, precum si la 
logout unde se 'sterg' datele de conectare (cookie si token) ale clientului 
care a parasit biblioteca). 

->Situatii de eroare tratate

Am folosit valorea campului 'login_flag' din structura 'client_info' pentru
a identifica situatia in care se incearca comanda login, cat timp un user deja
s-a autentificat. Am tratat acest caz de input invalid prin afisarea unui mesaj 
de eroare catre user, fara a mai trimite vreau mesaj catre server, trimiterea 
cererii de login catre server fiind deci conditionata de valorea 'login_flag'
egala cu false.
	
Pentru situatia in care se cere enter_library in contextul in care nu este
logat niciun user, am folosit raspunsul dat de server pentru a atentiona userul
asupra comenzii invalide. Analog am procedat si pentru situatia in care se 
introduce get_book cu niciun user logat sau fara acces la biblioteca sau cand 
se incearca comenzile add_book, delete_book fara a avea acces. In cazurile
precizate am extras din raspunsul dat de server acea parte care descrie eroarea
(implementare in 'parse_error_msg'), incercand sa afisez mesajul intr-un format 
cat mai prietenos.
	Pentru id de carte invalid introdus la comanda get_book, am ales sa trimit 
catre server cererea chiar daca id-ul nu este intr-o forma invalida si sa folosesc
ulterior raspunsul serverului pentru a-i semnala userului faptul ca id-ul nu este 
de tipul int (pentru asta am verificat daca raspunsul intors de server, parsat 
intr-o data de tipul JSON_Object, are asociata o valoare nenula pentru campul
'error'). Pentru situatia de id valid, am extras din obiectul JSON_Object 
construit din raspunsul dat de server, valorile asociate pentru campurile 
id, title, author, publisher, genre, page_count.
	
Pentru adaugare unei carti, am verificat validitatea campurilor introduse de 
user si, doar daca acestea respecta formatul precizat am trimis cererea catre 
server. Am tratat aceasta situatie in acest mod deoarece am observat faptul ca
serverul nu valideaza valorile campurilor introduse. Deci, in abordarea pe care 
am ales-o afisat un mesaj de eroare care atentioneza user-ul("Wrong data format. 
Please check the information you entered.") in urmatoarele situatii:
	-campul id nu este numar pozitiv sau nu este un intreg (nu poate fi string 
sau numar cu virgula).
	-celelalte valori care au ca valori date de tipul String nu pot fi alcatuite 
doar din cifre (am considerat ca nu este valid va avem, de exemplu, un nume de 
carte format doar din cifre).
