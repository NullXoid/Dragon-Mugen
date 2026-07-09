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

from lab_ops import (
    CommandResult,
    build_sprites,
    command_for_build,
    import_or_prepare_video,
    load_config,
    promote_selected_frames,
    run_proof,
    save_config,
    save_selected_frames,
    submit_comfy_image_to_video,
    submit_comfy_smoke_video,
    submit_comfy_workflow,
)
from lab_paths import (
    ACTIONS,
    MEDIA_SUFFIXES,
    OWNED_CHARACTERS,
    character_kind,
    character_names,
    config_path,
    find_repo_root,
    format_selected_frames,
    is_relative_to,
    media_url,
    relpath,
)
from lab_workspace import (
    character_summary,
    configured_build_modes,
    display_path,
    path_status,
    proof_command_catalog,
)


def esc(value: object) -> str:
    return html.escape(str(value), quote=True)


def link_for(path: Path | None, root: Path, label: str | None = None) -> str:
    if not path:
        return "<span>none</span>"
    text = label or display_path(path, root)
    return f'<a href="{esc(media_url(path, root))}" target="_blank">{esc(text)}</a>'


def path_text(path: Path | None, root: Path, missing: str = "not found") -> str:
    return esc(display_path(path, root, missing))


def render_result(result: CommandResult | None) -> str:
    if not result:
        return ""
    status = "ok" if result.ok else "fail"
    body = "\n".join(part for part in (result.stdout.strip(), result.stderr.strip()) if part)
    if not body:
        body = "(no output)"
    return f"""
    <section class="panel result {status}" id="last-result">
      <div class="result-head">
        <h2>Last Result</h2>
        <b>{'PASS' if result.ok else 'FAIL'} · exit {result.returncode}</b>
      </div>
      <div class="command-line"><code>{esc(result.command)}</code></div>
      <pre>{esc(body)}</pre>
    </section>
    """


def render_tabs(root: Path, selected_character: str) -> str:
    return "".join(
        f'<a class="tab {"active" if char == selected_character else ""}" href="/?char={quote(char)}">'
        f"{esc(char)}<small>{esc(character_kind(char))}</small></a>"
        for char in character_names(root)
    )


def render_workspace(summary: dict, root: Path, selected_character: str) -> str:
    workspace_items = []
    for label, path in summary["workspace_dirs"]:
        present = path.exists()
        status = "present" if present else "missing"
        workspace_items.append(
            "<li>"
            f"<span>{esc(label)}</span>"
            f"<code>{esc(relpath(path, root) if is_relative_to(path, root) else str(path))}</code>"
            f'<b class="{status}">{status}</b>'
            "</li>"
        )
    return f"""
    <section class="panel">
      <h2>{esc(selected_character)} Workspace</h2>
      <ul class="workspace-list">{''.join(workspace_items)}</ul>
    </section>
    """


def render_mugen_files(summary: dict, root: Path, selected_character: str) -> str:
    def_info = summary["def_info"]
    display_name = def_info.get("displayname") or def_info.get("name") or selected_character
    author = def_info.get("author", "unknown")
    localcoord = def_info.get("localcoord", "not declared")
    def_path = summary["def_path"]
    def_status = "present" if def_path else "missing"
    def_label = relpath(def_path, root) if def_path else "no DEF found"
    mugen_file_items = []
    if summary["mugen_files"]:
        for row in summary["mugen_files"]:
            status = "present" if row["present"] else "missing"
            path_label = relpath(row["path"], root) if row["path"] and is_relative_to(row["path"], root) else row["value"]
            detail = f"<span>{esc(row['detail'])}</span>" if row["detail"] else "<span></span>"
            mugen_file_items.append(
                "<li>"
                f"<span>{esc(row['key'])}</span>"
                f"<code>{esc(path_label)}</code>"
                f"{detail}"
                f'<b class="{status}">{status}</b>'
                "</li>"
            )
    else:
        mugen_file_items.append(
            '<li><span>files</span><code>No [Files] entries parsed.</code><span></span><b class="missing">missing</b></li>'
        )

    return f"""
    <section class="panel">
      <h2>MUGEN Character Files</h2>
      <div class="meta-grid">
        <div><span>kind</span>{esc(summary["kind"])}</div>
        <div><span>display name</span>{esc(display_name)}</div>
        <div><span>author</span>{esc(author)}</div>
        <div><span>localcoord</span>{esc(localcoord)}</div>
        <div><span>definition</span><code>{esc(def_label)}</code> <b class="{def_status}">{def_status}</b></div>
      </div>
      <ul class="runtime-list">{''.join(mugen_file_items)}</ul>
    </section>
    """


