from __future__ import annotations

import json
import mimetypes
import urllib.error
import urllib.request
import uuid
from pathlib import Path
from typing import Any

from lab_config import configured_path, load_config
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


def node_title(node: dict[str, Any]) -> str:
    meta = node.get("_meta")
    meta_title = meta.get("title", "") if isinstance(meta, dict) else ""
    return " ".join(str(part).lower() for part in (node.get("class_type", ""), node.get("title", ""), meta_title) if part)


def workflow_graph(workflow: dict[str, Any]) -> dict[str, Any]:
    prompt = workflow.get("prompt")
    if isinstance(prompt, dict):
        return prompt
    return workflow


UI_WIDGET_INPUTS: dict[str, tuple[str, ...]] = {
    "RandomNoise": ("noise_seed",),
    "KSamplerSelect": ("sampler_name",),
    "ManualSigmas": ("sigmas",),
    "CFGGuider": ("cfg",),
    "LoraLoaderModelOnly": ("lora_name", "strength_model"),
    "ResizeImagesByLongerEdge": ("longer_edge",),
    "LTXVImgToVideoInplace": ("strength", "bypass"),
    "LTXVPreprocess": ("img_compression",),
    "EmptyLTXVLatentVideo": ("width", "height", "length", "batch_size"),
    "LTXVConditioning": ("frame_rate",),
    "LTXVEmptyLatentAudio": ("frames_number", "frame_rate", "batch_size"),
    "CreateVideo": ("fps",),
    "LatentUpscaleModelLoader": ("model_name",),
    "CLIPTextEncode": ("text",),
    "VAEDecodeTiled": ("tile_size", "overlap", "temporal_size", "temporal_overlap"),
    "CheckpointLoaderSimple": ("ckpt_name",),
    "LTXAVTextEncoderLoader": ("text_encoder", "ckpt_name", "device"),
    "LTXVAudioVAELoader": ("ckpt_name",),
    "SaveVideo": ("filename_prefix", "format", "codec"),
    "LoadImage": ("image",),
}

UI_DIRECT_ONLY_TYPES = {
    "MarkdownNote",
    "PrimitiveBoolean",
    "PrimitiveInt",
    "PrimitiveStringMultiline",
    "ComfyMathExpression",
    "Reroute",
}


def ui_node_type(node: dict[str, Any]) -> str:
    return str(node.get("type") or node.get("class_type") or "")


def ui_node_title(node: dict[str, Any]) -> str:
    return " ".join(str(part).lower() for part in (node.get("type", ""), node.get("title", "")) if part)


def ui_nodes_by_id(nodes: list[dict[str, Any]]) -> dict[int, dict[str, Any]]:
    return {int(node["id"]): node for node in nodes if isinstance(node, dict) and "id" in node}


def ui_links_by_id(links: list[Any]) -> dict[int, dict[str, Any]]:
    by_id: dict[int, dict[str, Any]] = {}
    for link in links:
        if isinstance(link, dict):
            by_id[int(link["id"])] = link
        elif isinstance(link, list) and len(link) >= 6:
            by_id[int(link[0])] = {
                "id": int(link[0]),
                "origin_id": int(link[1]),
                "origin_slot": int(link[2]),
                "target_id": int(link[3]),
                "target_slot": int(link[4]),
                "type": link[5],
            }
    return by_id


def ui_widget_inputs(node: dict[str, Any]) -> dict[str, Any]:
    widget_names = UI_WIDGET_INPUTS.get(ui_node_type(node), ())
    widgets = node.get("widgets_values")
    if not isinstance(widgets, list):
        return {}
    return {name: widgets[index] for index, name in enumerate(widget_names) if index < len(widgets)}


def ui_input_links(node: dict[str, Any]) -> list[tuple[str, int | None]]:
    inputs = node.get("inputs")
    if not isinstance(inputs, list):
        return []
    result: list[tuple[str, int | None]] = []
    for entry in inputs:
        if not isinstance(entry, dict):
            continue
        name = str(entry.get("name") or "")
        link = entry.get("link")
        result.append((name, int(link) if isinstance(link, int) else None))
    return result


def ui_output_source_for_link(
    link_id: int | None,
    links_by_id: dict[int, dict[str, Any]],
    nodes_by_id: dict[int, dict[str, Any]],
    external_image_source: list[Any],
    external_values: dict[int, Any] | None = None,
) -> Any | None:
    if link_id is None:
        return None
    link = links_by_id.get(link_id)
    if not link:
        return None
    origin_id = int(link.get("origin_id", -1))
    origin_slot = int(link.get("origin_slot", 0))
    if origin_id == -10:
        if external_values and origin_slot in external_values:
            return external_values[origin_slot]
        return external_image_source if str(link.get("type", "")).startswith("IMAGE") else None
    origin_node = nodes_by_id.get(origin_id)
    if not origin_node:
        return None
    if ui_node_type(origin_node) == "Reroute":
        upstream = None
        for _, candidate in ui_input_links(origin_node):
            upstream = candidate
            break
        return ui_output_source_for_link(upstream, links_by_id, nodes_by_id, external_image_source, external_values)
    if ui_node_type(origin_node) in UI_DIRECT_ONLY_TYPES:
        return None
    return [str(origin_id), origin_slot]


def set_if_missing(inputs: dict[str, Any], key: str, value: Any) -> None:
    if key not in inputs:
        inputs[key] = value


def clip_text_role(
    node: dict[str, Any],
    links_by_id: dict[int, dict[str, Any]],
    nodes_by_id: dict[int, dict[str, Any]],
) -> str:
    node_id = int(node["id"])
    title = ui_node_title(node)
    if "negative" in title:
        return "negative"
    if "positive" in title or "prompt" in title:
        return "positive"
    for link in links_by_id.values():
        if int(link.get("origin_id", -1)) != node_id:
            continue
        target = nodes_by_id.get(int(link.get("target_id", -1)))
        if target and ui_node_type(target) == "LTXVConditioning":
            return "negative" if int(link.get("target_slot", 0)) == 1 else "positive"
    widgets = node.get("widgets_values")
    default_text = str(widgets[0]).lower() if isinstance(widgets, list) and widgets else ""
    if any(token in default_text for token in ("ugly", "blurry", "low quality", "bad anatomy")):
        return "negative"
    return "positive"


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
    external_values: dict[int, Any] = {
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

        uploaded = comfy_upload_image(server_url, image_path)
        uploaded_name = str(uploaded.get("name") or image_path.name)
        output_prefix = f"dragon_asset_lab/{character}_{action}_{run_name}"
        workflow = json.loads(workflow_path.read_text(encoding="utf-8"))
        prompt_graph, notes = patch_comfy_i2v_workflow(
            workflow,
            uploaded_name,
            positive_prompt,
            negative_prompt,
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
                f"Reference image uploaded: {image_path} -> {uploaded_name}",
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
