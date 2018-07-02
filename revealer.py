from ledgerblue.comm import getDongle
import binascii

from electrum_gui.qt.util import *
from PyQt5.QtGui import QImage
from PyQt5.QtPrintSupport import QPrinter


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

def toPdf(image):
    printer = QPrinter()
    printer.setPaperSize(QSizeF(210, 297), QPrinter.Millimeter)
    printer.setResolution(600)
    printer.setOutputFormat(QPrinter.PdfFormat)
    printer.setOutputFileName(os.getcwd() + '/test.pdf')
    printer.setPageMargins(0, 0, 0, 0, 6)
    painter = QPainter()
    painter.begin(printer)

    delta_h = round(image.width() / 34)
    delta_v = round(image.height() / 21)

    size_h = 2028 + ((int(1) * 2028 / (2028 - (delta_h * 2) + int(1))) / 2)
    size_v = 1284 + ((int(1) * 1284 / (1284 - (delta_v * 2) + int(1))) / 2)

    image = image.scaled(size_h, size_v)

    painter.drawImage(553, 533, image)
    wpath = QPainterPath()
    wpath.addRoundedRect(QRectF(553, 533, size_h, size_v), 19, 19)
    painter.setPen(QPen(Qt.black, 1))
    painter.drawPath(wpath)
    painter.end()
    return image


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

def updateColumn(img, b, x, refresh = False):
    print(x)
    white = qRgba(255, 255, 255, 0)
    black = qRgba(0, 0, 0, 255)
    for i in range(img.height()):
        if b[i]==0:
            img.setPixel(x,i,black)
        if b[i]==1:
            img.setPixel(x, i, white)
    if refresh is True:
        img.save(os.getcwd() + '/column.png')
    return img

img = QImage(159,97, QImage.Format_ARGB32)

img.YX = 0

args = get_argparser().parse_args()
dongle = getDongle(args.apdu)
#data = binascii.unhexlify("80CA000000")
#result = dongle.exchange(bytearray(data))

#if args.apdu:
#    print("<= Clear " + str(result))

for i in range(img.width()):
    data = binascii.unhexlify("80CB00" + "0x{:02x}".format(i)[2:] + "00")
    print(data)
    result = dongle.exchange(bytearray(data))
    updateColumn(img,result,i, True)
    if args.apdu:
        print("<= Clear " + str(result))
img.save(os.getcwd() + '/column.png')
img = pixelcode_2x2(img)
#img = img.convertToFormat(QImage.Format_Mono)
#img = QBitmap.fromImage(img)
#img = toPdf(QImage(img))
#QDesktopServices.openUrl (QUrl.fromLocalFile(os.getcwd() +'/test.pdf'))
img.save(os.getcwd() + '/cypher.png')

