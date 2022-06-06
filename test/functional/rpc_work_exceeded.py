#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2021 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.pop import mine_until_pop_active
from test_framework.test_framework import BitcoinTestFramework

class WorkExceededTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-txindex", "-rpcworkqueue=100"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_pypoptools()

    def setup_network(self):
        self.setup_nodes()

        mine_until_pop_active(self.nodes[0])

    def getblocktemplate(self):
        arg = {
            "capabilities": ["coinbasetxn", "workid", "coinbase/append"],
            "rules": ["segwit"]
        }
        return self.nodes[0].getblocktemplate(arg)

    def run_test(self):
        # VBK block
        self.nodes[0].apm.mineVbkBlocks(10000)

        # spam with the getblock template rpc function
        for i in range(100000):
            res = self.getblocktemplate()
            # print(i)
        assert False



if __name__ == '__main__':
    WorkExceededTest().main()