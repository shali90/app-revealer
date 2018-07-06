from ledgerblue.comm import getDongle
import binascii

from electrum_gui.qt.util import *
from PyQt5.QtGui import QImage
from PyQt5.QtPrintSupport import QPrinter

from PIL import Image


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
    #img = QImage(image.width, image.height, QImage.Format_Mono)
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
    result = QImage(img.width() * 2, img.height() * 2, QImage.Format_Mono)
    for x in range(img.width()):
        for y in range(img.height()):
            c = img.pixel(QPoint(x, y))
            colors = QColor(c).getRgbF()
            if colors[0]:
                result.setPixel(x * 2 + 1, y * 2 + 1, 1)
                result.setPixel(x * 2, y * 2 + 1, 0)
                result.setPixel(x * 2 + 1, y * 2, 0)
                result.setPixel(x * 2, y * 2, 1)
            else:
                result.setPixel(x * 2 + 1, y * 2 + 1, 0)
                result.setPixel(x * 2, y * 2 + 1, 1)
                result.setPixel(x * 2 + 1, y * 2, 1)
                result.setPixel(x * 2, y * 2, 0)
    return result

def overlay_marks(img):
    f_size = QSize(1014 * 2, 642 * 2)
    abstand_v = 34

    border_color = Qt.white
    base_img = QImage(f_size.width(), f_size.height(), QImage.Format_ARGB32)
    base_img.fill(border_color)
    img = QImage(img)

    painter = QPainter()
    painter.begin(base_img)

    total_distance_h = round(base_img.width() / abstand_v)
    dist_v = round(total_distance_h) / 2
    dist_h = round(total_distance_h) / 2

    img = img.scaledToWidth(base_img.width() - (2 * (total_distance_h)))
    painter.drawImage(total_distance_h,
                      total_distance_h,
                      img)

    # frame around image
    pen = QPen(Qt.black, 2)
    painter.setPen(pen)

    # horz
    painter.drawLine(0, total_distance_h, base_img.width(), total_distance_h)
    painter.drawLine(0, base_img.height() - (total_distance_h), base_img.width(),
                     base_img.height() - (total_distance_h))
    # vert
    painter.drawLine(total_distance_h, 0, total_distance_h, base_img.height())
    painter.drawLine(base_img.width() - (total_distance_h), 0, base_img.width() - (total_distance_h), base_img.height())

    # border around img
    border_thick = 6
    Rpath = QPainterPath()
    Rpath.addRect(QRectF((total_distance_h) + (border_thick / 2),
                         (total_distance_h) + (border_thick / 2),
                         base_img.width() - ((total_distance_h) * 2) - ((border_thick) - 1),
                         (base_img.height() - ((total_distance_h)) * 2) - ((border_thick) - 1)))
    pen = QPen(Qt.black, border_thick)
    pen.setJoinStyle(Qt.MiterJoin)

    painter.setPen(pen)
    painter.drawPath(Rpath)

    Bpath = QPainterPath()
    Bpath.addRect(QRectF((total_distance_h), (total_distance_h),
                         base_img.width() - ((total_distance_h) * 2), (base_img.height() - ((total_distance_h)) * 2)))
    pen = QPen(Qt.black, 1)
    painter.setPen(pen)
    painter.drawPath(Bpath)

    pen = QPen(Qt.black, 1)
    painter.setPen(pen)
    painter.drawLine(0, base_img.height() / 2, total_distance_h, base_img.height() / 2)
    painter.drawLine(base_img.width() / 2, 0, base_img.width() / 2, total_distance_h)

    painter.drawLine(base_img.width() - total_distance_h, base_img.height() / 2, base_img.width(),
                     base_img.height() / 2)
    painter.drawLine(base_img.width() / 2, base_img.height(), base_img.width() / 2,
                     base_img.height() - total_distance_h)

    painter.setPen(QPen(Qt.black, 1, Qt.DashDotDotLine))
    painter.drawLine(0, dist_v, base_img.width(), dist_v)
    painter.drawLine(dist_h, 0, dist_h, base_img.height())
    painter.drawLine(0, base_img.height() - dist_v, base_img.width(), base_img.height() - (dist_v))
    painter.drawLine(base_img.width() - (dist_h), 0, base_img.width() - (dist_h), base_img.height())

    painter.drawImage(f_size.width()-163-123, f_size.height()-148-128,
                      QImage(os.getcwd() + '/Logo.png').scaledToWidth(4 * (total_distance_h), Qt.SmoothTransformation))

    painter.end()

    return base_img

def updateColumn(img, b, x, refresh = False):
    print(x)
    white = qRgba(255, 255, 255, 0)
    black = qRgba(0, 0, 0, 255)
    for i in range(img.height()):
        if b[i]==0:
            img.setPixel(x,i,0)
        if b[i]==1:
            img.setPixel(x, i, 1)
    if refresh is True:
        img.save(os.getcwd() + '/column.png')
    return img

img = QImage(159,97, QImage.Format_Mono)

args = get_argparser().parse_args()
dongle = getDongle(args.apdu)

for i in range(img.width()):
    data = binascii.unhexlify("80CB00" + "0x{:02x}".format(i)[2:] + "00")
    print(data)
    result = dongle.exchange(bytearray(data))
    updateColumn(img,result,i, False)
    if args.apdu:
        print("<= Clear " + str(result))
#img.save(os.getcwd() + '/column.png')
#img = img.convertToFormat(QImage.Format_Mono)
cypher = pixelcode_2x2(img)
cypher.invertPixels()
#cypher = QBitmap.fromImage(cypher)
cypher = overlay_marks(cypher)
cypher = cypher.scaled(1014, 642)
cypher.save(os.getcwd() + '/cypher.png')

#pdf = toPdf(QImage(cypher))



#pdf.save(os.getcwd() + '/cypher.pdf')

#test = Image.open(os.getcwd() + '/cypher.png')
#test.save(os.getcwd() + '/cypher.pdf', 'pdf')
