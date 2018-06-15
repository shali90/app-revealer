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
dongle = getDongle(args.apdu)
data = binascii.unhexlify("80CA000000")
result = dongle.exchange(bytearray(data))
if args.apdu:
    print("<= Clear " + str(result))