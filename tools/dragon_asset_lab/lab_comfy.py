from __future__ import annotations

import json
import mimetypes
import urllib.error
import urllib.request
import uuid
from pathlib import Path
from typing import Any

from PIL import Image

from lab_config import configured_path, load_config
from lab_comfy_workflow import (
    UI_DIRECT_ONLY_TYPES,
    clip_text_role,
    node_title,
    set_if_missing,
    ui_input_links,
    ui_links_by_id,
    ui_node_title,
    ui_node_type,
    ui_nodes_by_id,
    ui_output_source_for_link,
    ui_widget_inputs,
    workflow_graph,
)
from lab_paths import ACTIONS, OWNED_CHARACTERS, safe_run_name, timestamp
from lab_result import CommandResult, result_error


def require_owned_character(character: str) -> None:
    if character not in OWNED_CHARACTERS:
        raise ValueError(f"{character} is browse-only in Asset Lab; writes are limited to {', '.join(OWNED_CHARACTERS)}.")


def require_action(action: str) -> str:
    normalized = action.strip().lower()
    if normalized not in ACTIONS:
        raise ValueError(f"Unsupported action: {action!r}")
    return normalized


def positive_int(raw_value: str, default: int, name: str, minimum: int, maximum: int) -> int:
    value_text = raw_value.strip()
    if not value_text:
        return default
    try:
        value = int(value_text)
    except ValueError as exc:
        raise ValueError(f"{name} must be an integer: {raw_value!r}") from exc
    if value < minimum or value > maximum:
        raise ValueError(f"{name} must be between {minimum} and {maximum}; got {value}.")
    return value


def url_error_text(exc: urllib.error.URLError) -> str:
    if isinstance(exc, urllib.error.HTTPError):
        try:
            body = exc.read().decode("utf-8", errors="replace")
        except OSError:
            body = ""
        return f"{exc}\n{body}".strip()
    return str(exc)


def comfy_upload_image(server_url: str, image_path: Path) -> dict[str, Any]:
    if not image_path.exists() or not image_path.is_file():
        raise FileNotFoundError(f"Reference image not found: {image_path}")
    mime_type = mimetypes.guess_type(image_path.name)[0] or "application/octet-stream"
    boundary = f"----DragonAssetLab{uuid.uuid4().hex}"

    def form_part(name: str, value: str) -> bytes:
        return (
            f"--{boundary}\r\n"
            f'Content-Disposition: form-data; name="{name}"\r\n\r\n'
            f"{value}\r\n"
        ).encode("utf-8")

    body = bytearray()
    body.extend(form_part("type", "input"))
    body.extend(form_part("overwrite", "true"))
    body.extend(
        (
            f"--{boundary}\r\n"
            f'Content-Disposition: form-data; name="image"; filename="{image_path.name}"\r\n'
            f"Content-Type: {mime_type}\r\n\r\n"
        ).encode("utf-8")
    )
    body.extend(image_path.read_bytes())
    body.extend(f"\r\n--{boundary}--\r\n".encode("utf-8"))

    request = urllib.request.Request(
        server_url + "/upload/image",
        data=bytes(body),
        headers={"Content-Type": f"multipart/form-data; boundary={boundary}"},
        method="POST",
    )
    with urllib.request.urlopen(request, timeout=60) as response:
        return json.loads(response.read().decode("utf-8", errors="replace"))


def comfy_object_info(server_url: str) -> dict[str, Any]:
    request = urllib.request.Request(server_url.rstrip("/") + "/object_info", method="GET")
    with urllib.request.urlopen(request, timeout=15) as response:
        return json.loads(response.read().decode("utf-8", errors="replace"))


def combo_choices(object_info: dict[str, Any], class_type: str, input_name: str) -> list[str]:
    node_info = object_info.get(class_type)
    if not isinstance(node_info, dict):
        return []
    input_info = node_info.get("input")
    if not isinstance(input_info, dict):
        return []
    for section_name in ("required", "optional"):
        section = input_info.get(section_name)
        if not isinstance(section, dict):
            continue
        schema = section.get(input_name)
        if isinstance(schema, list) and schema and isinstance(schema[0], list):
            return [str(choice) for choice in schema[0]]
        if isinstance(schema, list) and len(schema) > 1 and isinstance(schema[1], dict):
            options = schema[1].get("options")
            if isinstance(options, list):
                return [str(choice) for choice in options]
    return []


