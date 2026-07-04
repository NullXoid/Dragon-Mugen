from __future__ import annotations

import argparse
import html
import json
import mimetypes
import os
import posixpath
import sys
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, quote, unquote, urlparse


DEFAULT_REPO_ROOT = Path(r"C:\Users\kasom\projects\dragon-mugen-arena")
CHARACTERS = ("A.Ben", "I.Chie")
ACTIONS = ("idle", "walk", "jump", "punch", "kick", "dash")
MEDIA_SUFFIXES = {".png", ".gif", ".mp4", ".jpg", ".jpeg", ".webp"}


def find_repo_root() -> Path:
    local_root = Path(__file__).resolve().parents[2]
    if (local_root / "game" / "chars").exists():
        return local_root
    return DEFAULT_REPO_ROOT


def load_json(path: Path) -> dict:
    if not path.exists():
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}


def is_relative_to(path: Path, root: Path) -> bool:
    try:
        path.resolve().relative_to(root.resolve())
        return True
    except ValueError:
        return False


def relpath(path: Path, root: Path) -> str:
    return path.resolve().relative_to(root.resolve()).as_posix()


def localize_manifest_path(raw_path: object, fallback_dir: Path, root: Path) -> Path | None:
    if not raw_path:
        return None
    candidate = Path(str(raw_path))
    if candidate.exists() and is_relative_to(candidate, root):
        return candidate
    fallback = fallback_dir / candidate.name
    if fallback.exists() and is_relative_to(fallback, root):
        return fallback
    return None


def first_existing(paths: list[Path]) -> Path | None:
    for path in paths:
        if path.exists():
            return path
    return None


def media_url(path: Path | None, root: Path) -> str:
    if not path:
        return ""
    return "/asset/" + quote(relpath(path, root))


def command_text(root: Path) -> list[tuple[str, str]]:
    return [
        (
            "Prepare LTX run",
            "python engine/tools/ltx_sprite_pipeline.py prepare "
            "--character A.Ben --action walk --video game/chars/A.Ben/source_videos/walk_LTX-2_00068_.mp4",
        ),
        (
            "Promote selected frames",
            "python engine/tools/ltx_sprite_pipeline.py promote "
            "--run-dir game/chars/A.Ben/source_art/ltx_runs/<run-name> --selected 0,6,12",
        ),
        ("Build A.Ben walk SFF", "python engine/tools/build_aben_walk_sff.py"),
        ("Roster compatibility proof", "build/dragon_mugen.exe --verify roster-compatibility-smoke"),
    ]


def character_summary(root: Path, character: str) -> dict:
    char_dir = root / "game" / "chars" / character
    source_art = char_dir / "source_art"
    source_videos = char_dir / "source_videos"
    curated = source_art / "curated_game_sprites"
    contacts = curated / "contacts"
    previews = curated / "previews"
    frames = curated / "frames"
    shop = char_dir / "shop"

    curated_manifest_path = curated / "manifest.json"
    video_manifest_path = source_videos / "manifest.json"
    curated_manifest = load_json(curated_manifest_path)
    video_manifest = load_json(video_manifest_path)
    curated_actions = curated_manifest.get("actions", {}) if isinstance(curated_manifest.get("actions"), dict) else {}
    video_actions = video_manifest.get("videos", {}) if isinstance(video_manifest.get("videos"), dict) else {}

    action_rows = []
    for action in ACTIONS:
        manifest_action = curated_actions.get(action, {}) if isinstance(curated_actions.get(action), dict) else {}
        frame_dir = frames / action
        frame_files = sorted(path for path in frame_dir.glob("*.png")) if frame_dir.exists() else []
        selected = manifest_action.get("selected_source_frames")
        promoted = manifest_action.get("promoted_frames")
        frame_count = len(frame_files)
        if frame_count == 0 and isinstance(promoted, list):
            frame_count = len(promoted)
        if frame_count == 0 and isinstance(selected, list):
            frame_count = len(selected)

        contact = localize_manifest_path(manifest_action.get("contact"), contacts, root)
        if not contact:
            contact = first_existing([contacts / f"{action}_selected_contact.png"])
        preview = localize_manifest_path(manifest_action.get("gif_preview") or manifest_action.get("preview"), previews, root)
        if not preview:
            preview = first_existing([previews / f"{action}_selected_preview.gif"])

        video_info = video_actions.get(action, {}) if isinstance(video_actions.get(action), dict) else {}
        video_file = source_videos / str(video_info.get("file", ""))
        source_video = video_file if video_file.exists() else first_existing(sorted(source_videos.glob(f"{action}*.mp4")))

        action_rows.append(
            {
                "name": action,
                "frame_count": frame_count,
                "contact": contact,
                "preview": preview,
                "source_video": source_video,
                "manifest": manifest_action,
            }
        )

    workspace_dirs = [
        ("source_art", source_art),
        ("source_videos", source_videos),
        ("shop", shop),
        ("curated_game_sprites", curated),
    ]
    return {
        "character": character,
        "char_dir": char_dir,
        "exists": char_dir.exists(),
        "workspace_dirs": workspace_dirs,
        "curated_manifest_path": curated_manifest_path if curated_manifest_path.exists() else None,
        "video_manifest_path": video_manifest_path if video_manifest_path.exists() else None,
        "curated_manifest": curated_manifest,
        "action_rows": action_rows,
    }


