#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

import yaml


CALIBRATION_KEYS = [
    "vaside",
    "vbside",
    "iaside",
    "ialpha",
    "ibeta",
    "igamma",
    "iRefree",
]

BEGIN_MARKER = "/* CALIBRATION_BEGIN */"
END_MARKER = "/* CALIBRATION_END */"


def format_float(value: object) -> str:
    number = float(value)
    text = f"{number:.10f}f"
    if number < 0:
        return f"({text})"
    return text


def validate_sampler_config(data: dict) -> dict:
    if not isinstance(data, dict):
        raise ValueError("YAML 顶层必须是对象。")

    sampler = data.get("sampler")
    if not isinstance(sampler, dict):
        raise ValueError("缺少 sampler 配置对象。")

    for key in CALIBRATION_KEYS:
        entry = sampler.get(key)
        if not isinstance(entry, dict):
            raise ValueError(f"sampler.{key} 缺失或不是对象。")

        for field in ("adc_channel", "k", "b", "cutoff_freq"):
            if field not in entry:
                raise ValueError(f"sampler.{key}.{field} 缺失。")

        if not isinstance(entry["adc_channel"], str) or not entry["adc_channel"].strip():
            raise ValueError(f"sampler.{key}.adc_channel 必须是非空字符串。")

        for field in ("k", "b", "cutoff_freq"):
            try:
                float(entry[field])
            except (TypeError, ValueError) as exc:
                raise ValueError(f"sampler.{key}.{field} 必须是数字。") from exc

    return sampler


def render_sampler_block(sampler: dict) -> str:
    lines: list[str] = []
    for index, key in enumerate(CALIBRATION_KEYS):
        entry = sampler[key]
        lines.extend(
            [
                f"        .{key} = {{",
                f"            .adc_channel = {entry['adc_channel']},",
                f"            .k = {format_float(entry['k'])},",
                f"            .b = {format_float(entry['b'])},",
                f"            .cutoff_freq = {format_float(entry['cutoff_freq'])}",
                "        }" + ("," if index != len(CALIBRATION_KEYS) - 1 else ""),
            ]
        )
    return "\n".join(lines)


def replace_calibration_block(content: str, rendered_block: str) -> str:
    pattern = re.compile(
        rf"(?P<begin>[ \t]*{re.escape(BEGIN_MARKER)}\n)(?P<body>[\s\S]*?)(?P<end>[ \t]*{re.escape(END_MARKER)})"
    )

    match = pattern.search(content)
    if not match:
        raise ValueError("未在目标文件中找到 CALIBRATION_BEGIN / CALIBRATION_END 标记。")

    replacement = f"{match.group('begin')}{rendered_block}\n        {END_MARKER}"
    return content[: match.start()] + replacement + content[match.end() :]


def main() -> int:
    parser = argparse.ArgumentParser(description="根据 YAML 更新 SuperCap.c 中的采样标定参数块")
    parser.add_argument("yaml_path", help="设备 YAML 配置文件路径")
    parser.add_argument(
        "--target",
        default="Src/app/SuperCap.c",
        help="要回写的目标 C 文件，默认 Src/app/SuperCap.c",
    )
    args = parser.parse_args()

    yaml_path = Path(args.yaml_path)
    target_path = Path(args.target)

    if not yaml_path.is_file():
        raise FileNotFoundError(f"YAML 文件不存在: {yaml_path}")
    if not target_path.is_file():
        raise FileNotFoundError(f"目标文件不存在: {target_path}")

    with yaml_path.open("r", encoding="utf-8") as file:
        data = yaml.safe_load(file)

    sampler = validate_sampler_config(data)
    rendered_block = render_sampler_block(sampler)

    original_content = target_path.read_text(encoding="utf-8")
    updated_content = replace_calibration_block(original_content, rendered_block)
    target_path.write_text(updated_content, encoding="utf-8")

    device_id = data.get("device_id", yaml_path.stem)
    print(f"已将设备 {device_id} 的采样标定参数写入 {target_path}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:  # noqa: BLE001
        print(f"错误: {exc}", file=sys.stderr)
        raise SystemExit(1)