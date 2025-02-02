
TEMA1 - PROTOCOALE DE COMUNICATIE - ROUTER

In implementarea procesului de dirijare a pachetelor dintr-un router am folosit suportul oferit de laboratoarele 4 si 5, urmarind totodata indicatiile din enunt. Mentionez ca, de asemenea, au fost foarte utile si discutiile de pe forum care m-au ajutat sa inteleg mai bine tot ceea ce am avut de facut.

Am implementat protocolul ARP si aceste functioneaza corespunzator. 

Pentru a stoca in reprezentarea interna tabela de rutare si tabela ARP am folosit structurile prezente si in cadrul laboratorului.
Tabela de rutare este reprezentate sub forma unui vector de structuri (route_table_entry), construit in functia read_rtable, prin parcurgerea fiecarei linii din fisierului 'rtable.txt', prelucrand stringul citit si realizand conversii pentru a memora datele avnd tipurile date in definitia structurii.

 ->Sortarea tabelei de rutare
 
Am realizat sortarea tabelei de rutare folosind functia predifinita qsort. Am folosit functia 'comparator' prin intermediul careia se realizeaza sortarea: intrarile din tabela de rutare sunt sortate crescator dupa prefix iar intrarile care au prefixele egale sunt sortate in ordinea descrescatoare a numarului de biti de 1 din masca. In acest fel, prima intrare din tabela sortata, avand o anumita valoare a prefixului, va fi cea care are masca cea mai lunga, deci cea pe care a alegem pentru a obtine next_hop-ul unui pachet care trebuie dirijat. Deci, pentru a obtine intrarea cea mai specifica din tabela de rutare asociata unei anumite adrese destinatie, am definit functia 'get_best_route' care efectueaza o cautare de tipul binary search in tabela sortata conform principiului prezentat anterior, intorcand indicele primei intrari gasite care face match cu adresa destinatie (dest_ip & intrare.mask == intrare.prefix).

->Implementare ARP
Pentru fiecare pachet IP primit de router, am verificat daca acesta este un pachet de tipul IP sau ARP prin verificarea campului ether_type din headerul ethernet al pachetului primit.

In cazul in care routerul primeste un pachet ARP, am analizat campul arp_type din headerul arp (pentru headerul arp am folosit structura ether_arp).
In situatia in care la router a ajuns un pachet de tipul 'ARP Request', am verificat daca acesta ii este destinat routerului adica daca adresa IP destinatie extrasa din headerul ARP (target protocol address) este aceeasi cu adresa IP a interfetei routerului cu hostul de pe care a fost trimis pachetul. In cazul in care cele doua adrese mentionate anterior coincid, am construit un pachet de tipul ARP reply, pe baza pachetului primit, modificand campuri din headerul ethernet si arp. Adresa MAC pentru care s-a facut request este plasata in campul 'arp->sha' din headerul arp, aceasta fiind adresa interfetei routerului cu hostul care a facut cererea (este obtinuta cu functia get_intreface_mac() care primeste ca prim parametru interfata pachetului request primit). In cadrul pachetului ARP Reply trimis, campurile arp_tpa si arp_spa vor fi inversate. Pachetul Reply va fi trimis pe interfata pe care a sosit pachetul Request.

Atunci cand routerul primeste un pachet ARP Reply, este scos un pachet din coada care retine pachetele care nu fost dirijate si este trimis mai departe pe interfata indicata prin campul 'interface' din structura pachetului. Adresa MAC destinatie din headerul ethernet al pachetului trimis este adresa din campul arp_sha al pachetului arp_reply care tocmai a ajuns la router; adresa sursa din headerul ethernet este adresa MAC asociata interfetei pachetului. Pachetul scos din coada este trimis pe interfata indicata de campul interface din structura pachetului.

->Implementarea procesului de dirijare

Procesul de dirijare al unui pachet implica cautarea in tabela de rutare, pentru aceasta folosind functia get_best_route descrisa mai sus, care este o forma modificata a binary search. Pentru fiecare pachet IP primit de router, daca acesta nu este destinat routerului(in acesta situatie am avea posibilitatea trimiterii unui IMCP Echo Reply), dupa ce se fac verificari pentru TTl si checksum, este cautata intrare din tabela de rutare cu cea mai buna potrivire cu adresa IP destinatie a pachetului primit. Odata determinata best_route, se extrage campul next_hop asociat si se verifica daca tabela ARP contine o intrare cu adresa IP catre care trebuie trimis pachetul. In situatia in care adresa MAC este cunoscuta local, se trimite acum pachetul la destinatie, altfel, routerul trimite un ARP Request iar pachetul este salvat in coada si se asteapta ARP Reply cu adresa MAC destinatie.

->Implementare suport pentru ICMP 

Pentru cele trei tipuri de mesaje ICMP pe care routerul le poate trimite ca raspuns, am construit pachete ICMP de la zero. In structura acestor pachete am setat campurile specifice headerelor ethernet, ip si icmp in cooncordanta cu tipul de pachet care trebuie trimis si anume: pentru un pachet de tipul 'Time exceeded', campul type din headerul icmp are valoarea 11, pentru un pachet de tip 'Destination unreachable' type este 3, iar pentru 'Echo Reply' are valoarea 0. Restul campurilor au fost complemtate in maniera similara cu abordarea din Laboratorul 5.
