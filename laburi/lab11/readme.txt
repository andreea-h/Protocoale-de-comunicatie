Horovei Andreea, 325CC
Lab11

Atasez output-ul obtinut la rularea comenzii 'make run' si raspunsul din server:


./send_mail 127.0.0.1 file.txt
Am primit: 220 andreea-ubuntu Python SMTP proxy version 0.3

Trimit: HELO localhost
Am primit: 250 andreea-ubuntu
Trimit: MAIL FROM: <cineva1@upb.ro>
Am primit: 250 OK
Trimit: RCPT TO: <cineva2@upb.ro>
Am primit: 250 OK
Trimit: DATA
Am primit: 354 End data with <CR><LF>.<CR><LF>
Trimit: MIME-Version: 1.0
From: cineva_care_trimite_mail <cineva1@upb.ro>
To: cineva_care_primeste_mail <cineva2@upb.ro>
Subject: Re: un_subiect
Content-Type: multipart/mixed; boundary=fff
--fff
Content-Type: text/plain
acesta_este_continutul_mail-ului
Toate cele bune,
cineva_care_a_trimis_mail
--fff
Content-Type: text/plain
Content-Disposition: attachment; filename="file.txt"
This is an attachment.
This is attachment1.
This is attachment2.
This is attachment3.
--fff
.
Am primit: 250 OK
Trimit: QUIT
Am primit: 221 Bye

/***************************************************************************8

---------- MESSAGE FOLLOWS ----------
b'MIME-Version: 1.0'
b'From: cineva_care_trimite_mail <cineva1@upb.ro>'
b'To: cineva_care_primeste_mail <cineva2@upb.ro>'
b'Subject: Re: un_subiect'
b'Content-Type: multipart/mixed; boundary=fff'
b'--fff'
b'Content-Type: text/plain'
b'acesta_este_continutul_mail-ului'
b'Toate cele bune,'
b'cineva_care_a_trimis_mail'
b'--fff'
b'Content-Type: text/plain'
b'Content-Disposition: attachment; filename="file.txt"'
b'This is an attachment.'
b'This is attachment1.'
b'This is attachment2.'
b'This is attachment3.'
b'--fff'
------------ END MESSAGE ------------
