#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2022 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

from requests import post
from requests.auth import HTTPBasicAuth

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

    def __getattr__(self, name):
        def method(*args, **kwargs):
            payload = {
                "id": self.nonce,
                "method": name,
                "params": list(args) + list(kwargs.values())
            }
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

        return method

rpc_port = 8332
rpc_url = "http://{}:{}/".format("127.0.0.1", rpc_port)
rpc_user = "user"
rpc_password = "pass"

rpc = JsonRpcApi(rpc_url, user=rpc_user, password=rpc_password)

walk_block = rpc.getblock(rpc.getbestblockhash())

# Find last contained vtb alt block
def find_last_contained_vtb_block(rpc):
    walk_block = rpc.getblock(rpc.getbestblockhash())
    while walk_block != None:
        walk_block = rpc.getblock(walk_block['previousblockhash'])
        print("height: {}".format(walk_block['height']), end="\r", flush=True)
        vtbs = walk_block["pop"]["state"]["stored"]["vtbs"]
        if len(vtbs) > 0:
            print("height: {}".format(walk_block['height']))
            return walk_block 

block = find_last_contained_vtb_block(rpc=rpc)
print(block["hash"])