def render_media_panel(title: str, media_path: Path | None, root: Path, empty: str) -> str:
    media = '<div class="empty-media">' + esc(empty) + "</div>"
    url = media_url(media_path, root) if media_path else ""
    if url:
        media = f'<img src="{esc(url)}" alt="{esc(title)}">'
    return f'<div><div class="media-label">{esc(title)}</div><div class="media">{media}</div></div>'


def render_action_cards(summary: dict, root: Path, selected_character: str) -> str:
    is_owned = selected_character in OWNED_CHARACTERS
    action_cards = []
    for row in summary["action_rows"]:
        action = row["name"]
        manifest_action = row["manifest"]
        run = row["run"]
        selected_frames = format_selected_frames(manifest_action.get("selected_source_frames"))
        curated_media = row["preview"] or row["contact"]
        source_media = run.get("source_preview") or run.get("source_contact")
        source_count = run.get("source_count")
        run_action = run.get("run_action")
        has_manifest_action = bool(manifest_action)
        can_edit = is_owned and has_manifest_action and isinstance(source_count, int) and source_count > 0
        can_promote = can_edit and (not run_action or run_action == action)
        edit_disabled = "" if can_edit else " disabled"
        promote_disabled = "" if can_promote else " disabled"
        edit_note = ""
        if not is_owned:
            edit_note = "Local test characters are browse-only."
        elif not has_manifest_action:
            edit_note = "No curated manifest entry for this action yet."
        elif not can_edit:
            edit_note = "No source frame directory is available for validation."
        elif not can_promote:
            edit_note = f"Promote disabled: run manifest action is {run_action!r}."

        source_video = row["source_video"]
        video_status = "yes" if source_video else "no"
        run_manifest_path = run.get("run_manifest_path")
        source_dir = run.get("source_dir")

        action_cards.append(
            f"""
            <article class="action-card">
              <header><h3>{esc(action)}</h3><span>{row["frame_count"]} curated frames</span></header>
              <div class="media-grid">
                {render_media_panel("Curated preview", curated_media, root, "No curated preview")}
                {render_media_panel("Source preview", source_media, root, "No source preview")}
              </div>
              <dl>
                <dt>curated contact</dt><dd>{link_for(row["contact"], root)}</dd>
                <dt>curated preview</dt><dd>{link_for(row["preview"], root)}</dd>
                <dt>source video</dt><dd>{esc(video_status)} · {link_for(source_video, root, "open video") if source_video else "<span>none</span>"}</dd>
                <dt>run manifest</dt><dd><code>{path_text(run_manifest_path, root)}</code></dd>
                <dt>source frames</dt><dd><code>{path_text(source_dir, root)}</code> · {esc(source_count if source_count is not None else "unknown")} frames</dd>
                <dt>source contact</dt><dd>{link_for(run.get("source_contact"), root)}</dd>
              </dl>
              <form method="post" class="action-form">
                <input type="hidden" name="char" value="{esc(selected_character)}">
                <input type="hidden" name="action" value="{esc(action)}">
                <label>selected_source_frames</label>
                <textarea name="selected_frames" spellcheck="false"{edit_disabled}>{esc(selected_frames)}</textarea>
                <div class="form-row">
                  <button type="submit" name="op" value="save_selection"{edit_disabled}>Save selection</button>
                  <button type="submit" name="op" value="promote_selection"{promote_disabled}>Promote selected</button>
                </div>
                <p class="note">{esc(edit_note or "Save writes the curated manifest with a backup; promote uses a 512x672 cell so wide poses keep more horizontal room.")}</p>
              </form>
            </article>
            """
        )
    return f"""
    <section>
      <h2>Action Dashboard</h2>
      <div class="actions">{''.join(action_cards)}</div>
    </section>
    """


def render_manifest_viewer(summary: dict) -> str:
    manifest_actions = summary["curated_manifest"].get("actions", {}) if isinstance(summary["curated_manifest"].get("actions"), dict) else {}
    selected_manifest = json.dumps(manifest_actions, indent=2, sort_keys=True) if manifest_actions else "{}"
    return f"""
    <section class="panel">
      <h2>Manifest Viewer</h2>
      <textarea readonly class="manifest-view">{esc(selected_manifest)}</textarea>
      <div class="note">Read-only snapshot. Use per-action Save selection controls for explicit schema-aware writes with backups.</div>
    </section>
    """


