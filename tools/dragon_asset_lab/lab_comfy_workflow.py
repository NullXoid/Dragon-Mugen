from __future__ import annotations

from typing import Any


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
