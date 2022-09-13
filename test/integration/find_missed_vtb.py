#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2022 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

from requests import post
from requests.auth import HTTPBasicAuth
import time

class JsonRpcException(Exception):
    def __init__(self, error, http_status):
        super().__init__('{} (code: {})'.format(error['message'], error['code']))
        self.error = error
        self.http_status = http_status


class JsonRpcApi:
    def __init__(self, url, user=None, password=None):
        self.url = url
        self.nonce = 0
        if user is not None and password is not None:
            self.auth = HTTPBasicAuth(user, password)
        else:
            self.auth = None

    def execute(self, payload):
        response = post(
            url=self.url,
            auth=self.auth,
            json=payload,
            headers={
                "Content-type": "application/json"
            },
            timeout=300
        )
        self.nonce += 1
            
        response_body = response.json()
        http_status = response.status_code

        if response_body.get('error', None) is not None:
            raise JsonRpcException(response_body['error'], http_status)
        elif 'result' not in response_body:
            raise JsonRpcException({'code': -343, 'message': 'missing JSON-RPC result'}, http_status)
        elif http_status != 200:
            raise JsonRpcException({'code': -342, 'message': 'non-200 HTTP status code'}, http_status)
        else:
            return response_body['result']


    def call(self, name, params):
        payload = {
                "jsonrpc": "2.0",
                "id": self.nonce,
                "method": name,
                "params": params,
        }
        return self.execute(payload)

btc_rpc_port = 8332
btc_rpc_url = "http://{}:{}/".format("127.0.0.1", btc_rpc_port)
btc_rpc_user = "user"
btc_rpc_password = "pass"
btc_rpc = JsonRpcApi(btc_rpc_url, user=btc_rpc_user, password=btc_rpc_password)

vbk_rpc_port = 10500
vbk_rpc_url = "http://{}:{}/".format("127.0.0.1", vbk_rpc_port)
vbk_rpc_user = ""
vbk_rpc_password = ""
vbk_rpc = JsonRpcApi(vbk_rpc_url, user=vbk_rpc_user, password=vbk_rpc_password)

# Find last contained vtb alt block
# Founded block hash: "0000000000000219688ebe3acf46507b2edb5c1eb349283fa51feebc0cf7f570", block height: 950080
def find_last_contained_vtb_alt_block(rpc):
    walk_block = rpc.call("getblock", [rpc.call("getbestblockhash", [])])
    while walk_block != None:
        walk_block = rpc.call("getblock", [walk_block["previousblockhash"]])
        print("height: {}".format(walk_block["height"]), end="\r", flush=True)
        vtbs = walk_block["pop"]["state"]["stored"]["vtbs"]
        if len(vtbs) > 0:
            print("height: {}".format(walk_block["height"]))
            return walk_block

# Find last contained vtb vbk block
# Founded block hash: "0000000003d22d88b51e37304e251dfcb679ce06f2ee3d6e", block height: 3186756
def find_last_contained_vtb_vbk_block(rpc):
    walk_block = rpc.call("getvbkblock", [rpc.call("getvbkbestblockhash", [])])
    while walk_block != None:
        time.sleep(0.1)
        walk_block = rpc.call("getvbkblock", [walk_block["header"]["previousBlock"]])
        print("height: {}".format(walk_block["height"]), end="\r", flush=True)
        vtbs = walk_block["stored"]["vtbids"]
        if len(vtbs) > 0:
            print("height: {}".format(walk_block["height"]))
            return walk_block

block = find_last_contained_vtb_vbk_block(rpc=btc_rpc)
print(block["header"]["hash"])