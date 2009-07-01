#!/bin/sh
exec java -Djavax.net.ssl.keyStore=keystore -Djavax.net.ssl.keyStorePassword=EniViD -cp "build" divine.server.DwiServer --start --verbose 25
