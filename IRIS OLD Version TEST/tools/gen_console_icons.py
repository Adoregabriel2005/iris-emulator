from PySide6.QtWidgets import QApplication
from PySide6.QtSvg import QSvgRenderer
from PySide6.QtGui import QImage, QPainter, QColor
import sys

app = QApplication(sys.argv)

pairs = [
    ("console-atari2600", "atari2600_icon"),
    ("console-atarilynx", "lynx_icon"),
]

for svg_name, out_name in pairs:
    img = QImage(512, 512, QImage.Format.Format_ARGB32)
    img.fill(QColor(13, 13, 26, 255))
    painter = QPainter(img)
    renderer = QSvgRenderer(f"src/resources/icons/white/svg/{svg_name}.svg")
    renderer.render(painter)
    painter.end()
    img.save(f"src/resources/{out_name}_512.png")
    print(f"Salvo: src/resources/{out_name}_512.png")
