from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


CELL_W = 24
CELL_H = 16
COLUMNS = 8
SCALE = 4
IDS = [
    "U", "D", "F", "B", "UF", "UB", "DF", "DB",
    "LP", "MP", "SP", "LK", "MK", "SK", "P", "K",
    "SQ", "TRI", "L1", "X", "O", "R1", "A", "B",
    "Y", "LB", "RB", "START", "MENU", "OPT", "HOLD", "MASH",
    "+", "/", "..", "-", "L3", "R3", "TP", "SEL",
]


def load_font(size: int) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    for name in ("DejaVuSansMono-Bold.ttf", "arialbd.ttf", "Arial Bold.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default()


def text_fit_font(label: str, max_w: int, max_h: int) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    for size in range(10 * SCALE, 4 * SCALE, -1):
        font = load_font(size)
        bbox = ImageDraw.Draw(Image.new("RGBA", (1, 1))).textbbox((0, 0), label, font=font)
        if bbox[2] - bbox[0] <= max_w and bbox[3] - bbox[1] <= max_h:
            return font
    return load_font(5 * SCALE)


def draw_centered_text(draw: ImageDraw.ImageDraw, box: tuple[int, int, int, int], label: str) -> None:
    x0, y0, x1, y1 = box
    font = text_fit_font(label, x1 - x0 - 2 * SCALE, y1 - y0 - 2 * SCALE)
    bbox = draw.textbbox((0, 0), label, font=font)
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]
    x = x0 + (x1 - x0 - w) // 2 - bbox[0]
    y = y0 + (y1 - y0 - h) // 2 - bbox[1]
    draw.text((x, y), label, fill=(245, 248, 255, 255), font=font)


def cell_box(index: int) -> tuple[int, int, int, int]:
    col = index % COLUMNS
    row = index // COLUMNS
    x = col * CELL_W * SCALE
    y = row * CELL_H * SCALE
    return x, y, x + CELL_W * SCALE, y + CELL_H * SCALE


