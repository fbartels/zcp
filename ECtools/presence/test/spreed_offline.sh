#!/bin/sh
curl -u presence:presence -X PUT -d '{"AuthenticationToken":"1421324321:markd@zarafa.com:WFAI3NU3YBAA91F4+9BOILXYGTPAV+PVDRNKIJ+JCVY=", "Type": "UserStatus", "UserStatus": [{"user_id": "markd", "spreed":{"status":"away","message":"toilet"}}]}' http://localhost:1234/ -H "Content-Type: application/json"
./get_status.sh