def comfy_uploaded_image_name(uploaded_image: Any) -> str:
    if isinstance(uploaded_image, dict):
        name = str(uploaded_image.get("name") or "")
        subfolder = str(uploaded_image.get("subfolder") or "").strip("/")
        image_type = str(uploaded_image.get("type") or "input")
        if subfolder:
            name = f"{subfolder}/{name}"
        if image_type and image_type != "input":
            name = f"{name} [{image_type}]"
        return name
    return str(uploaded_image)


def sprite_reference_upload_dir(root: Path) -> Path:
    upload_dir = root / "artifacts" / "asset_lab" / "comfy_uploads"
    upload_dir.mkdir(parents=True, exist_ok=True)
    return upload_dir


def alpha_content_bbox(image: Image.Image) -> tuple[int, int, int, int] | None:
    if "A" not in image.getbands():
        return None
    return image.getchannel("A").getbbox()


def prepare_sprite_reference_for_comfy(
    root: Path,
    image_path: Path,
    character: str,
    action: str,
    run_name: str,
    width: int,
    height: int,
) -> Path:
    source = Image.open(image_path).convert("RGBA")
    bbox = alpha_content_bbox(source)
    cropped = source.crop(bbox) if bbox else source

    margin_x = max(32, int(width * 0.12))
    margin_y = max(32, int(height * 0.08))
    fit_width = max(1, width - margin_x * 2)
    fit_height = max(1, height - margin_y * 2)
    scale = min(fit_width / cropped.width, fit_height / cropped.height)
    scale = min(scale, 1.35)
    scaled_size = (max(1, int(round(cropped.width * scale))), max(1, int(round(cropped.height * scale))))
    resampling = getattr(Image.Resampling, "LANCZOS", Image.LANCZOS)
    fitted = cropped.resize(scaled_size, resampling)

    canvas = Image.new("RGBA", (width, height), (245, 245, 245, 255))
    x = (width - fitted.width) // 2
    y = height - margin_y - fitted.height
    if y < margin_y:
        y = margin_y
    canvas.alpha_composite(fitted, (x, y))

    upload_dir = sprite_reference_upload_dir(root)
    output_name = safe_run_name(f"{character}_{action}_{run_name}_reference_{width}x{height}") + ".png"
    output_path = upload_dir / output_name
    canvas.convert("RGB").save(output_path)
    return output_path


def append_prompt_guardrails(positive_prompt: str, negative_prompt: str) -> tuple[str, str]:
    positive_guardrail = (
        " Full body centered inside frame with extra empty margin around hands and shoes."
        " Locked side-view fighting game sprite camera."
        " Character remains fully visible from head to feet."
        " Plain solid white studio background."
        " No scene background."
    )
    negative_guardrail = (
        "cropped, cut off hands, cut off feet, missing shoes, missing fingers, out of frame,"
        " close-up, camera zoom, camera pan, checkerboard background, patterned wall,"
        " dotted wall, fence, mesh, net, lattice, chain-link, crowd, stadium,"
        " busy background, extra limbs, duplicate body"
    )
    combined_positive = f"{positive_prompt.rstrip()} {positive_guardrail}".strip()
    if negative_prompt:
        combined_negative = f"{negative_prompt.rstrip()}, {negative_guardrail}"
    else:
        combined_negative = negative_guardrail
    return combined_positive, combined_negative


def first_matching_choice(choices: list[str], *needles: str) -> str | None:
    lowered_needles = [needle.lower() for needle in needles if needle]
    for choice in choices:
        lowered_choice = choice.lower().replace("\\", "/")
        if all(needle in lowered_choice for needle in lowered_needles):
            return choice
    return None


