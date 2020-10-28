
Readme Test practic

Pentru a reprezenta produsele am definit tipul 'produs' care contine numele celui care l-a comandat si descrierea. Produsele sunt memorate sub forma unui vector de structuri de tipul 'produs'.

La fiecare conectare a unui client nou, trimit imedait dupa apelul connect din client si numele clientului preluat din argv[3]. Verific daca nu exista deja un client cu acest nume in server (daca exista deja, cererea curenta este ignorata). 

Am retinut un vector cu numele clientilor: la indexul i in vector de afla numele clientului conectat pe socketul i.

Comenzile all, add descriere si show sunt functionale din cate am testat eu.

La show clientul introduce comanda "show" dar trimit din client stringul concatenat "show nume_client", ca sa pot identifica in server produsele din lista de produse. Daca clientul care a introdus show nu a cerut nimic, afisez in mesaj corespunzator (la fel si pt all).

Alte detalii in comentariile din cod.

La introducerea comenzii exit, clientul de inchide si se afiseaza un mesaj corespunzator in server.
	

	Makefile-ul nu contine regula run. Am rulat astfel: ./server port_server, ./client ip_server port_server nume_client.

