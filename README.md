# acmacs-webserver
Lightweight multi-threaded webserver with websockets support

# Apache httpd.conf

enable mod\_ssl, mod\_proxy, mod\_proxy\_http, mod\_proxy\_wstunnel

```xml
<IfModule mod_ssl.c>
    <VirtualHost *:443>
        SSLEngine on
        SSLCertificateFile "path/self-signed.crt"
        SSLCertificateKeyFile "path/self-signed.key"

        <IfModule proxy_http_module>
            SSLProxyEngine on
            SSLProxyVerify none
            SSLProxyCheckPeerCN off
            SSLProxyCheckPeerName off
            SSLProxyCheckPeerExpire off

            ProxyPass /ads/ https://localhost:10067/
            #ProxyPassReverse /ads https://localhost:10067/
        </IfModule>

        <IfModule proxy_wstunnel_module>
            ProxyPass "/ads-ws/" "wss://localhost:10067/ads-ws"
        </IfModule>

        # to properly handle python3 cgitb output
        HttpProtocolOptions Unsafe

    </VirtualHost>
</IfModule>
```