def render_build_tools(root: Path, selected_character: str) -> str:
    modes = configured_build_modes(root, selected_character)
    if not modes:
        return f"""
        <section class="panel">
          <h2>Sprite Builds</h2>
          <p class="note">No safe browser build tool is configured for {esc(selected_character)}. A.Ben can use build_aben_walk_sff.py; I.Chie still needs a dedicated builder.</p>
        </section>
        """
    controls = []
    for mode in modes:
        controls.append(
            f"""
            <form method="post" class="command-card">
              <input type="hidden" name="char" value="{esc(selected_character)}">
              <input type="hidden" name="build_mode" value="{esc(mode["mode"])}">
              <h3>{esc(mode["label"])}</h3>
              <p>{esc(mode["description"])}</p>
              <pre><code>{esc(command_for_build(root, mode["mode"]))}</code></pre>
              <button type="submit" name="op" value="build_sprites">Run build</button>
            </form>
            """
        )
    return f"""
    <section class="panel">
      <h2>Sprite Builds</h2>
      <div class="commands">{''.join(controls)}</div>
    </section>
    """


def render_video_import(root: Path, selected_character: str) -> str:
    disabled = "" if selected_character in OWNED_CHARACTERS else " disabled"
    action_options = "".join(f'<option value="{esc(action)}">{esc(action)}</option>' for action in ACTIONS)
    return f"""
    <section class="panel">
      <h2>Source Video Import / Prepare</h2>
      <form method="post" class="grid-form">
        <input type="hidden" name="char" value="{esc(selected_character)}">
        <label>Action<select name="action"{disabled}>{action_options}</select></label>
        <label class="wide">Completed video path<input name="video_path" placeholder="C:\\path\\to\\finished_video.mp4"{disabled}></label>
        <label>Run name<input name="run_name" placeholder="optional, e.g. walk_v2"{disabled}></label>
        <label>Sample FPS<input name="sample_fps" placeholder="optional"{disabled}></label>
        <label class="wide">Notes<input name="video_notes" placeholder="optional source note"{disabled}></label>
        <label class="check"><input type="checkbox" name="overwrite_video" value="1"{disabled}> overwrite same-name source_videos copy</label>
        <label class="check"><input type="checkbox" name="prepare_video" value="1"{disabled}> run ltx_sprite_pipeline.py prepare after import</label>
        <label class="check"><input type="checkbox" name="force_prepare" value="1"{disabled}> allow --force for an existing run name</label>
        <div class="wide"><button type="submit" name="op" value="import_video"{disabled}>Import / prepare</button></div>
      </form>
      <p class="note">Imports copy videos into the selected character's source_videos folder and update source_videos/manifest.json with a backup. Prepare outputs stay under source_art/ltx_runs.</p>
    </section>
    """


