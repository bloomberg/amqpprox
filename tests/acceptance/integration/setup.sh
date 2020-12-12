#! /bin/sh
curl -s -u guest:guest -H "content-type:application/json" -XPOST http://localhost:15672/api/definitions -d @full-definitions.json
curl -s -u guest:guest -H "content-type:application/json" -XPOST http://localhost:15675/api/definitions -d @full-definitions.json
