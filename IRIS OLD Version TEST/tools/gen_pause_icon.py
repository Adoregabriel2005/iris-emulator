from PySide6.QtWidgets import QApplication
from PySide6.QtGui import QImage, QPainter, QColor, QPainterPath
import sys

app = QApplication(sys.argv)

img = QImage(512, 512, QImage.Format.Format_ARGB32)
img.fill(QColor(13, 13, 26, 255))
p = QPainter(img)
p.setRenderHint(QPainter.RenderHint.Antialiasing)

# Circulo de fundo
p.setBrush(QColor(30, 30, 60))
p.setPen(QColor(74, 144, 217, 180))
p.drawEllipse(56, 56, 400, 400)

# Duas barras do pause
p.setBrush(QColor(74, 144, 217))
p.setPen(QColor(0,0,0,0))
p.drawRoundedRect(160, 156, 72, 200, 12, 12)
p.drawRoundedRect(280, 156, 72, 200, 12, 12)

p.end()
img.save("src/resources/pause_icon_512.png")
print("Salvo: src/resources/pause_icon_512.png")
