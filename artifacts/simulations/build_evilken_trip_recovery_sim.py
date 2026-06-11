from pathlib import Path
import html
import math
import re
import struct

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(r"C:\Users\kasom\projects\dragon-mugen-arena")
CHAR_DIR = ROOT / "game" / "chars" / "EvilKen"
SFF_PATH = CHAR_DIR / "evilken.sff"
AIR_PATH = CHAR_DIR / "evilken.air"
ACT_PATH = CHAR_DIR / "Default.act"
OUT_DIR = ROOT / "artifacts" / "simulations"
OUT_DIR.mkdir(parents=True, exist_ok=True)


def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def i16(data, offset):
    return struct.unpack_from("<h", data, offset)[0]


def read_sff(path):
    data = path.read_bytes()
    if len(data) < 512 or data[:12] != b"ElecbyteSpr\0":
        raise ValueError(f"Not an SFF v1 archive: {path}")
    count = u32(data, 20)
    offset = u32(data, 24)
    subheader_size = u32(data, 28)
    sprites = []
    for index in range(count):
        if offset == 0 or offset + 32 > len(data):
            break
        next_offset = u32(data, offset)
        sprite = {
            "index": index,
            "data_length": u32(data, offset + 4),
            "axis_x": i16(data, offset + 8),
            "axis_y": i16(data, offset + 10),
            "group": i16(data, offset + 12),
            "image": i16(data, offset + 14),
            "linked_index": i16(data, offset + 16),
            "data_offset": offset + subheader_size,
        }
        sprites.append(sprite)
        offset = next_offset
    return data, sprites, {(s["group"], s["image"]): s for s in sprites}


def reversed_act_palette(path):
    raw = path.read_bytes()[:768]
    if len(raw) < 768:
        raise ValueError(f"Palette too small: {path}")
    palette = bytearray()
    for index in range(256):
        source = (255 - index) * 3
        palette.extend(raw[source:source + 3])
    return bytes(palette)


def decode_pcx_indices(blob):
    if len(blob) < 128 or blob[0] != 0x0A or blob[2] != 1 or blob[3] != 8:
        return None
    width = u16(blob, 8) - u16(blob, 4) + 1
    height = u16(blob, 10) - u16(blob, 6) + 1
    planes = blob[65]
    bytes_per_line = u16(blob, 66)
    if width <= 0 or height <= 0 or planes != 1 or bytes_per_line <= 0:
        return None

    palette_marker = len(blob) - 769 if len(blob) >= 769 and blob[-769] == 0x0C else len(blob)
    decoded = bytearray(bytes_per_line * height)
    read = 128
    write = 0
    while read < palette_marker and write < len(decoded):
        value = blob[read]
        read += 1
        if (value & 0xC0) == 0xC0 and read < palette_marker:
            count = value & 0x3F
            run_value = blob[read]
            read += 1
            for _ in range(count):
                if write >= len(decoded):
                    break
                decoded[write] = run_value
                write += 1
        else:
            decoded[write] = value
            write += 1

    indices = bytearray(width * height)
    for y in range(height):
        start = y * bytes_per_line
        indices[y * width:(y + 1) * width] = decoded[start:start + width]
    return width, height, indices


def sprite_image(data, sprites, sprite, palette):
    source = sprite
    if source["data_length"] == 0 and 0 <= source["linked_index"] < len(sprites):
        source = sprites[source["linked_index"]]
    if source["data_length"] <= 0:
        return None
    start = source["data_offset"]
    end = start + source["data_length"]
    if end > len(data):
        return None
    decoded = decode_pcx_indices(data[start:end])
    if decoded is None:
        return None
    width, height, indices = decoded
    image = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    pixels = image.load()
    for y in range(height):
        for x in range(width):
            color = indices[y * width + x]
            if color == 0:
                continue
            palette_index = color * 3
            pixels[x, y] = (
                palette[palette_index],
                palette[palette_index + 1],
                palette[palette_index + 2],
                255,
            )
    return image


def parse_air(path):
    actions = {}
    current = None
    begin_re = re.compile(r"^\s*\[\s*Begin\s+Action\s+(-?\d+)\s*\]", re.I)
    frame_re = re.compile(r"^\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)")
    for raw in path.read_text(errors="replace").splitlines():
        line = raw.split(";", 1)[0].strip()
        begin = begin_re.match(line)
        if begin:
            current = int(begin.group(1))
            actions[current] = {"frames": [], "loop_index": None}
            continue
        if current is None or not line:
            continue
        if line.lower().startswith("loopstart"):
            actions[current]["loop_index"] = len(actions[current]["frames"])
            continue
        if line.lower().startswith("clsn"):
            continue
        frame = frame_re.match(line)
        if frame:
            group, image, x, y, ticks = map(int, frame.groups())
            if group >= 0 and image >= 0:
                actions[current]["frames"].append(
                    {"group": group, "image": image, "x": x, "y": y, "ticks": ticks}
                )
    return actions


