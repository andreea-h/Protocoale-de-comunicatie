Laborator 10 - HTTP
Horovei Andreea, 325CC

Output-ul obtinut in urma rularii comenzii 'make run' este urmatorul:

./client
GET /api/v1/dummy HTTP/1.1
Host: 3.8.116.10

SERVER_RESPONSE:
HTTP/1.1 200 OK
Access-Control-Allow-Origin: *
X-DNS-Prefetch-Control: off
X-Frame-Options: SAMEORIGIN
Strict-Transport-Security: max-age=15552000; includeSubDomains
X-Download-Options: noopen
X-Content-Type-Options: nosniff
X-XSS-Protection: 1; mode=block
Content-Type: text/html; charset=utf-8
Content-Length: 12
ETag: W/"c-axnLN5C22o98NLTYiV14pW0HhiQ"
Date: Fri, 24 Apr 2020 14:33:38 GMT
Connection: keep-alive

Hello there!

**************************************
POST /api/v1/dummy HTTP/1.1
Host: 3.8.116.10
Content-Type: application/x-www-form-urlencoded
Content-Length: 43


SERVER_RESPONSE:
HTTP/1.1 200 OK
Access-Control-Allow-Origin: *
X-DNS-Prefetch-Control: off
X-Frame-Options: SAMEORIGIN
Strict-Transport-Security: max-age=15552000; includeSubDomains
X-Download-Options: noopen
X-Content-Type-Options: nosniff
X-XSS-Protection: 1; mode=block
Content-Type: application/json; charset=utf-8
Content-Length: 50
ETag: W/"32-VDQw9vVVhc2DlsRiLl3ELqXQNXM"
Date: Fri, 24 Apr 2020 14:33:38 GMT
Connection: keep-alive

{"ceva_mesajfvbgnhmj,..ikjblalalalala;dckfdnv":""}

**************************************
POST /api/v1/auth/login HTTP/1.1
Host: 3.8.116.10
Content-Type: application/x-www-form-urlencoded
Content-Length: 33


POST /api/v1/auth/login HTTP/1.1
Host: 3.8.116.10
Content-Type: application/x-www-form-urlencoded
Content-Length: 33

username=student&password=student

SERVER_RESPONSE:
HTTP/1.1 200 OK
Access-Control-Allow-Origin: *
X-DNS-Prefetch-Control: off
X-Frame-Options: SAMEORIGIN
Strict-Transport-Security: max-age=15552000; includeSubDomains
X-Download-Options: noopen
X-Content-Type-Options: nosniff
X-XSS-Protection: 1; mode=block
Set-Cookie: connect.sid=s%3AHIe0iGmZb2aFlQSJtRFaFlsC_oS4yws2.0EPTeg27S3HsdfMssTk2o8tnAlVAKD28OyejB%2B%2FxX5M; Path=/; HttpOnly
Date: Fri, 24 Apr 2020 14:33:38 GMT
Connection: keep-alive
Content-Length: 0


COOKIE: Set-Cookie: connect.sid=s%3AHIe0iGmZb2aFlQSJtRFaFlsC_oS4yws2.0EPTeg27S3HsdfMssTk2o8tnAlVAKD28OyejB%2B%2FxX5M; Path=/; HttpOnly
Cheie:connect.sid
Valoare:s%3AHIe0iGmZb2aFlQSJtRFaFlsC_oS4yws2.0EPTeg27S3HsdfMssTk2o8tnAlVAKD28OyejB%2B%2FxX5M

**************************************
Cookie: connect.sid:s%3AHIe0iGmZb2aFlQSJtRFaFlsC_oS4yws2.0EPTeg27S3HsdfMssTk2o8tnAlVAKD28OyejB%2B%2FxX5M
GET /api/v1/weather/key HTTP/1.1
Host: 188.166.16.132

SERVER_RESPONSE:
HTTP/1.1 404 Not Found
Server: openresty
Date: Fri, 24 Apr 2020 14:33:38 GMT
Content-Type: text/html
Content-Length: 150
Connection: keep-alive

<html>
<head><title>404 Not Found</title></head>
<body>
<center><h1>404 Not Found</h1></center>
<hr><center>openresty</center>
</body>
</html>


**************************************
GET /api/v1/auth/logout HTTP/1.1
Host: 3.8.116.10

SERVER_RESPONSE:
HTTP/1.1 200 OK
Access-Control-Allow-Origin: *
X-DNS-Prefetch-Control: off
X-Frame-Options: SAMEORIGIN
Strict-Transport-Security: max-age=15552000; includeSubDomains
X-Download-Options: noopen
X-Content-Type-Options: nosniff
X-XSS-Protection: 1; mode=block
Date: Fri, 24 Apr 2020 14:33:39 GMT
Connection: keep-alive
Content-Length: 0



	Am intampinat probleme la ex 4: raspunsul dat de server este '404 Not Found'.