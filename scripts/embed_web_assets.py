from pathlib import Path


Import("env")

build_dir = Path(env.subst("$BUILD_DIR"))
output_path = build_dir / "generated_web_assets.h"
data_dir = Path(env.subst("$PROJECT_DIR")) / "web"


def format_asset(name, path):
    data = path.read_bytes()
    values = ", ".join(f"0x{value:02X}" for value in data)
    return f"const uint8_t {name}[] PROGMEM = {{{values}}};\n"


header = """#pragma once

#include <Arduino.h>

"""
header += format_asset("INDEX_HTML", data_dir / "index.html")
header += format_asset("FAVICON_ICO", data_dir / "favicon.ico")

if not output_path.exists() or output_path.read_text() != header:
    output_path.write_text(header)

env.Append(CPPPATH=[str(build_dir)])
