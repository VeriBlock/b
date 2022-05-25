#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2021 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from decimal import Decimal
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    connect_nodes,
    p2p_port
)

class WorkExceededTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-txindex", "-rpcworkqueue=100"]]

    def setup_network(self):
        self.setup_nodes()

    def getblocktemplate(self):
        arg = {
            "capabilities": ["coinbasetxn", "workid", "coinbase/append"],
            "rules": ["segwit"]
        }
        return self.nodes[0].getblocktemplate(arg)

    def run_test(self):
        # mine 200 blocks
        self.nodes[0].generate(200)

        # spam with the getblock template rpc function
        for i in range(100000):
            res = self.getblocktemplate()
            # print(i)
        assert False



if __name__ == '__main__':
    WorkExceededTest().main()