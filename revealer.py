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
from PyQt5.QtGui import QImage, QBitmap
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

CHUNK_SIZE = 250

def update_image_byte(img,b, bnum, chunk, x,y):
    white = qRgba(255, 255, 255, 0)
    black = qRgba(0, 0, 0, 255)
    #y_int = y
    x_int = bnum * 8 + x
    if y == 13:
        a = 0
    if x_int >= img.width():
        y = int(x_int/img.width())
        x_int = x_int % img.width()
    for i in range(8):
        if x_int == img.width():
            y+=1
            #x_int = (x_int+i)%img.width()
            x_int = 0
        if (b>>i)&1 == 0:
            img.setPixel(x_int,y,black)
        elif (b>>i)&1 == 1:
            img.setPixel(x_int,y,white)
        if y == 13:
            a = 10
        print(x_int,y)
        x_int += 1
    return img

def update_image_chunk(img, bytes, chunk, last=False):
    #one chunk is 250*8 pixels = 2000 pixels
    pixel_idx = chunk*CHUNK_SIZE*8
    bnum = 0
    for byte in bytes:
        update_image_byte(img, byte, bnum, chunk, pixel_idx%img.width(), int(pixel_idx/img.width()))
        bnum += 1
    return img

def blank(img):
    white = qRgba(255, 255, 255, 0)
    black = qRgba(0, 0, 0, 255)
    for i in range(img.width()):
        for j in range(img.height()):
            #print(i, j)
            img.setPixel(i,j,white)
    return img

def update_image_byte2(img):
    return img

def update_image_chunk2(img, bytes):
    white = qRgba(255, 255, 255, 0)
    black = qRgba(0, 0, 0, 255)
    y = int(img.px_cnt / img.width())
    x = img.px_cnt % img.width()
    if img.px_cnt == 0:
        x = 0
        y = 0
    for b in bytes:
        for i in range(8):
            if img.px_cnt >= 15423:
                return img
            print(x,y)
            if (b >> i) & 1 == 0:
                img.setPixel(x, y, black)
            elif (b >> i) & 1 == 1:
                img.setPixel(x, y, white)
            img.px_cnt += 1
            y = int(img.px_cnt/img.width())
            x = img.px_cnt%img.width()
    return img

def pixelcode_2x2(img):
    result = QImage(img.width() * 2, img.height() * 2, QImage.Format_ARGB32)
    white = qRgba(255, 255, 255, 0)
    black = qRgba(0, 0, 0, 255)

    for x in range(img.width()):
        for y in range(img.height()):
            c = img.pixel(QPoint(x, y))
            colors = QColor(c).getRgbF()
            if colors[0]:
                result.setPixel(x * 2 + 1, y * 2 + 1, black)
                result.setPixel(x * 2, y * 2 + 1, white)
                result.setPixel(x * 2 + 1, y * 2, white)
                result.setPixel(x * 2, y * 2, black)

            else:
                result.setPixel(x * 2 + 1, y * 2 + 1, white)
                result.setPixel(x * 2, y * 2 + 1, black)
                result.setPixel(x * 2 + 1, y * 2, black)
                result.setPixel(x * 2, y * 2, white)
    return result



img = QImage(159,97, QImage.Format_ARGB32)
#bitmap = QBitmap.fromImage(img, Qt.MonoOnly)
#bitmap.fill(Qt.white)
#painter = QPainter()
#painter.begin(bitmap)
#painter.end()
#img = bitmap.toImage()

#img.setPixel(x,y,random.randint(0, 1))
img = blank(img)

img.px_cnt = 0
if 0:
    data = [0]*250

    for i in range(8):
        update_image_chunk2(img,data)

    img.save('/home/philippe/Nano S projects/bolos-app-revealer/test.png')

    img = img.convertToFormat(QImage.Format_Mono)
    img = pixelcode_2x2(img)

    img.save('/home/philippe/Nano S projects/bolos-app-revealer/cypherseed.png')
if 1:
    args = get_argparser().parse_args()
    dongle = getDongle(args.apdu)

    data = binascii.unhexlify("80CA000000")
    result = dongle.exchange(bytearray(data))
    if args.apdu:
        print("<= Clear " + str(result))
    img = update_image_chunk2(img, result)
    #img.save('/home/philippe/Nano S projects/bolos-app-revealer/test0.png')
    for i in range(7):
        data = binascii.unhexlify("80CB000"+str(i+1)+"00")
        result = dongle.exchange(bytearray(data))
        if args.apdu:
            print("<= Clear " + str(result))
        img = update_image_chunk2(img, result)
    img.save('/home/philippe/Nano S projects/bolos-app-revealer/test.png')
    img = img.convertToFormat(QImage.Format_Mono)
    img = pixelcode_2x2(img)

    img.save('/home/philippe/Nano S projects/bolos-app-revealer/cypherseed.png')

