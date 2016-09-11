#!/bin/sh
#spawn-fcgi -a 127.0.0.1 -p 8081 -f ./demo
#spawn-fcgi -a 127.0.0.1 -p 8082 -f ./echo_test
#spawn-fcgi -a 127.0.0.1 -p 8082 -f ./load_get
spawn-fcgi -a 127.0.0.1 -p 8082 -f ./load
spawn-fcgi -a 127.0.0.1 -p 8083 -f ./data_cgi
spawn-fcgi -a 127.0.0.1 -p 8084 -f ./reg_cgi
spawn-fcgi -a 127.0.0.1 -p 8085 -f ./login_cgi
