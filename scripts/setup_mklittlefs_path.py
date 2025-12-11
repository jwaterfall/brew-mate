Import("env")  # type: ignore

"""
PlatformIO extra script to add mklittlefs tool to PATH.

This is required for the Seeed Studio XIAO ESP32C6 platform because
the custom platform doesn't automatically configure filesystem tool paths
like the official Espressif platform does.
"""

import os

tool_path = os.path.join(env.PioPlatform().get_package_dir("tool-mklittlefs"))  # type: ignore

if os.path.exists(tool_path):
    env.PrependENVPath("PATH", tool_path)  # type: ignore

