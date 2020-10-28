#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

#include "stdio.h"
#include "string.h"

int main()
{
    BIO * bio;
    SSL * ssl;
    SSL_CTX * ctx;

    int p;

    char * request = "GET / HTTP/1.0\r\nHost: www.verisign.com\r\n\r\n";
    char r[1024];

    /* initializare librarie */

    SSL_library_init ();
    ERR_load_BIO_strings();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    /* TO DO initializare context */
    ctx = SSL_CTX_new(SSLv23_client_method());
   

    /* incarca trust store */


    if(! SSL_CTX_load_verify_locations(ctx, "TrustStore.pem", NULL))
    {
        fprintf(stderr, "Error loading trust store\n");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return 0;
    }


    /* stabileste conexiune */

    bio = BIO_new_ssl_connect(ctx);
   
    /* Seteaza flag SSL_MODE_AUTO_RETRY  */

    BIO_get_ssl(bio, & ssl); //scoate obiectul de ssl din bio
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

    /* TO DO setup conexiune */
    BIO_set_conn_hostname(bio, "verisign.com:443");

    if(BIO_do_connect(bio) <= 0) {
    	printf("Eroare la conexiune\n");
    	return 1;
    }


    /* TO DO verifica certificat */
    if(SSL_get_verify_result(ssl) != X509_V_OK) {
    	//efec verificare
    	printf("Eroare verificare certificat\n");
    	return 1;
    }

    /* Trimite request */

    BIO_write(bio, request, strlen(request));

    /* TO DO Citeste raspuns si pregateste  output*/

	//citeste tot ce trimite serverul
	//se opreste cand se inchise conexiunea (cand se intoarce 0)
    // < 0 -> eroare

    FILE *fp = fopen("pagina.html", "w");

    int len = 1024;
    int x;
    while(1) {
    	x = BIO_read(bio, r, len);

    	if(x == 0) {
    		//printf("Conexiunea s-a inchis\n");
    		fclose(fp);
    		break;
   		}
	    else if(x < 0) {

	    	if(! BIO_should_retry(bio)) {
	    		printf("Esec la citire\n");
	    		continue;
	    	}
	    }
	    fprintf(fp, "%s", r);
    }
    
	

    /* Inchide conexiune si elibereaza context */
    BIO_reset(bio);

    BIO_free_all(bio);
    SSL_CTX_free(ctx);
    return 0;
}
