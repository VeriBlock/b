import struct
from .messages import ser_uint256, hash256

KEYSTONE_INTERVAL = 5


def isKeystone(height):
    return height % KEYSTONE_INTERVAL == 0


def getPreviousKeystoneHeight(height):
    diff = height % KEYSTONE_INTERVAL
    if diff == 0:
        diff = KEYSTONE_INTERVAL
    prevks = height - diff
    return max(0, prevks)


def getKeystones(node, height):
    zero = '00' * 32
    if height == 0:
        return [zero, zero]

    prev1 = getPreviousKeystoneHeight(height)
    a = node.getblockhash(prev1)

    assert prev1 >= 0
    if prev1 == 0:
        return [a, zero]

    prev2 = getPreviousKeystoneHeight(prev1)
    b = node.getblockhash(prev2)

    return [a, b]


class ContextInfoContainer:
    __slots__ = ("height", "keystone1", "keystone2", "txRoot")

    @staticmethod
    def create(node, prev):
        if isinstance(prev, int):
            prev = ser_uint256(prev)[::-1].hex()

        assert (isinstance(prev, str))
        best = node.getblock(blockhash=prev)
        height = int(best['height']) + 1
        p1, p2 = getKeystones(node, height)
        return ContextInfoContainer(height, p1, p2)

    def __init__(self, height=None, keystone1=None, keystone2=None):
        self.height: int = height
        self.keystone1: str = keystone1
        self.keystone2: str = keystone2
        self.txRoot: str = ""

    def getUnauthenticated(self):
        data = b''
        data += struct.pack(">I", self.height)
        data += bytes.fromhex(self.keystone1)[::-1]
        data += bytes.fromhex(self.keystone2)[::-1]
        return data

    def getUnauthenticatedHash(self):
        # a double sha of unauthenticated context
        return hash256(self.getUnauthenticated())

    def getTopLevelMerkleRoot(self):
        data = b''
        data += bytes.fromhex(self.txRoot)[::-1]
        data += self.getUnauthenticatedHash()
        return hash256(data)[::-1]

    def __repr__(self):
        return "ContextInfo(height={}, ks1={}, ks2={}, mroot={})".format(self.height, self.keystone1,
                                                                         self.keystone2,
                                                                         self.txRoot)
