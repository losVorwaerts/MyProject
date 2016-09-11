#! /bin/sh

UPLOAD=`ps aux | grep load | grep Ss | sed "2,2d" | awk {'print $2'}`

DATA_CGI=`ps aux | grep data_cgi | grep Ss | sed "2,2d" | awk {'print $2'}`

REG_CGI=`ps aux | grep reg_cgi | grep Ss | sed "2,2d" | awk {'print $2'}`

LOGIN_CGI=`ps aux | grep login_cgi | grep Ss | sed "2,2d" | awk {'print $2'}`

if [ -z ${UPLOAD} ]
then    echo "load not start"
else    kill $UPLOAD
fi
if [ -z $DATA_CGI ]
then    echo "data_cgi not start"
else    kill $DATA_CGI
fi
if [ -z $REG_CGI ]
then    echo "reg_cgi not start"
else    kill $REG_CGI
fi
if [ -z $LOGIN_CGI ]
then    echo "login_cgi not start"
else    kill $LOGIN_CGI
fi