def render_comfy_config(root: Path, selected_character: str) -> str:
    config = load_config(root)
    video_status = path_status(str(config.get("workflow_json", "")), root)
    i2i_status = path_status(str(config.get("image_to_image_workflow_json", "")), root)
    output_status = path_status(str(config.get("video_output_dir", "")), root)
    direct_ready = bool(config.get("enable_direct_submit") and config.get("comfy_server_url") and video_status == "present")
    i2i_ready = bool(config.get("enable_direct_submit") and config.get("comfy_server_url") and i2i_status == "present")
    direct_disabled = "" if direct_ready else " disabled"
    i2i_disabled = "" if i2i_ready else " disabled"
    i2v_disabled = "" if direct_ready and selected_character in OWNED_CHARACTERS else " disabled"
    checked = " checked" if config.get("enable_direct_submit") else ""
    default_server = esc(str(config.get("comfy_server_url", "")).rstrip("/") or "http://127.0.0.1:8188")
    action_options = "".join(f'<option value="{esc(action)}">{esc(action)}</option>' for action in ACTIONS)
    return f"""
    <section class="panel">
      <h2>Comfy / LTX Config</h2>
      <form method="post" class="grid-form">
        <label class="wide">Comfy server URL<input name="comfy_server_url" value="{esc(config.get("comfy_server_url", ""))}" placeholder="http://127.0.0.1:8188"></label>
        <label class="wide">Video workflow JSON<input name="workflow_json" value="{esc(config.get("workflow_json", ""))}" placeholder="workflow_api.json"></label>
        <label class="wide">Image-to-image workflow JSON<input name="image_to_image_workflow_json" value="{esc(config.get("image_to_image_workflow_json", ""))}" placeholder="optional"></label>
        <label class="wide">Video output folder<input name="video_output_dir" value="{esc(config.get("video_output_dir", ""))}" placeholder="optional external Comfy output folder"></label>
        <label class="check wide"><input type="checkbox" name="enable_direct_submit" value="1"{checked}> enable direct Comfy HTTP submission</label>
        <div class="wide"><button type="submit" name="op" value="save_config">Save config</button></div>
      </form>
      <div class="status-row">
        <span>Config: <code>{esc(config_path(root))}</code></span>
        <span>video workflow: <b class="{video_status.replace(' ', '-')}">{esc(video_status)}</b></span>
        <span>image-to-image workflow: <b class="{i2i_status.replace(' ', '-')}">{esc(i2i_status)}</b></span>
        <span>output folder: <b class="{output_status.replace(' ', '-')}">{esc(output_status)}</b></span>
      </div>
      <div class="commands">
        <form method="post" class="command-card">
          <h3>Submit video workflow</h3>
          <pre><code>POST {default_server}/prompt</code></pre>
          <input type="hidden" name="workflow_kind" value="video">
          <button type="submit" name="op" value="submit_comfy"{direct_disabled}>Submit configured workflow</button>
        </form>
        <form method="post" class="command-card">
          <h3>Submit image-to-image workflow</h3>
          <pre><code>POST {default_server}/prompt</code></pre>
          <input type="hidden" name="workflow_kind" value="image_to_image">
          <button type="submit" name="op" value="submit_comfy"{i2i_disabled}>Submit configured workflow</button>
        </form>
      </div>
      <form method="post" class="grid-form">
        <h3 class="wide">Image + prompt to video</h3>
        <input type="hidden" name="char" value="{esc(selected_character)}">
        <label>Action<select name="action"{i2v_disabled}>{action_options}</select></label>
        <label>Run name<input name="run_name" placeholder="optional, e.g. dash_v03"{i2v_disabled}></label>
        <label>Width<input name="width" value="512"{i2v_disabled}></label>
        <label>Height<input name="height" value="672"{i2v_disabled}></label>
        <label>FPS<input name="fps" value="12"{i2v_disabled}></label>
        <label>Duration seconds<input name="duration_seconds" value="6"{i2v_disabled}></label>
        <label class="wide">Reference image path<input name="reference_image_path" placeholder="C:\\path\\to\\reference.png"{i2v_disabled}></label>
        <label class="wide">Positive prompt<textarea name="positive_prompt" spellcheck="false"{i2v_disabled}>full body A.Ben fighting game sprite animation, complete character visible, side view, wide margins, clean background, smooth motion</textarea></label>
        <label class="wide">Negative prompt<textarea name="negative_prompt" spellcheck="false"{i2v_disabled}>cropped body, cut off feet, cut off hands, missing limbs, extra limbs, camera close-up, blurry, duplicate character</textarea></label>
        <div class="wide"><button type="submit" name="op" value="submit_comfy_i2v"{i2v_disabled}>Upload image and queue video</button></div>
      </form>
      <form method="post" class="grid-form">
        <h3 class="wide">Connection smoke MP4</h3>
        <input type="hidden" name="char" value="{esc(selected_character)}">
        <label>Action<select name="action"{i2v_disabled}>{action_options}</select></label>
        <label>Run name<input name="run_name" placeholder="optional, e.g. smoke_01"{i2v_disabled}></label>
        <label>Width<input name="width" value="512"{i2v_disabled}></label>
        <label>Height<input name="height" value="672"{i2v_disabled}></label>
        <label>FPS<input name="fps" value="12"{i2v_disabled}></label>
        <label>Duration seconds<input name="duration_seconds" value="2"{i2v_disabled}></label>
        <label class="wide">Reference image path<input name="reference_image_path" placeholder="C:\\path\\to\\reference.png"{i2v_disabled}></label>
        <div class="wide"><button type="submit" name="op" value="submit_comfy_smoke"{i2v_disabled}>Upload image and queue smoke MP4</button></div>
      </form>
      <p class="note">Direct Comfy calls are off unless explicitly enabled. The image-to-video helper uploads a local reference image to Comfy, patches the configured video workflow, queues it, then relies on the import form for the completed MP4.</p>
    </section>
    """