def draw_cardinal_arrow(draw: ImageDraw.ImageDraw, box: tuple[int, int, int, int], icon_id: str) -> None:
    x0, y0, x1, y1 = box
    cx = (x0 + x1) // 2
    cy = (y0 + y1) // 2
    stem = 4 * SCALE
    head = 6 * SCALE
    half = 5 * SCALE
    color = (245, 248, 255, 255)
    if icon_id == "U":
        points = [(cx, cy - head), (cx - half, cy - SCALE), (cx - stem // 2, cy - SCALE),
                  (cx - stem // 2, cy + head), (cx + stem // 2, cy + head),
                  (cx + stem // 2, cy - SCALE), (cx + half, cy - SCALE)]
    elif icon_id == "D":
        points = [(cx, cy + head), (cx - half, cy + SCALE), (cx - stem // 2, cy + SCALE),
                  (cx - stem // 2, cy - head), (cx + stem // 2, cy - head),
                  (cx + stem // 2, cy + SCALE), (cx + half, cy + SCALE)]
    elif icon_id == "F":
        points = [(cx + head, cy), (cx + SCALE, cy - half), (cx + SCALE, cy - stem // 2),
                  (cx - head, cy - stem // 2), (cx - head, cy + stem // 2),
                  (cx + SCALE, cy + stem // 2), (cx + SCALE, cy + half)]
    else:
        points = [(cx - head, cy), (cx - SCALE, cy - half), (cx - SCALE, cy - stem // 2),
                  (cx + head, cy - stem // 2), (cx + head, cy + stem // 2),
                  (cx - SCALE, cy + stem // 2), (cx - SCALE, cy + half)]
    draw.polygon(points, fill=color)


def draw_diagonal_arrow(draw: ImageDraw.ImageDraw, box: tuple[int, int, int, int], icon_id: str) -> None:
    x0, y0, x1, y1 = box
    pad_x = 7 * SCALE
    pad_y = 4 * SCALE
    color = (245, 248, 255, 255)
    width = 3 * SCALE
    if icon_id == "UF":
        start, end = (x0 + pad_x, y1 - pad_y), (x1 - pad_x, y0 + pad_y)
        head = [(end[0], end[1]), (end[0] - 5 * SCALE, end[1]), (end[0], end[1] + 5 * SCALE)]
    elif icon_id == "UB":
        start, end = (x1 - pad_x, y1 - pad_y), (x0 + pad_x, y0 + pad_y)
        head = [(end[0], end[1]), (end[0] + 5 * SCALE, end[1]), (end[0], end[1] + 5 * SCALE)]
    elif icon_id == "DF":
        start, end = (x0 + pad_x, y0 + pad_y), (x1 - pad_x, y1 - pad_y)
        head = [(end[0], end[1]), (end[0] - 5 * SCALE, end[1]), (end[0], end[1] - 5 * SCALE)]
    else:
        start, end = (x1 - pad_x, y0 + pad_y), (x0 + pad_x, y1 - pad_y)
        head = [(end[0], end[1]), (end[0] + 5 * SCALE, end[1]), (end[0], end[1] - 5 * SCALE)]
    draw.line([start, end], fill=color, width=width)
    draw.polygon(head, fill=color)


def draw_shape_button(draw: ImageDraw.ImageDraw, box: tuple[int, int, int, int], icon_id: str) -> None:
    x0, y0, x1, y1 = box
    cx = (x0 + x1) // 2
    cy = (y0 + y1) // 2
    color = (245, 248, 255, 255)
    width = 2 * SCALE
    r = 5 * SCALE
    if icon_id == "SQ":
        draw.rounded_rectangle((cx - r, cy - r, cx + r, cy + r), radius=SCALE, outline=color, width=width)
    elif icon_id == "TRI":
        points = [(cx, cy - r - SCALE), (cx - r - SCALE, cy + r), (cx + r + SCALE, cy + r), (cx, cy - r - SCALE)]
        draw.line(points, fill=color, width=width, joint="curve")
    elif icon_id == "O":
        draw.ellipse((cx - r, cy - r, cx + r, cy + r), outline=color, width=width)
    elif icon_id == "X":
        draw.line((cx - r, cy - r, cx + r, cy + r), fill=color, width=width)
        draw.line((cx + r, cy - r, cx - r, cy + r), fill=color, width=width)
    else:
        draw_centered_text(draw, box, icon_id)


def draw_operator(draw: ImageDraw.ImageDraw, box: tuple[int, int, int, int], icon_id: str) -> None:
    x0, y0, x1, y1 = box
    cx = (x0 + x1) // 2
    cy = (y0 + y1) // 2
    color = (245, 248, 255, 255)
    width = 2 * SCALE
    if icon_id == "+":
        draw.line((cx - 5 * SCALE, cy, cx + 5 * SCALE, cy), fill=color, width=width)
        draw.line((cx, cy - 5 * SCALE, cx, cy + 5 * SCALE), fill=color, width=width)
    elif icon_id == "/":
        draw.line((cx - 5 * SCALE, cy + 5 * SCALE, cx + 5 * SCALE, cy - 5 * SCALE), fill=color, width=width)
    elif icon_id == "..":
        draw.ellipse((cx - 5 * SCALE, cy - SCALE, cx - 3 * SCALE, cy + SCALE), fill=color)
        draw.ellipse((cx + 3 * SCALE, cy - SCALE, cx + 5 * SCALE, cy + SCALE), fill=color)
    elif icon_id == "-":
        draw.line((cx - 5 * SCALE, cy, cx + 5 * SCALE, cy), fill=color, width=width)
    else:
        draw_centered_text(draw, box, icon_id)


def draw_icon(draw: ImageDraw.ImageDraw, box: tuple[int, int, int, int], icon_id: str) -> None:
    if icon_id in {"U", "D", "F", "B"}:
        draw_cardinal_arrow(draw, box, icon_id)
    elif icon_id in {"UF", "UB", "DF", "DB"}:
        draw_diagonal_arrow(draw, box, icon_id)
    elif icon_id in {"SQ", "TRI", "X", "O"}:
        draw_shape_button(draw, box, icon_id)
    elif icon_id in {"+", "/", "..", "-"}:
        draw_operator(draw, box, icon_id)
    else:
        label = {"START": "ST", "MENU": "MN", "HOLD": "HLD", "MASH": "MSH"}.get(icon_id, icon_id)
        draw_centered_text(draw, box, label)


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    ui_dir = root / "game" / "data" / "ui"
    out = ui_dir / "command_input_icons.png"
    rows = (len(IDS) + COLUMNS - 1) // COLUMNS
    image = Image.new("RGBA", (COLUMNS * CELL_W * SCALE, rows * CELL_H * SCALE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    for index, icon_id in enumerate(IDS):
        draw_icon(draw, cell_box(index), icon_id)
    resample = getattr(Image, "Resampling", Image).LANCZOS
    image = image.resize((COLUMNS * CELL_W, rows * CELL_H), resample)
    ui_dir.mkdir(parents=True, exist_ok=True)
    image.save(out)
    print(out)

    check_out = ui_dir / "command_complete_check.png"
    check = Image.new("RGBA", (48 * SCALE, 48 * SCALE), (0, 0, 0, 0))
    check_draw = ImageDraw.Draw(check)
    cx = 24 * SCALE
    cy = 24 * SCALE
    check_draw.ellipse(
        (cx - 20 * SCALE, cy - 20 * SCALE, cx + 20 * SCALE, cy + 20 * SCALE),
        fill=(18, 42, 34, 220),
        outline=(128, 255, 190, 230),
        width=2 * SCALE,
    )
    check_draw.ellipse(
        (cx - 15 * SCALE, cy - 15 * SCALE, cx + 15 * SCALE, cy + 15 * SCALE),
        outline=(255, 223, 128, 150),
        width=SCALE,
    )
    # Drawn at high resolution and downsampled so the final PNG keeps a clean pixel-art edge.
    stroke = 7 * SCALE
    shadow = 10 * SCALE
    points = [
        (13 * SCALE, 25 * SCALE),
        (21 * SCALE, 33 * SCALE),
        (36 * SCALE, 15 * SCALE),
    ]
    check_draw.line(points, fill=(4, 10, 8, 160), width=shadow, joint="curve")
    check_draw.line(points, fill=(255, 240, 146, 255), width=stroke + SCALE, joint="curve")
    check_draw.line(points, fill=(130, 255, 190, 255), width=stroke, joint="curve")
    check_draw.line(points, fill=(240, 255, 248, 255), width=2 * SCALE, joint="curve")
    check = check.resize((48, 48), resample)
    check.save(check_out)
    print(check_out)


if __name__ == "__main__":
    main()
