from ledgerblue.comm import getDongle
from ledgerblue.deployed import getDeployedSecretV2
from ledgerblue.ecWrapper import PrivateKey
from Crypto.Cipher import AES
import sys
import fileinput
import binascii
from ledgerblue.hexLoader import HexLoader

from hashlib import sha256
import random

from electrum_gui.qt.util import *
import array


def auto_int(x):
    return int(x, 0)


import argparse


def get_argparser():
    parser = argparse.ArgumentParser(description="""Read a sequence of command APDUs from a file and send them to the
device. The file must be formatted as hex, with one CAPDU per line.""")
    parser.add_argument("--noiseSeed", help="noise seed")
    parser.add_argument("--apdu", help="Display APDU log", action='store_true')
    parser.add_argument("--targetId", help="The device's target ID (default is Ledger Nano S)", type=auto_int)
    return parser


args = get_argparser().parse_args()


def code_hashid(txt):
    x = txt.encode("utf-8")
    hash = sha256(x).hexdigest()
    return hash[-3:].upper()

def is_noise(txt):
    id = code_hashid(txt[:-3])
    if (txt[-3:].upper() == id.upper()):
        return True
    return False

def setBit(int_type, offset):
    mask = 1 << offset
    return (int_type | mask)

def encodeInt(array):
    a = 0
    for i in range(16):
        if array[i]:
            setBit(a, i)
    return a

noiseSeed = ""

if args.noiseSeed:
    noiseSeed = args.noiseSeed

if is_noise(noiseSeed.lower()):
    print("Noise Seed is valid")
    w = 159
    h = 97
    rawnoise = QImage(w, h, QImage.Format_Mono)
    noiseSeedInt = int(noiseSeed[:-3],16)
    random.seed(noiseSeedInt)
    dongle = getDongle(args.apdu)
    noiseSeed = noiseSeed.upper().encode("utf-8")
    i = 0
    randPixels = []
    byteEncodedPixels = []
    b = 0
    for x in range(w):
        for y in range(h):
            randPixels += [random.randint(0, 1)]
            rawnoise.setPixel(x, y, randPixels[-1])
            b = (b << 1) | randPixels[-1]
            if (i+1)%8 is 0:
                byteEncodedPixels += [b]
                b = 0
            i = i+1
    b = 0

    #last byteEncodedPixel
    for i in range(7):
        b = (b << 1) | randPixels[-7+i]
    byteEncodedPixels += [b]

    l = int(len(byteEncodedPixels)/8)

    #data = binascii.unhexlify("80CA000000")
    #result = dongle.exchange(bytearray(data))
    #if args.apdu:
    #    print("<= Clear " + str(result))

    for i in range(7):
        data = binascii.unhexlify("80CA000000") + bytes(byteEncodedPixels[i*l:(i+1)*l])
        result = dongle.exchange(bytearray(data))
        if args.apdu:
            print("<= Clear " + str(result))

    data = binascii.unhexlify("80CB000000") + bytes(byteEncodedPixels[7 * l:(7 + 1) * l])
    result = dongle.exchange(bytearray(data))
    if args.apdu:
        print("<= Clear " + str(result))
else:
    print("invalid noise seed")