def render_page(root: Path, selected_character: str) -> bytes:
    if selected_character not in CHARACTERS:
        selected_character = CHARACTERS[0]
    summary = character_summary(root, selected_character)
    manifest_actions = summary["curated_manifest"].get("actions", {})

    char_tabs = "".join(
        f'<a class="tab {"active" if char == selected_character else ""}" href="/?char={quote(char)}">{html.escape(char)}</a>'
        for char in CHARACTERS
    )

    workspace_items = []
    for label, path in summary["workspace_dirs"]:
        present = path.exists()
        status = "present" if present else "missing"
        workspace_items.append(
            "<li>"
            f"<span>{html.escape(label)}</span>"
            f"<code>{html.escape(relpath(path, root) if is_relative_to(path, root) else str(path))}</code>"
            f'<b class="{status}">{status}</b>'
            "</li>"
        )

    action_cards = []
    for row in summary["action_rows"]:
        contact_url = media_url(row["contact"], root)
        preview_url = media_url(row["preview"], root)
        video_url = media_url(row["source_video"], root)
        contact_path = relpath(row["contact"], root) if row["contact"] else "not found"
        preview_path = relpath(row["preview"], root) if row["preview"] else "not found"
        video_status = "yes" if row["source_video"] else "no"
        media = ""
        if preview_url:
            media = f'<img src="{preview_url}" alt="{html.escape(row["name"])} preview">'
        elif contact_url:
            media = f'<img src="{contact_url}" alt="{html.escape(row["name"])} contact sheet">'
        else:
            media = '<div class="empty-media">No preview</div>'
        video_link = f'<a href="{video_url}" target="_blank">open video</a>' if video_url else "<span>none</span>"
        contact_link = f'<a href="{contact_url}" target="_blank">{html.escape(contact_path)}</a>' if contact_url else html.escape(contact_path)
        preview_link = f'<a href="{preview_url}" target="_blank">{html.escape(preview_path)}</a>' if preview_url else html.escape(preview_path)

        action_cards.append(
            '<article class="action-card">'
            f'<header><h3>{html.escape(row["name"])}</h3><span>{row["frame_count"]} frames</span></header>'
            f'<div class="media">{media}</div>'
            '<dl>'
            f"<dt>contact</dt><dd>{contact_link}</dd>"
            f"<dt>preview</dt><dd>{preview_link}</dd>"
            f"<dt>source video</dt><dd>{html.escape(video_status)} · {video_link}</dd>"
            "</dl>"
            "</article>"
        )

    selected_manifest = json.dumps(manifest_actions, indent=2, sort_keys=True) if manifest_actions else "{}"
    commands = "".join(
        "<section class='command'>"
        f"<h3>{html.escape(label)}</h3>"
        f"<pre><code>{html.escape(command)}</code></pre>"
        "</section>"
        for label, command in command_text(root)
    )

    body = f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Dragon Asset Lab</title>
  <style>
    :root {{
      color-scheme: dark;
      --bg: #120f12;
      --panel: #1c1718;
      --panel-2: #251c1a;
      --line: #4b3329;
      --text: #f4e7d3;
      --muted: #b99b82;
      --accent: #e5552f;
      --gold: #f3b33d;
      --good: #78d08c;
      --bad: #d96f6f;
    }}
    * {{ box-sizing: border-box; }}
    body {{
      margin: 0;
      background: var(--bg);
      color: var(--text);
      font-family: Segoe UI, Arial, sans-serif;
      line-height: 1.45;
    }}
    header.hero {{
      padding: 24px 32px 16px;
      border-bottom: 1px solid var(--line);
      background: linear-gradient(180deg, #211513 0%, #120f12 100%);
    }}
    h1, h2, h3 {{ margin: 0; letter-spacing: 0; }}
    h1 {{ font-size: 30px; }}
    h2 {{ font-size: 18px; margin-bottom: 12px; color: var(--gold); }}
    h3 {{ font-size: 16px; }}
    .repo {{ margin-top: 8px; color: var(--muted); }}
    main {{ padding: 20px 32px 36px; display: grid; gap: 20px; }}
    .tabs {{ display: flex; gap: 8px; flex-wrap: wrap; }}
    .tab {{
      color: var(--text);
      text-decoration: none;
      border: 1px solid var(--line);
      padding: 8px 12px;
      background: var(--panel);
    }}
    .tab.active {{ border-color: var(--accent); color: white; background: #3a1f18; }}
    .panel {{
      border: 1px solid var(--line);
      background: var(--panel);
      padding: 16px;
    }}
    .workspace-list {{ list-style: none; margin: 0; padding: 0; display: grid; gap: 8px; }}
    .workspace-list li {{
      display: grid;
      grid-template-columns: 170px minmax(0, 1fr) 80px;
      gap: 12px;
      align-items: center;
      border-bottom: 1px solid #33251f;
      padding-bottom: 8px;
    }}
    code, pre {{ font-family: Consolas, monospace; }}
    code {{ color: #f9cf8f; overflow-wrap: anywhere; }}
    .present {{ color: var(--good); }}
    .missing {{ color: var(--bad); }}
    .actions {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 14px; }}
    .action-card {{ border: 1px solid var(--line); background: var(--panel-2); padding: 12px; }}
    .action-card header {{ display: flex; justify-content: space-between; gap: 12px; align-items: baseline; margin-bottom: 10px; }}
    .action-card header span {{ color: var(--gold); font-weight: 600; }}
    .media {{
      min-height: 160px;
      display: grid;
      place-items: center;
      background: #100d0d;
      border: 1px solid #352720;
      overflow: hidden;
    }}
    .media img {{ max-width: 100%; max-height: 260px; object-fit: contain; image-rendering: auto; }}
    .empty-media {{ color: var(--muted); }}
    dl {{ display: grid; grid-template-columns: 95px minmax(0, 1fr); gap: 6px 10px; margin: 12px 0 0; }}
    dt {{ color: var(--muted); }}
    dd {{ margin: 0; overflow-wrap: anywhere; }}
    a {{ color: #ff9b6d; }}
    textarea {{
      width: 100%;
      min-height: 360px;
      resize: vertical;
      background: #0f0d0d;
      color: var(--text);
      border: 1px solid var(--line);
      padding: 12px;
      font-family: Consolas, monospace;
      font-size: 13px;
    }}
    .note {{ color: var(--muted); margin-top: 8px; }}
    .commands {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 12px; }}
    .command {{ border: 1px solid var(--line); background: #171312; padding: 12px; }}
    .command pre {{ white-space: pre-wrap; margin: 10px 0 0; }}
    @media (max-width: 720px) {{
      header.hero, main {{ padding-left: 16px; padding-right: 16px; }}
      .workspace-list li {{ grid-template-columns: 1fr; gap: 4px; }}
      dl {{ grid-template-columns: 1fr; }}
    }}
  </style>
</head>
<body>
  <header class="hero">
    <h1>Dragon Asset Lab</h1>
    <div class="repo">Repo root: <code>{html.escape(str(root))}</code></div>
  </header>
  <main>
    <nav class="tabs">{char_tabs}</nav>
    <section class="panel">
      <h2>{html.escape(selected_character)} Workspace</h2>
      <ul class="workspace-list">{''.join(workspace_items)}</ul>
    </section>
    <section>
      <h2>Action Dashboard</h2>
      <div class="actions">{''.join(action_cards)}</div>
    </section>
    <section class="panel">
      <h2>Manifest Viewer</h2>
      <textarea readonly>{html.escape(selected_manifest)}</textarea>
      <div class="note">Read-only for this MVP to avoid corrupting curated manifests. Next step: add schema-aware edits for notes, selected_source_frames, and preview_fps with atomic backup writes.</div>
    </section>
    <section>
      <h2>Export / Proof Command Stubs</h2>
      <div class="commands">{commands}</div>
    </section>
  </main>
</body>
</html>"""
    return body.encode("utf-8")


class DragonAssetLabHandler(BaseHTTPRequestHandler):
    repo_root: Path = find_repo_root()

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path == "/":
            query = parse_qs(parsed.query)
            selected_character = query.get("char", [CHARACTERS[0]])[0]
            self.send_bytes(render_page(self.repo_root, selected_character), "text/html; charset=utf-8")
            return
        if parsed.path.startswith("/asset/"):
            self.serve_asset(parsed.path.removeprefix("/asset/"))
            return
        self.send_error(HTTPStatus.NOT_FOUND, "Not found")

    def serve_asset(self, raw_rel_path: str) -> None:
        rel = posixpath.normpath(unquote(raw_rel_path)).lstrip("/")
        if rel.startswith("../"):
            self.send_error(HTTPStatus.FORBIDDEN, "Invalid path")
            return
        path = (self.repo_root / Path(rel)).resolve()
        if not is_relative_to(path, self.repo_root):
            self.send_error(HTTPStatus.FORBIDDEN, "Outside repo root")
            return
        if path.suffix.lower() not in MEDIA_SUFFIXES:
            self.send_error(HTTPStatus.FORBIDDEN, "Unsupported media type")
            return
        if not path.exists() or not path.is_file():
            self.send_error(HTTPStatus.NOT_FOUND, "Asset not found")
            return
        mime_type = mimetypes.guess_type(path.name)[0] or "application/octet-stream"
        try:
            data = path.read_bytes()
        except OSError:
            self.send_error(HTTPStatus.INTERNAL_SERVER_ERROR, "Could not read asset")
            return
        self.send_bytes(data, mime_type)

    def send_bytes(self, data: bytes, content_type: str) -> None:
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, format: str, *args: object) -> None:
        sys.stderr.write("[dragon-asset-lab] " + format % args + os.linesep)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Local Dragon MUGEN sprite asset dashboard.")
    parser.add_argument("--repo-root", type=Path, default=find_repo_root(), help="Dragon MUGEN repo root")
    parser.add_argument("--host", default="127.0.0.1", help="Bind host")
    parser.add_argument("--port", type=int, default=8765, help="Bind port")
    return parser


def main() -> int:
    args = build_parser().parse_args()
    repo_root = args.repo_root.resolve()
    if not (repo_root / "game" / "chars").exists():
        print(f"error: repo root does not look valid: {repo_root}", file=sys.stderr)
        return 1
    DragonAssetLabHandler.repo_root = repo_root
    server = ThreadingHTTPServer((args.host, args.port), DragonAssetLabHandler)
    print(f"Dragon Asset Lab running at http://{args.host}:{args.port}/")
    print(f"Repo root: {repo_root}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nDragon Asset Lab stopped.")
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