def action_frame_at(actions, action, tick):
    action_data = actions[action]
    frames = action_data["frames"]
    if not frames:
        raise ValueError(f"Action {action} has no drawable frames")

    elapsed = 0
    for frame in frames:
        duration = frame["ticks"]
        if duration < 0:
            return frame
        if tick < elapsed + duration:
            return frame
        elapsed += duration

    loop_index = action_data["loop_index"]
    if loop_index is None or loop_index >= len(frames):
        return frames[-1]

    loop_frames = frames[loop_index:]
    loop_ticks = sum(max(1, f["ticks"]) for f in loop_frames)
    loop_tick = (tick - elapsed) % max(1, loop_ticks)
    elapsed = 0
    for frame in loop_frames:
        duration = max(1, frame["ticks"])
        if loop_tick < elapsed + duration:
            return frame
        elapsed += duration
    return loop_frames[-1]


def load_font(size):
    for name in ("consola.ttf", "arial.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default()


def fall_arc(initial_vx, initial_vy, gravity, start_x=0.0, start_y=0.0):
    x = start_x
    y = start_y
    vx = initial_vx
    vy = initial_vy
    samples = []
    while True:
        x += vx
        y += vy
        samples.append((x, y, vx, vy))
        vy += gravity
        if y >= 0.0 and vy > 0.0:
            samples[-1] = (x, 0.0, vx, vy)
            break
        if len(samples) > 90:
            break
    return samples


def build_timeline(initial_vx=1.3, bounce_vxs=(0.78, 0.494), label="engine-custom"):
    timeline = []

    def add_hold(action, ticks, phase, x=0.0, y=0.0):
        for tick in range(ticks):
            timeline.append({"action": action, "tick": tick, "phase": phase, "x": x, "y": y, "variant": label})

    add_hold(5070, 8, "5070 trip shake")
    last_x = 0.0
    for tick, (x, y, vx, vy) in enumerate(fall_arc(initial_vx, -3.0, 0.42)):
        last_x = x
        timeline.append(
            {
                "action": 5050,
                "tick": tick,
                "phase": "5050 fallback fall arc",
                "x": x,
                "y": y,
                "vx": vx,
                "vy": vy,
                "variant": label,
            }
        )
    add_hold(5100, 3, "5100 first floor hit", x=last_x)

    for bounce_index, vy in enumerate((-0.9, -0.55), start=1):
        bounce_vx = bounce_vxs[bounce_index - 1] if bounce_index - 1 < len(bounce_vxs) else 0.0
        for tick, (x, y, vx, frame_vy) in enumerate(fall_arc(bounce_vx, vy, 0.42, start_x=last_x)):
            last_x = x
            timeline.append(
                {
                    "action": 5160,
                    "tick": tick,
                    "phase": f"5160 floor bounce {bounce_index}",
                    "x": x,
                    "y": y,
                    "vx": vx,
                    "vy": frame_vy,
                    "variant": label,
                }
            )
        add_hold(5170, 3, f"5170 floor hit after bounce {bounce_index}", x=last_x)

    add_hold(5110, 24, "5110 lie down", x=last_x)
    add_hold(5120, 20, "5120 get up", x=last_x)
    add_hold(5030, 10, "standing-ready hold", x=last_x)
    return timeline


def draw_frame(frame_info, image_index, data, sprites, sprite_by_key, palette, actions, trail):
    width, height = 960, 520
    floor_y = 388
    origin_x = 430
    scale = 2

    frame = Image.new("RGBA", (width, height), (20, 22, 28, 255))
    draw = ImageDraw.Draw(frame)
    small = load_font(14)
    large = load_font(20)

    draw.rectangle([0, floor_y, width, height], fill=(34, 38, 44, 255))
    for x in range(0, width, 40):
        draw.line([x, floor_y, x + 24, height], fill=(46, 52, 61, 255), width=1)
    draw.line([0, floor_y, width, floor_y], fill=(226, 196, 101, 255), width=3)
    title = "Evil Ken Trip / Fall / Floor Bounce / Get-Up Simulation"
    if frame_info["variant"] == "cns-reference":
        title += " - CNS-only reference"
    draw.text((18, 14), title, font=large, fill=(245, 235, 176, 255))
    draw.text((18, 42), frame_info["phase"], font=small, fill=(196, 211, 234, 255))
    draw.text(
        (18, 64),
        f"action {frame_info['action']}  tick {frame_info['tick']}  x {frame_info['x']:.2f}  y {frame_info['y']:.2f}",
        font=small,
        fill=(184, 194, 211, 255),
    )
    if frame_info["variant"] == "engine-custom":
        draw.text(
            (18, 86),
            "Engine custom trip: low launch, away drift, two reduced floor bounces.",
            font=small,
            fill=(184, 194, 211, 255),
        )
    else:
        draw.text(
            (18, 86),
            "CNS-only state 420 reference: ground.velocity = 0,-3 and no fall.xvelocity.",
            font=small,
            fill=(184, 194, 211, 255),
        )

    attacker_x = origin_x - 178
    draw.rectangle([attacker_x - 18, floor_y - 72, attacker_x + 18, floor_y], outline=(214, 93, 93, 255), width=2)
    draw.text((attacker_x - 34, floor_y + 8), "attacker", font=small, fill=(214, 93, 93, 255))
    draw.line([attacker_x + 42, floor_y - 36, origin_x - 20, floor_y - 36], fill=(214, 93, 93, 190), width=2)

    if len(trail) > 1:
        points = [(origin_x + x * scale, floor_y + y * scale) for x, y in trail[-60:]]
        draw.line(points, fill=(92, 171, 241, 180), width=2)
        for point in points[::4]:
            draw.ellipse([point[0] - 2, point[1] - 2, point[0] + 2, point[1] + 2], fill=(92, 171, 241, 180))

    air_frame = action_frame_at(actions, frame_info["action"], frame_info["tick"])
    sprite = sprite_by_key.get((air_frame["group"], air_frame["image"]))
    sprite_rgba = sprite_image(data, sprites, sprite, palette)
    if sprite_rgba is None:
        raise ValueError(f"Missing sprite {air_frame['group']},{air_frame['image']}")
    sprite_rgba = sprite_rgba.resize((sprite_rgba.width * scale, sprite_rgba.height * scale), Image.Resampling.NEAREST)

    x = origin_x + frame_info["x"] * scale + (air_frame["x"] - sprite["axis_x"]) * scale
    y = floor_y + frame_info["y"] * scale + (air_frame["y"] - sprite["axis_y"]) * scale
    frame.alpha_composite(sprite_rgba, (round(x), round(y)))

    axis_x = origin_x + frame_info["x"] * scale + air_frame["x"] * scale
    axis_y = floor_y + frame_info["y"] * scale + air_frame["y"] * scale
    draw.line([axis_x - 8, axis_y, axis_x + 8, axis_y], fill=(255, 95, 95, 220), width=1)
    draw.line([axis_x, axis_y - 8, axis_x, axis_y + 8], fill=(255, 95, 95, 220), width=1)

    draw.text(
        (18, height - 32),
        f"sprite {air_frame['group']},{air_frame['image']}  offset {air_frame['x']},{air_frame['y']}  source AIR/SFF/ACT",
        font=small,
        fill=(224, 229, 239, 255),
    )
    draw.text((width - 102, height - 32), f"frame {image_index:03}", font=small, fill=(224, 229, 239, 255))
    return frame.convert("P", palette=Image.Palette.ADAPTIVE, colors=255)


def make_keyframe_sheet(frames, timeline):
    chosen = []
    target_phases = []
    for item in timeline:
        if item["phase"] not in target_phases:
            target_phases.append(item["phase"])
    for phase in target_phases:
        index = next(i for i, item in enumerate(timeline) if item["phase"] == phase)
        chosen.append((index, phase, frames[index].convert("RGBA")))

    cell_w, cell_h = 320, 220
    sheet = Image.new("RGBA", (cell_w * 3, cell_h * math.ceil(len(chosen) / 3)), (20, 22, 28, 255))
    draw = ImageDraw.Draw(sheet)
    font = load_font(13)
    for index, (frame_index, phase, frame) in enumerate(chosen):
        col = index % 3
        row = index // 3
        x = col * cell_w
        y = row * cell_h
        draw.rectangle([x, y, x + cell_w - 1, y + cell_h - 1], outline=(69, 78, 94, 255))
        thumb = frame.copy()
        thumb.thumbnail((cell_w - 16, cell_h - 48), Image.Resampling.BICUBIC)
        sheet.alpha_composite(thumb, (x + 8, y + 8))
        draw.text((x + 8, y + cell_h - 34), phase, font=font, fill=(245, 235, 176, 255))
        draw.text((x + 8, y + cell_h - 18), f"simulation frame {frame_index}", font=font, fill=(196, 211, 234, 255))
    return sheet


def write_html(gif_name, keyframe_name, cns_gif_name, cns_keyframe_name, timeline, cns_timeline):
    rows = "\n".join(
        f"<tr><td>{index}</td><td>{html.escape(item['phase'])}</td><td>{item['action']}</td><td>{item['tick']}</td><td>{item['x']:.2f}</td><td>{item['y']:.2f}</td></tr>"
        for index, item in enumerate(timeline)
    )
    cns_rows = "\n".join(
        f"<tr><td>{index}</td><td>{html.escape(item['phase'])}</td><td>{item['action']}</td><td>{item['tick']}</td><td>{item['x']:.2f}</td><td>{item['y']:.2f}</td></tr>"
        for index, item in enumerate(cns_timeline)
    )
    body = f"""<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>Evil Ken Trip Recovery Simulation</title>
<style>
body {{ background: #14161c; color: #e7ebf3; font-family: Consolas, monospace; margin: 24px; }}
h1 {{ color: #f4e8a5; font-size: 22px; }}
img {{ image-rendering: pixelated; border: 1px solid #4e596b; max-width: 100%; }}
.grid {{ display: grid; grid-template-columns: minmax(360px, 720px); gap: 18px; }}
table {{ border-collapse: collapse; font-size: 12px; margin-top: 18px; }}
td, th {{ border: 1px solid #3f4858; padding: 4px 8px; }}
th {{ color: #f4e8a5; }}
</style>
</head>
<body>
<h1>Evil Ken Trip / Fall / Floor Bounce / Get-Up Simulation</h1>
<div class="grid">
<h2>Engine custom trip response</h2>
<img src="{gif_name}" alt="Evil Ken custom trip recovery animated simulation">
<img src="{keyframe_name}" alt="Custom trip keyframes">
<h2>CNS-only state 420 reference</h2>
<img src="{cns_gif_name}" alt="Evil Ken CNS-only trip reference animated simulation">
<img src="{cns_keyframe_name}" alt="CNS-only keyframes">
</div>
<p>Sequence: 5070 trip shake, 5050 fallback fall arc, 5100 floor hit, two 5160/5170 floor bounces, 5110 lie-down, 5120 get-up.</p>
<p>The custom response intentionally adds away drift even when Evil Ken state 420 only declares ground.velocity = 0,-3.</p>
<table>
<thead><tr><th colspan="6">Engine custom timeline</th></tr><tr><th>Frame</th><th>Phase</th><th>Action</th><th>Action Tick</th><th>X</th><th>Y</th></tr></thead>
<tbody>
{rows}
</tbody>
</table>
<table>
<thead><tr><th colspan="6">CNS-only reference timeline</th></tr><tr><th>Frame</th><th>Phase</th><th>Action</th><th>Action Tick</th><th>X</th><th>Y</th></tr></thead>
<tbody>
{cns_rows}
</tbody>
</table>
</body>
</html>
"""
    return body


def main():
    data, sprites, sprite_by_key = read_sff(SFF_PATH)
    palette = reversed_act_palette(ACT_PATH)
    actions = parse_air(AIR_PATH)
    timeline = build_timeline()
    cns_timeline = build_timeline(initial_vx=0.0, bounce_vxs=(0.0, 0.0), label="cns-reference")
    trail = []
    frames = []
    for index, item in enumerate(timeline):
        trail.append((item["x"], item["y"]))
        frames.append(draw_frame(item, index, data, sprites, sprite_by_key, palette, actions, trail))
    cns_trail = []
    cns_frames = []
    for index, item in enumerate(cns_timeline):
        cns_trail.append((item["x"], item["y"]))
        cns_frames.append(draw_frame(item, index, data, sprites, sprite_by_key, palette, actions, cns_trail))

    gif_path = OUT_DIR / "evilken_trip_fall_bounce_getup.gif"
    frames[0].save(
        gif_path,
        save_all=True,
        append_images=frames[1:],
        duration=33,
        loop=0,
        disposal=2,
    )
    cns_gif_path = OUT_DIR / "evilken_trip_fall_bounce_getup_cns_reference.gif"
    cns_frames[0].save(
        cns_gif_path,
        save_all=True,
        append_images=cns_frames[1:],
        duration=33,
        loop=0,
        disposal=2,
    )

    keyframe_path = OUT_DIR / "evilken_trip_fall_bounce_getup_keyframes.png"
    make_keyframe_sheet(frames, timeline).save(keyframe_path)
    cns_keyframe_path = OUT_DIR / "evilken_trip_fall_bounce_getup_cns_reference_keyframes.png"
    make_keyframe_sheet(cns_frames, cns_timeline).save(cns_keyframe_path)

    html_path = OUT_DIR / "evilken_trip_fall_bounce_getup.html"
    html_path.write_text(
        write_html(gif_path.name, keyframe_path.name, cns_gif_path.name, cns_keyframe_path.name, timeline, cns_timeline),
        encoding="utf-8",
    )

    print(gif_path)
    print(keyframe_path)
    print(cns_gif_path)
    print(cns_keyframe_path)
    print(html_path)
    print(f"frames={len(frames)} cns_frames={len(cns_frames)}")


if __name__ == "__main__":
    main()