def preferred_combo_choice(class_type: str, input_name: str, current: Any, choices: list[str]) -> str:
    current_text = str(current)
    if current_text in choices:
        return current_text
    if class_type == "LTXVAudioVAELoader" and input_name == "ckpt_name":
        return first_matching_choice(choices, "audio", "vae") or choices[0]
    if class_type in {"CheckpointLoaderSimple", "LTXAVTextEncoderLoader", "LTXVGemmaCLIPModelLoader"} and input_name in {"ckpt_name", "ltxv_path"}:
        return (
            first_matching_choice(choices, "ltx-2-19b", "distilled")
            or first_matching_choice(choices, "ltx-2.3", "distilled")
            or first_matching_choice(choices, "ltx", "distilled")
            or first_matching_choice(choices, "ltx")
            or choices[0]
        )
    if class_type == "LTXAVTextEncoderLoader" and input_name == "text_encoder":
        return first_matching_choice(choices, "gemma_3_12b") or first_matching_choice(choices, "gemma") or choices[0]
    if class_type == "LTXVGemmaCLIPModelLoader" and input_name == "gemma_path":
        return first_matching_choice(choices, "gemma_3_12b") or first_matching_choice(choices, "gemma") or choices[0]
    if class_type == "LatentUpscaleModelLoader" and input_name == "model_name":
        return first_matching_choice(choices, "ltx-2.3", "spatial", "1.0") or first_matching_choice(choices, "ltx", "spatial") or choices[0]
    if class_type == "LoraLoaderModelOnly" and input_name == "lora_name":
        return (
            first_matching_choice(choices, "ltx-2.3", "distilled", "384")
            or first_matching_choice(choices, "ltx")
            or choices[0]
        )
    return choices[0]


def coerce_prompt_for_comfy_server(
    prompt: dict[str, Any],
    server_url: str,
) -> list[str]:
    if not server_url:
        return []
    notes: list[str] = []
    try:
        object_info = comfy_object_info(server_url)
    except (OSError, urllib.error.URLError, json.JSONDecodeError) as exc:
        return [f"warning: could not inspect Comfy object_info for model compatibility: {exc}"]

    for node_id, node in prompt.items():
        if not isinstance(node, dict):
            continue
        class_type = str(node.get("class_type", ""))
        inputs = node.get("inputs")
        if not isinstance(inputs, dict):
            continue

        if class_type == "LTXAVTextEncoderLoader" and "LTXVGemmaCLIPModelLoader" in object_info:
            old_inputs = inputs
            node["class_type"] = "LTXVGemmaCLIPModelLoader"
            node["inputs"] = {
                "gemma_path": old_inputs.get("text_encoder", "gemma_3_12B_it_fp4_mixed.safetensors"),
                "ltxv_path": old_inputs.get("ckpt_name", ""),
                "max_length": 1024,
            }
            class_type = "LTXVGemmaCLIPModelLoader"
            inputs = node["inputs"]
            notes.append(f"node {node_id}: replaced LTXAVTextEncoderLoader with LTXVGemmaCLIPModelLoader")

        if (
            class_type == "LTXVImgToVideoInplace"
            and "LTXVImgToVideoInplace" not in object_info
            and "LTXVImgToVideoConditionOnly" in object_info
        ):
            node["class_type"] = "LTXVImgToVideoConditionOnly"
            class_type = "LTXVImgToVideoConditionOnly"
            notes.append(f"node {node_id}: replaced LTXVImgToVideoInplace with LTXVImgToVideoConditionOnly")

        if class_type == "LoadImage":
            continue
        for input_name, value in list(inputs.items()):
            if isinstance(value, list):
                continue
            choices = combo_choices(object_info, class_type, input_name)
            if not choices or str(value) in choices:
                continue
            replacement = preferred_combo_choice(class_type, input_name, value, choices)
            inputs[input_name] = replacement
            notes.append(f"node {node_id}: {class_type}.{input_name} {value!r} -> {replacement!r}")
    return notes