def render_proof_tools(root: Path) -> str:
    cards = []
    for item in proof_command_catalog(root):
        button = "Show command" if item["kind"] == "launch_stub" else "Run"
        cards.append(
            f"""
            <form method="post" class="command-card">
              <h3>{esc(item["label"])}</h3>
              <p>{esc(item["description"])}</p>
              <pre><code>{esc(item["command"])}</code></pre>
              <input type="hidden" name="proof_kind" value="{esc(item["kind"])}">
              <button type="submit" name="op" value="run_proof">{button}</button>
            </form>
            """
        )
    return f"""
    <section class="panel">
      <h2>In-Game Proof Helpers</h2>
      <div class="commands">{''.join(cards)}</div>
    </section>
    """


def render_page(root: Path, selected_character: str, result: CommandResult | None = None) -> bytes:
    characters = character_names(root)
    if selected_character not in characters:
        selected_character = characters[0]
    summary = character_summary(root, selected_character)

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
      --field: #0f0d0d;
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
      display: grid;
      gap: 2px;
    }}
    .tab.active {{ border-color: var(--accent); color: white; background: #3a1f18; }}
    .tab small {{ color: var(--muted); font-size: 11px; text-transform: uppercase; }}
    .panel, .action-card, .command-card {{
      border: 1px solid var(--line);
      background: var(--panel);
      padding: 16px;
    }}
    .workspace-list, .runtime-list {{ list-style: none; margin: 0; padding: 0; display: grid; gap: 8px; }}
    .workspace-list li {{
      display: grid;
      grid-template-columns: 170px minmax(0, 1fr) 80px;
      gap: 12px;
      align-items: center;
      border-bottom: 1px solid #33251f;
      padding-bottom: 8px;
    }}
    .runtime-list li {{
      display: grid;
      grid-template-columns: 130px minmax(0, 1fr) 130px 80px;
      gap: 12px;
      align-items: center;
      border-bottom: 1px solid #33251f;
      padding-bottom: 8px;
    }}
    .meta-grid {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(190px, 1fr)); gap: 10px; margin: 0 0 14px; }}
    .meta-grid div {{ border: 1px solid #33251f; background: #171312; padding: 10px; }}
    .meta-grid span, label {{ display: block; color: var(--muted); font-size: 12px; text-transform: uppercase; }}
    code, pre {{ font-family: Consolas, monospace; }}
    code {{ color: #f9cf8f; overflow-wrap: anywhere; }}
    pre {{ white-space: pre-wrap; overflow-wrap: anywhere; margin: 10px 0 0; }}
    .present, .ok {{ color: var(--good); }}
    .missing, .fail {{ color: var(--bad); }}
    .not-configured {{ color: var(--muted); }}
    .actions {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(360px, 1fr)); gap: 14px; }}
    .action-card {{ background: var(--panel-2); }}
    .action-card header {{ display: flex; justify-content: space-between; gap: 12px; align-items: baseline; margin-bottom: 10px; }}
    .action-card header span {{ color: var(--gold); font-weight: 600; }}
    .media-grid {{ display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 10px; }}
    .media-label {{ color: var(--muted); font-size: 12px; margin-bottom: 4px; text-transform: uppercase; }}
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
    dl {{ display: grid; grid-template-columns: 105px minmax(0, 1fr); gap: 6px 10px; margin: 12px 0; }}
    dt {{ color: var(--muted); }}
    dd {{ margin: 0; overflow-wrap: anywhere; }}
    a {{ color: #ff9b6d; }}
    textarea, input, select {{
      width: 100%;
      background: var(--field);
      color: var(--text);
      border: 1px solid var(--line);
      padding: 9px;
      font: inherit;
      text-transform: none;
    }}
    textarea {{ min-height: 72px; resize: vertical; font-family: Consolas, monospace; font-size: 13px; }}
    textarea.manifest-view {{ min-height: 360px; }}
    button {{
      border: 1px solid #b7472c;
      background: #5a261b;
      color: #fff7ed;
      padding: 8px 12px;
      cursor: pointer;
      font-weight: 600;
    }}
    button:disabled {{ opacity: .45; cursor: not-allowed; }}
    .form-row {{ display: flex; gap: 8px; flex-wrap: wrap; margin-top: 8px; }}
    .note {{ color: var(--muted); margin: 8px 0 0; }}
    .commands {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 12px; }}
    .command-card {{ background: #171312; display: grid; align-content: start; gap: 8px; }}
    .command-card p {{ margin: 0; color: var(--muted); }}
    .grid-form {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 12px; }}
    .grid-form .wide {{ grid-column: 1 / -1; }}
    .grid-form .check {{ display: flex; align-items: center; gap: 8px; text-transform: none; color: var(--text); }}
    .grid-form .check input {{ width: auto; }}
    .status-row {{ display: flex; flex-wrap: wrap; gap: 10px 18px; margin: 12px 0; color: var(--muted); }}
    .result {{ background: #151f17; }}
    .result.fail {{ background: #251717; }}
    .result-head {{ display: flex; justify-content: space-between; gap: 12px; align-items: baseline; }}
    .command-line {{ margin: 8px 0; }}
    @media (max-width: 760px) {{
      header.hero, main {{ padding-left: 16px; padding-right: 16px; }}
      .workspace-list li, .runtime-list li {{ grid-template-columns: 1fr; gap: 4px; }}
      .media-grid {{ grid-template-columns: 1fr; }}
      dl {{ grid-template-columns: 1fr; }}
    }}
  </style>
</head>
<body>
  <header class="hero">
    <h1>Dragon Asset Lab</h1>
    <div class="repo">Repo root: <code>{esc(str(root))}</code></div>
  </header>
  <main>
    <nav class="tabs">{render_tabs(root, selected_character)}</nav>
    {render_result(result)}
    {render_workspace(summary, root, selected_character)}
    {render_mugen_files(summary, root, selected_character)}
    {render_action_cards(summary, root, selected_character)}
    {render_build_tools(root, selected_character)}
    {render_video_import(root, selected_character)}
    {render_comfy_config(root, selected_character)}
    {render_proof_tools(root)}
    {render_manifest_viewer(summary)}
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
            characters = character_names(self.repo_root)
            selected_character = query.get("char", [characters[0]])[0]
            self.send_bytes(render_page(self.repo_root, selected_character), "text/html; charset=utf-8")
            return
        if parsed.path.startswith("/asset/"):
            self.serve_asset(parsed.path.removeprefix("/asset/"))
            return
        self.send_error(HTTPStatus.NOT_FOUND, "Not found")

    def do_POST(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path != "/":
            self.send_error(HTTPStatus.NOT_FOUND, "Not found")
            return
        try:
            form = self.read_form()
        except ValueError as exc:
            self.send_error(HTTPStatus.BAD_REQUEST, str(exc))
            return
        characters = character_names(self.repo_root)
        selected_character = form.get("char", [characters[0]])[0]
        op = form.get("op", [""])[0]
        result = self.dispatch_post(op, form)
        self.send_bytes(render_page(self.repo_root, selected_character, result), "text/html; charset=utf-8")

    def read_form(self) -> dict[str, list[str]]:
        try:
            length = int(self.headers.get("Content-Length", "0"))
        except ValueError as exc:
            raise ValueError("Invalid Content-Length") from exc
        if length > 2_000_000:
            raise ValueError("Form body is too large")
        data = self.rfile.read(length).decode("utf-8", errors="replace")
        return parse_qs(data, keep_blank_values=True)

    def dispatch_post(self, op: str, form: dict[str, list[str]]) -> CommandResult:
        character = form.get("char", [""])[0]
        action = form.get("action", [""])[0]
        if op == "save_selection":
            return save_selected_frames(self.repo_root, character, action, form.get("selected_frames", [""])[0])
        if op == "promote_selection":
            return promote_selected_frames(self.repo_root, character, action, form.get("selected_frames", [""])[0])
        if op == "build_sprites":
            return build_sprites(self.repo_root, character, form.get("build_mode", [""])[0])
        if op == "import_video":
            return import_or_prepare_video(self.repo_root, character, action, form)
        if op == "save_config":
            return save_config(self.repo_root, form)
        if op == "submit_comfy":
            return submit_comfy_workflow(self.repo_root, form.get("workflow_kind", ["video"])[0])
        if op == "submit_comfy_i2v":
            return submit_comfy_image_to_video(self.repo_root, character, action, form)
        if op == "submit_comfy_smoke":
            return submit_comfy_smoke_video(self.repo_root, character, action, form)
        if op == "run_proof":
            return run_proof(self.repo_root, form.get("proof_kind", [""])[0])
        return CommandResult(title="Unknown action", command=op, returncode=1, stderr=f"Unknown operation: {op}")

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
