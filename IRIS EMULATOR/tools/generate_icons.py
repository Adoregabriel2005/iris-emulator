"""
Gera iris_emulator.ico usando Qt (PySide6) - sem dependencia de Cairo.
Requer: pip install PySide6 pillow
"""
import sys
from pathlib import Path

try:
    from PySide6.QtWidgets import QApplication
    from PySide6.QtSvg import QSvgRenderer
    from PySide6.QtGui import QImage, QPainter, QColor
    from PIL import Image
except ImportError:
    print("Instale: pip install PySide6 pillow")
    sys.exit(1)

BASE        = Path(__file__).parent.parent / "src" / "resources"
SVG_ICON    = BASE / "iris_icon.svg"
SVG_LOGO    = BASE / "iris_logo.svg"
OUT_ICO     = BASE / "iris_emulator.ico"
OUT_PNG     = BASE / "iris_logo_512.png"
OUT_DISCORD = BASE / "iris_discord_512.png"

SIZES = [16, 24, 32, 48, 64, 128, 256]


def render_svg(svg_path: Path, size: int) -> Image.Image:
    renderer = QSvgRenderer(str(svg_path))
    img = QImage(size, size, QImage.Format.Format_ARGB32)
    img.fill(QColor(0, 0, 0, 0))
    painter = QPainter(img)
    renderer.render(painter)
    painter.end()
    tmp = BASE / f"_tmp_{size}.png"
    img.save(str(tmp))
    pil = Image.open(str(tmp)).convert("RGBA")
    tmp.unlink()
    return pil


def main():
    app = QApplication(sys.argv)

    print(f"Renderizando icone: {SVG_ICON}")
    frames = []
    for s in SIZES:
        pil = render_svg(SVG_ICON, s)
        frames.append(pil)
        print(f"  {s}x{s} OK")

    frames[0].save(
        str(OUT_ICO),
        format="ICO",
        sizes=[(s, s) for s in SIZES],
        append_images=frames[1:]
    )
    print(f"Salvo: {OUT_ICO}")

    print(f"Renderizando logo 512px: {SVG_LOGO}")
    big = render_svg(SVG_LOGO, 512)
    big.save(str(OUT_PNG), "PNG")
    print(f"Salvo: {OUT_PNG}")

    discord_img = Image.new("RGBA", (512, 512), (13, 13, 26, 255))
    icon_512 = render_svg(SVG_ICON, 512)
    discord_img.paste(icon_512, (0, 0), icon_512)
    discord_img.save(str(OUT_DISCORD), "PNG")
    print(f"Salvo: {OUT_DISCORD}")

    print()
    print("=== PROXIMOS PASSOS ===")
    print(f"1. Use {OUT_ICO} como icone do exe no CMakeLists.txt")
    print(f"2. Faca upload de {OUT_DISCORD} no Discord Developer Portal:")
    print("   Rich Presence -> Art Assets -> Add Image -> nome: iris_logo")


if __name__ == "__main__":
    main()