def convert_ltx_ui_workflow_to_api_prompt(
    workflow: dict[str, Any],
    uploaded_image: Any,
    positive_prompt: str,
    negative_prompt: str,
    width: int,
    height: int,
    fps: int,
    duration_seconds: int,
    output_prefix: str,
) -> tuple[dict[str, Any] | None, list[str]]:
    nodes = workflow.get("nodes")
    definitions = workflow.get("definitions")
    subgraphs = definitions.get("subgraphs") if isinstance(definitions, dict) else None
    if not isinstance(nodes, list) or not isinstance(subgraphs, list) or not subgraphs:
        return None, []

    subgraph = next(
        (
            candidate
            for candidate in subgraphs
            if isinstance(candidate, dict)
            and any(isinstance(node, dict) and ui_node_type(node) == "CreateVideo" for node in candidate.get("nodes", []))
        ),
        None,
    )
    if not isinstance(subgraph, dict):
        return None, []

    top_nodes = [node for node in nodes if isinstance(node, dict)]
    top_load = next((node for node in top_nodes if ui_node_type(node) == "LoadImage"), None)
    top_save = next((node for node in top_nodes if ui_node_type(node) == "SaveVideo"), None)
    input_node = subgraph.get("inputNode") if isinstance(subgraph.get("inputNode"), dict) else None
    output_node = subgraph.get("outputNode") if isinstance(subgraph.get("outputNode"), dict) else None
    uses_blueprint_wrapper = not isinstance(top_load, dict) or not isinstance(top_save, dict)

    sub_nodes = [node for node in subgraph.get("nodes", []) if isinstance(node, dict)]
    all_nodes_by_id = ui_nodes_by_id([*top_nodes, *sub_nodes])
    links_by_id = ui_links_by_id([*workflow.get("links", []), *subgraph.get("links", [])])
    load_node_id = str(top_load["id"]) if isinstance(top_load, dict) else "asset_lab_reference_image"
    save_node_id = str(top_save["id"]) if isinstance(top_save, dict) else "asset_lab_save_video"
    external_image_source: list[Any] = [load_node_id, 0]
    frame_count = max(9, fps * duration_seconds + 1)
    prompt: dict[str, Any] = {}
    uploaded_image_name = comfy_uploaded_image_name(uploaded_image)
    subgraph_name = str(subgraph.get("name", "")).lower()
    if "first-last-frame" in subgraph_name:
        external_values: dict[int, Any] = {
            0: external_image_source,
            1: external_image_source,
            2: positive_prompt,
            3: width,
            4: height,
            5: duration_seconds,
            6: fps,
            7: 0,
            8: "ltx-2-19b-distilled.safetensors",
            9: "gemma_3_12B_it_fp4_mixed.safetensors",
        }
    elif "canny" in subgraph_name:
        external_values = {
            0: positive_prompt,
            1: external_image_source,
            2: 1.0,
            3: False,
            4: external_image_source,
            5: "ltx-2-19b-distilled.safetensors",
            6: "ltx-2-19b-ic-lora-canny-control.safetensors",
            7: "gemma_3_12B_it_fp4_mixed.safetensors",
            8: "ltx-2.3-22b-distilled-lora-384.safetensors",
            9: "ltx-2.3-spatial-upscaler-x2-1.0.safetensors",
        }
    elif "pose" in subgraph_name:
        external_values = {
            0: positive_prompt,
            1: external_image_source,
            2: external_image_source,
            3: 1.0,
            4: False,
            5: "ltx-2-19b-distilled.safetensors",
            6: "ltx-2-19b-ic-lora-pose-control.safetensors",
            7: "gemma_3_12B_it_fp4_mixed.safetensors",
            8: "ltx-2.3-22b-distilled-lora-384.safetensors",
            9: "ltx-2.3-spatial-upscaler-x2-1.0.safetensors",
        }
    else:
        external_values = {
            0: external_image_source,
            1: positive_prompt,
            2: width,
            3: height,
            4: duration_seconds,
            5: "ltx-2-19b-distilled.safetensors",
            6: "ltx-2.3-22b-distilled-lora-384.safetensors",
            7: "gemma_3_12B_it_fp4_mixed.safetensors",
            8: "ltx-2.3-spatial-upscaler-x2-1.0.safetensors",
            9: fps,
        }

    prompt[load_node_id] = {
        "class_type": "LoadImage",
        "inputs": {"image": uploaded_image_name},
        "_meta": {"title": "Reference Image"},
    }

    for node in sub_nodes:
        node_type = ui_node_type(node)
        if node_type in UI_DIRECT_ONLY_TYPES:
            continue
        node_id = str(node["id"])
        inputs = ui_widget_inputs(node)
        title = ui_node_title(node)

        if node_type == "ResizeImageMaskNode":
            image_link = next((link for name, link in ui_input_links(node) if name == "input"), None)
            prompt[node_id] = {
                "class_type": "ImageScale",
                "inputs": {
                    "image": ui_output_source_for_link(image_link, links_by_id, all_nodes_by_id, external_image_source, external_values) or external_image_source,
                    "upscale_method": "lanczos",
                    "width": width,
                    "height": height,
                    "crop": "disabled",
                },
                "_meta": {"title": "Asset Lab Reference Resize"},
            }
            continue

        if node_type == "ResizeImagesByLongerEdge":
            image_link = next((link for name, link in ui_input_links(node) if name == "images"), None)
            prompt[node_id] = {
                "class_type": "ImageScale",
                "inputs": {
                    "image": ui_output_source_for_link(image_link, links_by_id, all_nodes_by_id, external_image_source, external_values) or external_image_source,
                    "upscale_method": "lanczos",
                    "width": width,
                    "height": height,
                    "crop": "disabled",
                },
                "_meta": {"title": "Asset Lab Reference Scale"},
            }
            continue

        if node_type == "CLIPTextEncode":
            inputs["text"] = negative_prompt if clip_text_role(node, links_by_id, all_nodes_by_id) == "negative" else positive_prompt
        elif node_type == "EmptyLTXVLatentVideo":
            inputs.update({"width": max(64, width // 2), "height": max(64, height // 2), "length": frame_count, "batch_size": 1})
        elif node_type == "LTXVEmptyLatentAudio":
            inputs.update({"frames_number": frame_count, "frame_rate": fps, "batch_size": 1})
        elif node_type == "LTXVConditioning":
            inputs["frame_rate"] = float(fps)
        elif node_type == "CreateVideo":
            inputs["fps"] = float(fps)

        for input_name, link_id in ui_input_links(node):
            normalized_name = input_name.split(".", 1)[0]
            if normalized_name in inputs:
                continue
            source = ui_output_source_for_link(link_id, links_by_id, all_nodes_by_id, external_image_source, external_values)
            if source is not None:
                inputs[normalized_name] = source

        if node_type == "LTXVImgToVideoInplace":
            set_if_missing(inputs, "bypass", False)
        if node_type == "LTXVEmptyLatentAudio":
            set_if_missing(inputs, "batch_size", 1)
        if node_type == "LTXVAddGuide":
            set_if_missing(inputs, "frame_idx", 0)
            set_if_missing(inputs, "strength", 1.0)
        if node_type == "LTXVScheduler":
            set_if_missing(inputs, "steps", 20)
            set_if_missing(inputs, "max_shift", 2.05)
            set_if_missing(inputs, "base_shift", 0.95)
            set_if_missing(inputs, "stretch", True)
            set_if_missing(inputs, "terminal", 0.1)

        prompt[node_id] = {"class_type": node_type, "inputs": inputs}

    create_video = next((node for node in sub_nodes if ui_node_type(node) == "CreateVideo"), None)
    if not isinstance(create_video, dict):
        return None, ["warning: UI workflow was detected, but no CreateVideo node was found."]

    save_video_source: list[Any] = [str(create_video["id"]), 0]
    if output_node:
        output_id = int(output_node.get("id", -20))
        output_link = next(
            (
                link
                for link in links_by_id.values()
                if int(link.get("target_id", -1)) == output_id and int(link.get("target_slot", 0)) == 0
            ),
            None,
        )
        if output_link:
            save_video_source = [str(output_link.get("origin_id")), int(output_link.get("origin_slot", 0))]

    prompt[save_node_id] = {
        "class_type": "SaveVideo",
        "inputs": {
            "video": save_video_source,
            "filename_prefix": output_prefix,
            "format": "mp4",
            "codec": "h264",
        },
        "_meta": {"title": "Save Asset Lab Video"},
    }

    notes = [
        "converted Comfy UI LTX image-to-video template to API prompt",
        "using blueprint wrapper external inputs" if uses_blueprint_wrapper else "using top-level LoadImage/SaveVideo nodes",
        f"api nodes generated: {len(prompt)}",
        f"frame-count fields patched: 2 ({frame_count} frames requested)",
        "image nodes patched: 1",
        "positive prompt nodes patched: 1",
        "negative prompt nodes patched: 1",
        "size fields patched: 4",
        "fps fields patched: 3",
        "output prefix fields patched: 1",
    ]
    return prompt, notes


def patch_comfy_i2v_workflow(
    workflow: dict[str, Any],
    uploaded_image: Any,
    positive_prompt: str,
    negative_prompt: str,
    width: int,
    height: int,
    fps: int,
    duration_seconds: int,
    output_prefix: str,
    server_url: str = "",
) -> tuple[dict[str, Any], list[str]]:
    converted_graph, converted_notes = convert_ltx_ui_workflow_to_api_prompt(
        workflow,
        uploaded_image,
        positive_prompt,
        negative_prompt,
        width,
        height,
        fps,
        duration_seconds,
        output_prefix,
    )
    if converted_graph is not None:
        converted_notes.extend(coerce_prompt_for_comfy_server(converted_graph, server_url))
        return converted_graph, converted_notes

    graph = workflow_graph(workflow)
    frame_count = max(1, fps * duration_seconds)
    text_nodes: list[tuple[str, dict[str, Any]]] = []
    notes: list[str] = []
    image_count = 0
    size_count = 0
    fps_count = 0
    frame_count_updates = 0
    output_count = 0

    for _, node in graph.items():
        if not isinstance(node, dict):
            continue
        inputs = node.get("inputs")
        if not isinstance(inputs, dict):
            continue
        if node.get("class_type") == "LoadImage" and "image" in inputs:
            inputs["image"] = comfy_uploaded_image_name(uploaded_image)
            image_count += 1
        if "text" in inputs and isinstance(inputs.get("text"), str):
            text_nodes.append(("", node))
        for key in ("width", "W", "image_width"):
            if key in inputs:
                inputs[key] = width
                size_count += 1
        for key in ("height", "H", "image_height"):
            if key in inputs:
                inputs[key] = height
                size_count += 1
        if "fps" in inputs:
            inputs["fps"] = fps
            fps_count += 1
        for key in ("num_frames", "frames", "frame_count", "video_frames", "length"):
            if key in inputs:
                inputs[key] = frame_count
                frame_count_updates += 1
        for key in ("filename_prefix", "prefix"):
            if key in inputs:
                inputs[key] = output_prefix
                output_count += 1

    positive_set = 0
    negative_set = 0
    for _, node in text_nodes:
        title = node_title(node)
        inputs = node["inputs"]
        if "negative" in title:
            inputs["text"] = negative_prompt
            negative_set += 1
        elif "positive" in title or "prompt" in title:
            inputs["text"] = positive_prompt
            positive_set += 1
    if text_nodes and positive_set == 0:
        text_nodes[0][1]["inputs"]["text"] = positive_prompt
        positive_set = 1
    if negative_prompt and len(text_nodes) > 1 and negative_set == 0:
        text_nodes[1][1]["inputs"]["text"] = negative_prompt
        negative_set = 1

    if image_count == 0:
        notes.append("warning: no LoadImage node with an image input was found.")
    if positive_set == 0:
        notes.append("warning: no text prompt node was patched.")
    notes.extend(
        [
            f"image nodes patched: {image_count}",
            f"positive prompt nodes patched: {positive_set}",
            f"negative prompt nodes patched: {negative_set}",
            f"size fields patched: {size_count}",
            f"fps fields patched: {fps_count}",
            f"frame-count fields patched: {frame_count_updates} ({frame_count} frames requested)",
            f"output prefix fields patched: {output_count}",
        ]
    )
    notes.extend(coerce_prompt_for_comfy_server(graph, server_url))
    return graph, notes


def comfy_connection_smoke_prompt(
    uploaded_image: Any,
    width: int,
    height: int,
    fps: int,
    duration_seconds: int,
    output_prefix: str,
) -> dict[str, Any]:
    frame_count = max(1, fps * duration_seconds)
    return {
        "1": {
            "class_type": "LoadImage",
            "inputs": {"image": comfy_uploaded_image_name(uploaded_image)},
            "_meta": {"title": "Asset Lab Smoke Reference"},
        },
        "2": {
            "class_type": "ImageScale",
            "inputs": {
                "image": ["1", 0],
                "upscale_method": "lanczos",
                "width": width,
                "height": height,
                "crop": "disabled",
            },
            "_meta": {"title": "Asset Lab Smoke Resize"},
        },
        "3": {
            "class_type": "RepeatImageBatch",
            "inputs": {
                "image": ["2", 0],
                "amount": frame_count,
            },
            "_meta": {"title": "Asset Lab Smoke Frames"},
        },
        "4": {
            "class_type": "CreateVideo",
            "inputs": {
                "images": ["3", 0],
                "fps": float(fps),
                "bit_depth": 8,
            },
            "_meta": {"title": "Asset Lab Smoke Video"},
        },
        "5": {
            "class_type": "SaveVideo",
            "inputs": {
                "video": ["4", 0],
                "filename_prefix": output_prefix,
                "format": "mp4",
                "codec": "h264",
            },
            "_meta": {"title": "Save Asset Lab Smoke Video"},
        },
    }


def submit_comfy_prompt(server_url: str, prompt_graph: dict[str, Any]) -> dict[str, Any]:
    payload = json.dumps({"prompt": prompt_graph, "client_id": "dragon_asset_lab"}).encode("utf-8")
    request = urllib.request.Request(
        server_url + "/prompt",
        data=payload,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urllib.request.urlopen(request, timeout=60) as response:
        body = response.read().decode("utf-8", errors="replace")
    response_data = json.loads(body)
    response_data["_raw_body"] = body
    return response_data


def submit_comfy_workflow(root: Path, workflow_kind: str) -> CommandResult:
    title = "Submit Comfy workflow"
    config = load_config(root)
    if not config.get("enable_direct_submit"):
        return result_error(title, "POST /prompt", "Direct Comfy submission is disabled in Asset Lab config.")
    server_url = str(config.get("comfy_server_url", "")).rstrip("/")
    if not server_url:
        return result_error(title, "POST /prompt", "Comfy server URL is not configured.")
    workflow_key = "image_to_image_workflow_json" if workflow_kind == "image_to_image" else "workflow_json"
    workflow_path = configured_path(root, str(config.get(workflow_key, "")))
    if not workflow_path or not workflow_path.exists():
        return result_error(title, "POST /prompt", f"Workflow JSON is missing for {workflow_kind}.")
    try:
        workflow = json.loads(workflow_path.read_text(encoding="utf-8"))
        payload = json.dumps({"prompt": workflow, "client_id": "dragon_asset_lab"}).encode("utf-8")
        request = urllib.request.Request(
            server_url + "/prompt",
            data=payload,
            headers={"Content-Type": "application/json"},
            method="POST",
        )
        with urllib.request.urlopen(request, timeout=20) as response:
            body = response.read().decode("utf-8", errors="replace")
        return CommandResult(
            title=title,
            command=f"POST {server_url}/prompt",
            returncode=0,
            stdout=f"Submitted {workflow_path}\nResponse:\n{body}",
        )
    except urllib.error.URLError as exc:
        return result_error(title, f"POST {server_url}/prompt", url_error_text(exc))
    except (OSError, json.JSONDecodeError) as exc:
        return result_error(title, f"POST {server_url}/prompt", str(exc))


def submit_comfy_image_to_video(root: Path, character: str, action: str, form: dict[str, list[str]]) -> CommandResult:
    title = "Submit Comfy image-to-video"
    try:
        require_owned_character(character)
        action = require_action(action)
        config = load_config(root)
        if not config.get("enable_direct_submit"):
            return result_error(title, "POST /prompt", "Direct Comfy submission is disabled in Asset Lab config.")
        server_url = str(config.get("comfy_server_url", "")).rstrip("/")
        if not server_url:
            return result_error(title, "POST /prompt", "Comfy server URL is not configured.")
        workflow_path = configured_path(root, str(config.get("workflow_json", "")))
        if not workflow_path or not workflow_path.exists():
            return result_error(title, "POST /prompt", "Video workflow JSON is not configured or does not exist.")

        image_path = configured_path(root, form.get("reference_image_path", [""])[0])
        if not image_path:
            raise ValueError("Reference image path is required.")
        positive_prompt = form.get("positive_prompt", [""])[0].strip()
        if not positive_prompt:
            raise ValueError("Positive prompt is required.")
        negative_prompt = form.get("negative_prompt", [""])[0].strip()
        width = positive_int(form.get("width", [""])[0], 512, "width", 64, 2048)
        height = positive_int(form.get("height", [""])[0], 672, "height", 64, 2048)
        fps = positive_int(form.get("fps", [""])[0], 12, "fps", 1, 60)
        duration_seconds = positive_int(form.get("duration_seconds", [""])[0], 6, "duration_seconds", 1, 30)
        run_name = safe_run_name(form.get("run_name", [""])[0]) or safe_run_name(f"{action}_{timestamp()}")
        comfy_reference_path = prepare_sprite_reference_for_comfy(
            root,
            image_path,
            character,
            action,
            run_name,
            width,
            height,
        )
        patched_positive_prompt, patched_negative_prompt = append_prompt_guardrails(positive_prompt, negative_prompt)

        uploaded = comfy_upload_image(server_url, comfy_reference_path)
        uploaded_name = comfy_uploaded_image_name(uploaded)
        output_prefix = f"dragon_asset_lab/{character}_{action}_{run_name}"
        workflow = json.loads(workflow_path.read_text(encoding="utf-8"))
        prompt_graph, notes = patch_comfy_i2v_workflow(
            workflow,
            uploaded_name,
            patched_positive_prompt,
            patched_negative_prompt,
            width,
            height,
            fps,
            duration_seconds,
            output_prefix,
            server_url,
        )
        response_data = submit_comfy_prompt(server_url, prompt_graph)
        body = str(response_data.get("_raw_body", ""))
        prompt_id = response_data.get("prompt_id", "(unknown)")
        stdout = "\n".join(
            [
                f"Queued Comfy image-to-video prompt: {prompt_id}",
                f"Workflow: {workflow_path}",
                f"Original reference: {image_path}",
                f"Sprite-safe reference uploaded: {comfy_reference_path} -> {uploaded_name}",
                f"Output prefix: {output_prefix}",
                f"Requested: {width}x{height}, {fps} fps, {duration_seconds}s",
                "",
                "Patch report:",
                *notes,
                "",
                "Comfy response:",
                body,
                "",
                "After Comfy finishes, import the generated MP4 with Source Video Import / Prepare.",
            ]
        )
        return CommandResult(title=title, command=f"POST {server_url}/upload/image + /prompt", returncode=0, stdout=stdout)
    except urllib.error.URLError as exc:
        return result_error(title, "POST /upload/image + /prompt", url_error_text(exc))
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        return result_error(title, "POST /upload/image + /prompt", str(exc))


def submit_comfy_smoke_video(root: Path, character: str, action: str, form: dict[str, list[str]]) -> CommandResult:
    title = "Submit Comfy smoke MP4"
    try:
        require_owned_character(character)
        action = require_action(action)
        config = load_config(root)
        if not config.get("enable_direct_submit"):
            return result_error(title, "POST /prompt", "Direct Comfy submission is disabled in Asset Lab config.")
        server_url = str(config.get("comfy_server_url", "")).rstrip("/")
        if not server_url:
            return result_error(title, "POST /prompt", "Comfy server URL is not configured.")

        image_path = configured_path(root, form.get("reference_image_path", [""])[0])
        if not image_path:
            raise ValueError("Reference image path is required.")
        width = positive_int(form.get("width", [""])[0], 512, "width", 64, 2048)
        height = positive_int(form.get("height", [""])[0], 672, "height", 64, 2048)
        fps = positive_int(form.get("fps", [""])[0], 12, "fps", 1, 60)
        duration_seconds = positive_int(form.get("duration_seconds", [""])[0], 2, "duration_seconds", 1, 30)
        run_name = safe_run_name(form.get("run_name", [""])[0]) or safe_run_name(f"{action}_smoke_{timestamp()}")

        uploaded = comfy_upload_image(server_url, image_path)
        uploaded_name = str(uploaded.get("name") or image_path.name)
        output_prefix = f"dragon_asset_lab/{character}_{action}_{run_name}_smoke"
        prompt_graph = comfy_connection_smoke_prompt(uploaded_name, width, height, fps, duration_seconds, output_prefix)
        response_data = submit_comfy_prompt(server_url, prompt_graph)
        body = str(response_data.get("_raw_body", ""))
        prompt_id = response_data.get("prompt_id", "(unknown)")
        stdout = "\n".join(
            [
                f"Queued Comfy smoke MP4 prompt: {prompt_id}",
                f"Reference image uploaded: {image_path} -> {uploaded_name}",
                f"Output prefix: {output_prefix}",
                f"Requested: {width}x{height}, {fps} fps, {duration_seconds}s",
                "",
                "This is a connection/output proof only. It repeats the reference image into an MP4 and does not use AI generation or the prompt text.",
                "Use Image + prompt to video for the real LTX path once the required local LTX model files are installed.",
                "",
                "Comfy response:",
                body,
            ]
        )
        return CommandResult(title=title, command=f"POST {server_url}/upload/image + /prompt", returncode=0, stdout=stdout)
    except urllib.error.URLError as exc:
        return result_error(title, "POST /upload/image + /prompt", url_error_text(exc))
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        return result_error(title, "POST /upload/image + /prompt", str(exc))
