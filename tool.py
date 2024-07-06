import argparse
import asyncio
import datetime
import json
import os
import os.path
import re
import shutil
import sys
import tempfile
from dataclasses import dataclass
from functools import lru_cache
from typing import List, Optional, Tuple

C1 = "\033[34m"
C2 = "\033[32m"
C3 = "\033[33m"
C0 = "\033[0m"
LOGO = rf"""
{C1} ______     __         __         __  __   {C3} __   ______
{C1}/\  ___\   /\ \       /\ \       /\ \/\ \  {C3}/\_\ /\  ___\
{C1}\ \  __\   \ \ \____  \ \ \____  \ \ \_\ \ {C3}\/_/ \ \___  \
{C1} \ \_____\  \ \_____\  \ \_____\  \ \_____\{C3}      \/\_____\
{C1}  \/_____/   \/_____/   \/_____/   \/_____/{C3}       \/_____/ {C2}
 __    __     ______     _____     __  __     __         ______     ______
/\ "-./  \   /\  __ \   /\  __-.  /\ \/\ \   /\ \       /\  ___\   /\  ___\
\ \ \-./\ \  \ \ \/\ \  \ \ \/\ \ \ \ \_\ \  \ \ \____  \ \  __\   \ \___  \
 \ \_\ \ \_\  \ \_____\  \ \____-  \ \_____\  \ \_____\  \ \_____\  \/\_____\
  \/_/  \/_/   \/_____/   \/____/   \/_____/   \/_____/   \/_____/   \/_____/
{C0}"""

if __name__ == "__main__":
    print(LOGO)


log_debug = False


def log(level: str, text: str):
    if level == "debug" and not log_debug:
        return
    # Log level colors
    LEVEL_COLORS = {
        "error": "\033[31m",
        "success": "\033[32m",
        "warning": "\033[33m",
        "info": "\033[34m",
        "debug": "\033[35m",
    }
    RESET_COLOR = "\033[0m"
    # Log level name
    LEVEL_NAME = {
        "error": "[ERROR]  ",
        "success": "[SUCCESS]",
        "warning": "[WARNING]",
        "info": "[INFO]   ",
        "debug": "[DEBUG]  ",
    }
    print(
        f"{LEVEL_COLORS[level]}[{datetime.datetime.now().strftime('%H:%M:%S')}] {LEVEL_NAME[level]}{RESET_COLOR} {text}"
    )


def install_package(package):
    log("info", f"pip install {package} ...")
    try:
        import pip
    except ImportError:
        log("error", "pip not found, please install pip for your python environment")
        exit(1)
    output = ""
    with tempfile.TemporaryFile(mode="w+") as tempf:
        sys.stdout = tempf
        sys.stderr = tempf
        ret = pip.main(["install", package])
        sys.stdout = sys.__stdout__
        sys.stderr = sys.__stderr__
        tempf.seek(0)
        output = tempf.read()
    if ret != 0:
        log("error", f"failed to install {package}, see below for details:")
        print(output)
        exit(1)
    log("success", f"{package} successfully installed")


if os.name == "nt":  # Windows
    try:
        import curses  # noqa: F401
    except ImportError:
        install_package("windows-curses")

try:
    from kconfiglib import Kconfig
    from menuconfig import menuconfig as Kmenuconfig
except ImportError:
    install_package("kconfiglib")
    from kconfiglib import Kconfig
    from menuconfig import menuconfig as Kmenuconfig

try:
    from guiconfig import menuconfig as Kguiconfig

    GUI_AVAILABLE = True
except ImportError:
    GUI_AVAILABLE = False
    log("warning", "guiconfig not available (tkinter not available)")

try:
    from rich.console import Console
    from rich.prompt import Confirm, Prompt
    from rich.table import Table
except ImportError:
    install_package("rich")
    from rich.console import Console  # noqa: F401
    from rich.prompt import Confirm, Prompt
    from rich.table import Table

module_root = os.path.dirname(os.path.abspath(__file__))

con = Console()


@dataclass(eq=True, frozen=True)
class Module:
    name: str
    type: str
    path: str
    abs_path: str
    Mconfig: bool
    Kconfig: bool


@lru_cache()
def list_module_types(dir) -> List[str]:
    module_types = [
        d
        for d in os.listdir(dir)
        if (os.path.isdir(d) and not d.startswith("_") and not d.startswith("."))
    ]
    module_types.sort()
    return module_types


@lru_cache()
def list_modules(dir: str = ".") -> Tuple[List[Module], List[str]]:
    modules = []
    module_types = list_module_types(dir)
    for module_type in module_types:
        module_path = os.path.join(dir, module_type)
        module_dirs = [
            d
            for d in os.listdir(module_path)
            if (
                os.path.isdir(os.path.join(module_path, d))
                and not d.startswith("_")
                and not d.startswith(".")
            )
        ]
        module_dirs.sort()
        for module_dir in module_dirs:
            rel_path = os.path.join(module_type, module_dir)
            full_path = os.path.join(module_path, module_dir)
            Mconfig = os.path.exists(os.path.join(full_path, "Mconfig"))
            Kconfig = os.path.exists(os.path.join(full_path, "Kconfig"))
            modules.append(
                Module(module_dir, module_type, rel_path, full_path, Mconfig, Kconfig)
            )
    return modules, module_types


C_FILE_TEMP = """/**
 * @file &&&FILE_NAME&&&.c
 * @brief &&&BREIF&&&
 * @author &&&AUTHOR&&& (&&&EMAIL&&&)
 * @version &&&VERSION&&&
 * @date &&&DATE&&&
 *
 * THINK DIFFERENTLY
 */

#include "&&&FILE_NAME&&&.h"

// Private Defines --------------------------

// Private Typedefs -------------------------

// Private Macros ---------------------------

// Private Variables ------------------------

// Public Variables -------------------------

// Private Functions ------------------------

// Public Functions -------------------------

// Source Code End --------------------------
"""

H_FILE_TEMP = """/**
 * @file &&&FILE_NAME&&&.h
 * @brief &&&BREIF&&&
 * @author &&&AUTHOR&&& (&&&EMAIL&&&)
 * @date &&&DATE&&&
 *
 * THINK DIFFERENTLY
 */

#ifndef __&&&FILE_NAME_UPPER&&&_H__
#define __&&&FILE_NAME_UPPER&&&_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "modules.h"

// Public Defines ---------------------------

// Public Typedefs --------------------------

// Public Macros ----------------------------

// Exported Variables -----------------------

// Exported Functions -----------------------

#ifdef __cplusplus
}
#endif

#endif /* __&&&FILE_NAME_UPPER&&&__ */

"""

README_TEMP = """# Module: &&&MODULE_NAME&&&

&&&BREIF&&&

## 1. Introduction

## 2. Notice

## 3. Usage

"""

KCONFIG_TEMP1 = """
menuconfig MOD_ENABLE_&&&MODULE_NAME_UPPER&&&
    bool "&&&MODULE_NAME&&& (&&&BREIF&&&)"
    default n
"""
KCONFIG_TEMP2 = """if MOD_ENABLE_&&&MODULE_NAME_UPPER&&&
source "&&&TYPE&&&/&&&FILE_NAME&&&/Kconfig"
endif
"""

MOD_README_TEMP = """# [Ellu's Embedded Modules](https://github.com/ElluIFX/EmbeddedModules)

> [!WARNING]
> This folder is generated by `tool.py`, all the module folders **will be overwritten by next sync**.

## Modules List

> [!NOTE]
> To freeze a module, create a file named `.mfreeze` in this folder and add the module name to it.

&&&MODULE_LIST&&&

## License

To check the license of each module, please go to the original repository.
"""


def pull_latest():
    addr = "https://github.com/ElluIFX/EmbeddedModules"
    log("info", f"pulling latest version from {addr}")
    ret = os.system(f"git pull {addr} --quiet")
    if ret != 0:
        log("error", "pull failed, check your git and network connection")
        exit(1)
    log("success", "pull success")


def module_wizard(available_types):
    con.print(f"[blue]Module root: [green]{module_root}")

    module_type = Prompt.ask(
        "[yellow]Please select module type",
        choices=available_types,
    )

    module_name = Prompt.ask("[yellow]Please input module name")
    module_name = module_name.strip().capitalize()

    module_brief = Prompt.ask("[yellow]Please input module brief")
    module_brief = module_brief.strip().capitalize()

    module_author = "Ellu" if os.getenv("AUTHOR") is None else str(os.getenv("AUTHOR"))
    module_email = (
        "ellu.grif@gmail.com" if os.getenv("EMAIL") is None else str(os.getenv("EMAIL"))
    )
    module_version = (
        "1.0.0" if os.getenv("VERSION") is None else str(os.getenv("VERSION"))
    )

    module_date = datetime.datetime.now().strftime("%Y-%m-%d")
    module_file_name = module_name.replace(" ", "_").lower()

    table = Table(show_header=False, box=None)

    table.add_row("[blue]Module name", ":", f"[green]{module_name}")
    table.add_row("[blue]Module brief", ":", f"[green]{module_brief}")
    table.add_row("[blue]Module type", ":", f"[green]{module_type}")
    table.add_row("[blue]Module date", ":", f"[green]{module_date}")
    table.add_row("[blue]Module filename", ":", f"[green]{module_file_name}.\\[ch]")
    table.add_row("[blue]Module author", ":", f"[green]{module_author}")
    table.add_row("[blue]Module email", ":", f"[green]{module_email}")
    table.add_row("[blue]Module version", ":", f"[green]{module_version}")

    con.print(table)

    conf = Confirm.ask("[yellow]Confirm to create module?", default=True)
    if not conf:
        sys.exit(0)

    create_readme_conf = Confirm.ask("[yellow]Create README.md?", default=False)
    create_kconfig_conf = Confirm.ask("[yellow]Create Kconfig?", default=False)

    # Create module folder
    module_path = os.path.join(module_root, module_type, module_file_name)
    if os.path.exists(module_path):
        con.print(f"[red]Module dir already exists: [yellow]{module_path}")
        exit(1)
    os.makedirs(module_path)

    # Create module files
    c_file_path = os.path.join(module_path, module_file_name + ".c")
    h_file_path = os.path.join(module_path, module_file_name + ".h")

    c_file_content = (
        C_FILE_TEMP.replace("&&&FILE_NAME&&&", module_file_name)
        .replace("&&&BREIF&&&", module_brief)
        .replace("&&&AUTHOR&&&", module_author)
        .replace("&&&EMAIL&&&", module_email)
        .replace("&&&VERSION&&&", module_version)
        .replace("&&&DATE&&&", module_date)
    )
    with open(c_file_path, "w", encoding="utf-8") as f:
        f.write(c_file_content)

    h_file_content = (
        H_FILE_TEMP.replace("&&&FILE_NAME&&&", module_file_name)
        .replace("&&&BREIF&&&", module_brief)
        .replace("&&&AUTHOR&&&", module_author)
        .replace("&&&EMAIL&&&", module_email)
        .replace("&&&VERSION&&&", module_version)
        .replace("&&&DATE&&&", module_date)
        .replace("&&&FILE_NAME_UPPER&&&", module_file_name.upper())
    )
    with open(h_file_path, "w", encoding="utf-8") as f:
        f.write(h_file_content)

    if create_readme_conf:
        readme_file_path = os.path.join(module_path, "README.md")
        readme_content = README_TEMP.replace("&&&MODULE_NAME&&&", module_name).replace(
            "&&&BREIF&&&", module_brief
        )
        with open(readme_file_path, "w", encoding="utf-8") as f:
            f.write(readme_content)

    temp_list = [m.name for m in list_modules()[0] if m.type == module_type]
    temp_list.sort()
    idx = temp_list.index(module_file_name)
    module_path_up = os.path.join(module_root, module_type)
    kconfig_content = (
        KCONFIG_TEMP1.replace("&&&MODULE_NAME_UPPER&&&", module_file_name.upper())
        .replace("&&&MODULE_NAME&&&", module_file_name)
        .replace("&&&BREIF&&&", module_brief)
    )
    if create_kconfig_conf:
        kconfig_content += (
            KCONFIG_TEMP2.replace("&&&TYPE&&&", module_type)
            .replace("&&&FILE_NAME&&&", module_file_name)
            .replace("&&&MODULE_NAME_UPPER&&&", module_file_name.upper())
        )
    with open(os.path.join(module_path_up, "Kconfig"), "r", encoding="utf-8") as f:
        old_content = f.read().splitlines()
    if idx == len(temp_list) - 1:
        new_content = old_content[:-2] + [kconfig_content] + old_content[-2:]
    else:
        next_module = temp_list[idx + 1].upper()
        next_idx = old_content.index(f"menuconfig MOD_ENABLE_{next_module}")
        new_content = (
            old_content[:next_idx] + [kconfig_content] + old_content[next_idx:]
        )
    with open(os.path.join(module_path_up, "Kconfig"), "w", encoding="utf-8") as f:
        f.write("\n".join(new_content))
    if create_kconfig_conf:
        with open(os.path.join(module_path, "Kconfig"), "w", encoding="utf-8") as f:
            f.write("")

    con.print("[green]Module created.")
    con.print(f"[green]C file at: [yellow]{c_file_path}")
    con.print(f"[green]H file at: [yellow]{h_file_path}")


def generate_config_file(conf_name, kconfig_file, config_file, header_out):
    kconf = Kconfig(kconfig_file, warn=False, warn_to_stderr=False)

    # Load config
    kconf.load_config(config_file)
    kconf.write_config(config_file)
    kconf.write_autoconf(header_out)

    with open(header_out, "r+") as header_file:
        content = header_file.read()

    # Preprocess the content
    content = content.replace("CONFIG_", "")
    content = content.replace("\\\\", "\\")
    #  remove standalone " and leave \" as "
    content = re.sub(r'(?<!\\)"', "", content)
    content = re.sub(r"\\\"", '"', content)

    with open(header_out, "w") as header_file:
        conf_name = conf_name.upper()
        header_file.write(
            "/* Configuration header generated by tool.py from Kconfig output */\n\n"
        )
        header_file.write(f"#ifndef _{conf_name}_H_\n")
        header_file.write(f"#define _{conf_name}_H_\n\n")

        header_file.write("#ifdef __cplusplus\n")
        header_file.write('extern "C" {\n')
        header_file.write("#endif /* __cplusplus */\n\n")

        header_file.write(content)

        header_file.write("\n#ifdef __cplusplus\n")
        header_file.write("}\n")
        header_file.write("#endif /* __cplusplus */\n\n")
        header_file.write(f"#endif /* _{conf_name}_H_ */\n")


def prepare_config_file(config_file, output_dir):
    if not output_dir:
        return
    if os.path.exists(os.path.join(output_dir, config_file)):
        if os.path.exists(config_file):
            os.remove(config_file)
        shutil.copyfile(
            os.path.join(output_dir, config_file),
            config_file,
        )


def makeconfig(kconfig_file, config_file, header_file, output_dir):
    generate_config_file("MODULES_CONFIG", kconfig_file, config_file, header_file)
    config_old = f"{config_file}.old"
    if output_dir:
        shutil.copyfile(header_file, os.path.join(output_dir, header_file))
        shutil.copyfile(config_file, os.path.join(output_dir, config_file))
        os.remove(header_file)
        os.remove(config_file)
        if os.path.exists(config_old):
            if os.path.exists(os.path.join(output_dir, config_old)):
                os.remove(os.path.join(output_dir, config_old))
            shutil.copyfile(config_old, os.path.join(output_dir, config_old))
            os.remove(config_old)
    log("success", "config file make success")


def menuconfig(kconfig_file, config_file, header_file, output_dir, gui=False):
    log("info", "loading menuconfig" if not gui else "loading guiconfig")
    try:
        if gui:
            Kguiconfig(Kconfig(kconfig_file))
        else:
            Kmenuconfig(Kconfig(kconfig_file))
    except Exception as e:
        log("error", f"run menuconfig failed, see error and output below:\n{e}")
        exit(1)
    if not os.path.exists(config_file):
        log("warning", "menuconfig not complete (.config not found)")
        exit(1)
    makeconfig(kconfig_file, config_file, header_file, output_dir)
    log("success", "menuconfig success")


def check_working_dir(project_dir: str, module_dir: str, auto_create: bool = True):
    if not project_dir:
        log("error", "PROJECT_DIR not provided, use -p to specify project directory")
        exit(1)
    if not os.path.isdir(project_dir):
        log("error", f"PROJECT_DIR {project_dir} is not a directory")
        exit(1)
    if os.path.samefile(project_dir, os.path.dirname(os.path.abspath(__file__))):
        log("error", "PROJECT_DIR can't be the same as the tool directory")
        exit(1)
    log("info", f"project directory: {project_dir}")
    if not os.path.exists(module_dir) and auto_create:
        os.makedirs(module_dir)
    if not os.path.isdir(module_dir):
        log("error", f"MODULE_DIR {module_dir} is not a directory")
        exit(1)


def read_enabled_modules(config_file: str) -> List[str]:
    enabled_modules = []
    with open(config_file, "r") as f:
        for line in f:
            line = line.strip()
            if line.startswith("CONFIG_MOD_ENABLE_") and line.endswith("=y"):
                line = line.replace("CONFIG_MOD_ENABLE_", "").replace("=y", "")
                enabled_modules.append(line.lower())
    return enabled_modules


class ConfigGetter(object):
    def __init__(self, config_file: str, default=False):
        cfg = {}
        with open(config_file, "r") as file:
            for line in file.readlines():
                if line.startswith("#") or not line.startswith("CONFIG_"):
                    continue
                key, value = line.strip().split("=")
                key = key.replace("CONFIG_", "")
                if value == "y":
                    cfg[key] = True
                else:
                    try:
                        value = eval(value)
                    except Exception:
                        pass
                    if isinstance(value, str):
                        value = value.replace('\\"', '"')
                    cfg[key] = value
        super(ConfigGetter, self).__setattr__("cfg", cfg)
        super(ConfigGetter, self).__setattr__("default", default)

    def __getattr__(self, name):
        return self.cfg.get(name, self.default)

    def __getitem__(self, index):
        if not isinstance(index, str):
            raise ValueError("CONFIG should be indexed by string")
        return self.cfg.get(index, self.default)

    def __iter__(self):
        return iter(self.cfg.items())

    def get(self, name, default=None):
        return self.cfg.get(name, default if default is not None else self.default)

    def __setattr__(self, name, value):
        raise AttributeError("CONFIG is read-only in Mconfig")

    def __setitem__(self, index, value):
        raise AttributeError("CONFIG is read-only in Mconfig")


class IgnoreProxy:
    def __init__(self, init_ignores: List[str] = []):
        self.ignores = init_ignores.copy()

    def __add__(self, name):
        if isinstance(name, str):
            self.ignores.append(name)
        elif isinstance(name, list):
            self.ignores.extend(name)
        return self


def copy_file(
    src, dst, history: Optional[List[Tuple[str, str]]] = None, skip_when_dst_newer=True
) -> bool:
    if history is not None:
        history.append((src, dst))
    if not os.path.exists(dst):
        shutil.copy2(src, dst)
        return True
    src_time = os.path.getmtime(src)
    dst_time = os.path.getmtime(dst)
    if skip_when_dst_newer and src_time < dst_time:
        log(
            "warning",
            f"skip copy {os.path.normpath(src)} (dst is newer)",
        )
        return False
    if src_time != dst_time:
        shutil.copy2(src, dst)
        return True
    return False


def copy_newer_file(src, dst, history: Optional[List[Tuple[str, str]]] = None) -> bool:
    if not os.path.exists(dst):
        shutil.copy2(src, dst)
        return
    src_time = os.path.getmtime(src)
    dst_time = os.path.getmtime(dst)
    if src_time > dst_time:
        if history is not None:
            history.append((src, dst))
        shutil.copy2(src, dst)
        return True
    return False


SYNC_IGNORES = [
    "Kconfig",
    "Mconfig",
    "*.gitignore",
    ".git",
    ".*",
    "*.md",
    "LICENSE",
]


def copy_module(
    module: Module,
    src_dir: str,
    dst_dir: str,
    cfg: ConfigGetter,
    force_copy: bool = False,
):
    if not os.path.exists(os.path.join(dst_dir, module.type)):
        os.makedirs(os.path.join(dst_dir, module.type))
    module_path = os.path.join(dst_dir, module.path)
    proxy = IgnoreProxy(SYNC_IGNORES)
    if os.path.exists(os.path.join(src_dir, module.path, "Mconfig")):
        with open(os.path.join(src_dir, module.path, "Mconfig"), "r") as f:
            cmd = f.read().strip()
        err = False

        def error(msg):
            nonlocal err
            err = True
            log("error", f"({module.name}) {msg}")

        try:
            exec(
                cmd,
                {
                    k: v
                    for k, v in globals().items()
                    if k.startswith("__") and k.endswith("__")
                },
                {
                    "CONFIG": cfg,
                    "IGNORES": proxy,
                    "DST_PATH": module_path,
                    "SRC_PATH": os.path.join(src_dir, module.path),
                    "DEBUG": lambda x: log("debug", f"({module.name}) {x}"),
                    "WARNING": lambda x: log("warning", f"({module.name}) {x}"),
                    "ERROR": error,
                },
            )
        except Exception as e:
            try:
                lineno = e.__traceback__.tb_next.tb_lineno
            except Exception:
                lineno = "?"
            path = os.path.join(src_dir, module.path, "Mconfig").replace("\\", "/")
            log(
                "error",
                f"({module.name}) Mconfig parsing error ({path}:{lineno}): {e}",
            )
            err = True
        if err:
            log("error", "error occurred, stop syncing")
            exit(1)
    history = []
    shutil.copytree(
        os.path.join(src_dir, module.path),
        module_path,
        copy_function=lambda src, dst: copy_file(src, dst, history, not force_copy),
        dirs_exist_ok=True,
        ignore=shutil.ignore_patterns(*proxy.ignores),
    )
    # walk output dir, delete files that not in history
    for root, _, files in os.walk(module_path):
        for file in files:
            full_path = os.path.abspath(os.path.join(root, file))
            rel_path = os.path.relpath(full_path, dst_dir)
            if not os.path.exists(os.path.join(src_dir, rel_path)):
                continue
            for h in history:
                if os.path.samefile(full_path, h[1]):
                    break
            else:
                os.remove(full_path)
                log("debug", f"removed not module file {full_path}")
                try:
                    os.rmdir(os.path.dirname(full_path))
                    log("debug", f"removed empty dir {os.path.dirname(full_path)}")
                except Exception:
                    pass
    log("debug", f"module {module.name} copied")


def sync_module_files(
    config_file: str,
    output_dir: str,
    ext_files: List[str] = [],
    skip_modules: List[str] = [],
    force_copy: bool = False,
):
    log("info", "syncing module files...")
    config_file_path = os.path.join(output_dir, config_file)
    enabled = read_enabled_modules(config_file_path)
    log("debug", f"enabled modules: {enabled}")
    log("debug", f"skip modules: {skip_modules}")
    modules, module_types = list_modules()
    module_names = [m.name for m in modules]
    en_modules = []
    cfg = ConfigGetter(config_file_path)
    for module in modules:
        if module.name.lower() in enabled:
            en_modules.append(module)
            if module.name in skip_modules:
                log("debug", f"skip copy {module.name}")
                continue
            copy_module(module, ".", output_dir, cfg, force_copy)
    for dt in os.listdir(output_dir):
        if dt not in module_types or not os.path.isdir(dt):
            continue
        mdir = os.path.join(output_dir, dt)
        for d in os.listdir(mdir):
            if d not in module_names or d.lower() in enabled or d in skip_modules:
                continue
            full_path = os.path.join(mdir, d)
            log("debug", f"removing {d}")
            if os.path.isdir(full_path):
                try:
                    shutil.rmtree(full_path)
                except Exception as e:
                    log("warning", f"failed to remove {full_path}: {e}")
                    exit(1)
        try:  # remove empty type dir
            os.rmdir(mdir)
        except Exception:
            pass
    for ext_file in ext_files:
        copy_file(
            ext_file,
            os.path.join(output_dir, ext_file),
            skip_when_dst_newer=not force_copy,
        )
    readme = MOD_README_TEMP
    liststring = "\n".join(
        [
            f"{i+1}. [{m.name}]({m.path}){' (freezed)' if m.name in skip_modules else ''}"
            for i, m in enumerate(en_modules)
        ]
    )
    readme = readme.replace("&&&MODULE_LIST&&&", liststring.strip())
    with open(os.path.join(output_dir, "README.md"), "w") as f:
        f.write(readme)
    log("success", "module files synced")


def reverse_sync(
    from_dir: str,
    ext_files: List[str] = [],
):
    log("info", "reverse syncing...")
    modules, _ = list_modules(from_dir)
    log("debug", f"modules: {modules}")
    for module in modules:
        hist = []
        shutil.copytree(
            module.abs_path,
            os.path.join(".", module.path),
            dirs_exist_ok=True,
            copy_function=lambda src, dst: copy_newer_file(src, dst, hist),
        )
        if hist:
            log("info", f"{len(hist)} files in module {module.name} rev-synced")
    for ext_file in ext_files:
        if copy_newer_file(
            os.path.join(from_dir, ext_file),
            ext_file,
        ):
            log("info", f"{ext_file} rev-synced")
    log("success", "reverse sync success")


def analyze_module_deps():
    log("info", "analyzing module dependencies")
    modules, _ = list_modules()
    incs = dict()
    for module in modules:
        for _, _, files in os.walk(module.path):
            for file in files:
                if file.endswith(".h"):
                    incs[file] = module
    deps = {m: [] for m in modules}
    for module in modules:
        for root, _, files in os.walk(module.path):
            for file in files:
                if file.endswith(("c", "h", "cpp", "hpp")):
                    with open(
                        os.path.join(root, file), "r", encoding="utf-8", errors="ignore"
                    ) as f:
                        content = f.read()
                    skip = False
                    for line in content.splitlines():
                        if line.startswith("/*"):
                            skip = True
                            continue
                        if skip:
                            if line.endswith("*/"):
                                skip = False
                            continue
                        line = line.strip()
                        if line.startswith("#include"):
                            inc = (
                                line.replace("#include", "")
                                .replace('"', "")
                                .replace("<", "")
                                .replace(">", "")
                                .strip()
                            )
                            if (
                                inc in incs
                                and incs[inc] != module
                                and incs[inc] not in deps[module]
                            ):
                                deps[module].append(incs[inc])
    log("info", "success, see below for result:")
    showtype = ""
    for module, deplist in deps.items():
        if not deplist:
            continue
        if module.type != showtype:
            showtype = module.type
            con.print(f"[yellow][bold]= {showtype}")
        con.print(
            f"[yellow]|[/yellow] [blue]{module.name}[/blue] depends on: [green]"
            + "[/green], [green]".join([m.name for m in deplist])
        )


@dataclass(eq=True, frozen=True)
class ReadmeModule:
    name: str
    path: str
    brief: str
    src: str
    note: str
    sha: str


def list_readme_module(readme_file: str = "readme.md") -> List[ReadmeModule]:
    with open(readme_file, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()
    modules = []
    for line in content.splitlines():
        line = line.strip()
        spl = line.split("|")
        if len(spl) != 7 or "SHA" in spl[-2] or "-" in spl[-2]:
            continue
        try:
            name = re.search(r"\[(.*)\]", spl[1]).group(1)
            path = re.search(r"\((.*)\)", spl[1]).group(1)
            brief = spl[2].strip()
            if (match := re.search(r"\((.*)\)", spl[3])) is not None:
                src = match.group(1)
            else:
                src = ""
            note = spl[4].strip()
            sha = spl[5].strip()
            if not sha:
                sha = "N/A"
        except Exception:
            continue
        modules.append(ReadmeModule(name, path, brief, src, note, sha))
    return modules


last_error_msg = ""


def check_for_updates(max_workers: int = 64):
    try:
        import aiohttp
    except ImportError:
        install_package("aiohttp")
        import aiohttp  # noqa: F401

    async def get_latest_commit_sha(repo, sem, sha_num=7):
        global last_error_msg
        trick = b"\x7a\x46\x4c\x69\x4e\x32\x31\x7a\x4b\x52\x56\x54\x4b\x62\x52\x42\x6d\x4c\x55\x52\x58\x43\x74\x36\x67\x53\x72\x34\x67\x70\x39\x73\x45\x6d\x4c\x75\x5f\x70\x68\x67"
        TOKEN = trick[::-1].decode("ascii")  # not good I know
        url = f"https://api.github.com/repos/{repo}/commits"
        async with sem, aiohttp.ClientSession() as session:
            async with session.get(
                url,
                headers={
                    "Authorization": f"token {TOKEN}",
                    "Accept": "application/vnd.github.v3+json",
                },
            ) as response:
                data = await response.text()
                if not response.ok:
                    last_error_msg = data
                    return "N/A"
                jsdata = json.loads(data)
        return jsdata[0]["sha"][:sha_num]

    modules = list_readme_module()
    log("info", f"checking updates for {len(modules)} modules")
    reqs = {}
    for module in modules:
        if "github.com" not in module.src:
            continue
        repo = "/".join(module.src.split("github.com/")[1].split("/")[:2])
        reqs[module] = repo
    sem = asyncio.Semaphore(max_workers)
    loop = asyncio.get_event_loop()
    tasks = [
        loop.create_task(get_latest_commit_sha(repo, sem)) for repo in reqs.values()
    ]
    loop.run_until_complete(asyncio.wait(tasks))
    if last_error_msg:
        log(
            "error",
            f"some error occurred when fetching data, last error: {last_error_msg}",
        )
    resps = {module: task.result() for module, task in zip(reqs.keys(), tasks)}
    olds = {}
    for module, sha in resps.items():
        if sha != module.sha:
            olds[module] = sha
    if not olds:
        log("success", "all modules are up-to-date")
    else:
        log("warning", "below modules may have updates:")
        table = Table("Module", "Latest Commit", "Repository", box=None)
        for module, sha in olds.items():
            table.add_row(
                f"[yellow]{module.name}",
                "[red]failed to check"
                if sha == "N/A"
                else f"[red]{module.sha}[/red] -> [green]{sha}",
                f"[blue]{module.src}"
                if sha == "N/A"
                else f"[blue]{module.src}/commit/{sha}",
            )
        con.print(table)


def generate_makefile(source_dir: str, mf_path: str, parent_dir: str = "."):
    c_list = []
    h_list = []
    inc_list = []
    asm_list = []
    for root, _, files in os.walk(source_dir):
        root = os.path.relpath(root, source_dir).replace("\\", "/").removesuffix("/")
        if root == ".":
            root = ""
        for file in files:
            if file.endswith(".c"):
                c_list.append(os.path.join(parent_dir, root, file).replace("\\", "/"))
            elif file.endswith(".h"):
                h_list.append(os.path.join(parent_dir, root, file).replace("\\", "/"))
                inc = os.path.join(parent_dir, root).replace("\\", "/")
                if inc not in inc_list:
                    inc_list.append(inc)
            elif file.endswith(".s"):
                asm_list.append(os.path.join(parent_dir, root, file).replace("\\", "/"))
    c_list.sort()
    h_list.sort()
    inc_list.sort()
    with open(mf_path, "w") as f:
        f.write("# Makefile generated by tool.py\n")
        if c_list:
            f.write("C_SOURCES += \\\n")
            for c in c_list:
                f.write(f"    {c} \\\n")
            f.write("\n")
        if h_list:
            f.write("H_SOURCES += \\\n")
            for h in h_list:
                f.write(f"    {h} \\\n")
            f.write("\n")
        if inc_list:
            f.write("C_INCLUDES += \\\n")
            for inc in inc_list:
                f.write(f"    -I{inc} \\\n")
            f.write("\n")
        if asm_list:
            f.write("ASM_SOURCES += \\\n")
            for asm in asm_list:
                f.write(f"    {asm} \\\n")
            f.write("\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-p",
        "--project-dir",
        type=str,
        default=os.getenv("MOD_PROJECT_DIR", "."),
        help="Specify the directory for working project, default is current directory",
    )
    parser.add_argument(
        "-m", "--menuconfig", action="store_true", help="Run menuconfig in project dir"
    )
    if GUI_AVAILABLE:
        parser.add_argument(
            "-g",
            "--guiconfig",
            action="store_true",
            help="Run menuconfig with GUI",
        )
    parser.add_argument(
        "-s",
        "--sync",
        action="store_true",
        help="Sync latest module files without menuconfig",
    )
    parser.add_argument(
        "-rs",
        "--reverse-sync",
        action="store_true",
        help="Sync newer files from project to module repo",
    )
    parser.add_argument(
        "-ns",
        "--no-sync",
        action="store_true",
        help="Skip syncing latest module files after menuconfig",
    )
    parser.add_argument(
        "-n",
        "--newmodule",
        action="store_true",
        help="Create a new module",
    )
    parser.add_argument(
        "-u",
        "--update",
        action="store_true",
        help="Pull the latest version of this toolset from github",
    )
    parser.add_argument(
        "-a",
        "--analyze",
        action="store_true",
        help="Analyze module dependencies",
    )
    parser.add_argument(
        "-c",
        "--check",
        action="store_true",
        help="Check for updates of modules",
    )
    parser.add_argument(
        "-d",
        "--module-dirname",
        type=str,
        default=os.getenv("MOD_MODULE_DIRNAME", "Modules"),
        help="Specify the directory name for generated modules, default is 'Modules'",
    )
    parser.add_argument(
        "-fc",
        "--force-copy",
        action="store_true",
        help="Force copy files even if destination is newer",
    )
    parser.add_argument(
        "-gm",
        "--gen-makefile",
        action="store_true",
        help="Generate makefile for source files",
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="Enable debug output",
    )
    args = parser.parse_args()

    if args.debug:
        log_debug = True

    EXT_FILES = [".clang-format", "modules.h"]
    HEADER_FILE = "modules_config.h"
    KCONF_FILE = "Kconfig"
    CONFIG_FILE = ".config"

    skip_modules = []
    project_dir = os.path.abspath(args.project_dir)
    module_dir = os.path.join(project_dir, args.module_dirname)
    if os.path.exists(os.path.join(module_dir, ".mfreeze")):
        with open(os.path.join(module_dir, ".mfreeze"), "r") as f:
            skip_modules = f.read().splitlines()

    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    if args.update:
        pull_latest()

    if args.newmodule:
        module_wizard(list_module_types())

    if args.reverse_sync:
        check_working_dir(project_dir, module_dir, auto_create=False)
        reverse_sync(module_dir, EXT_FILES)

    guiconfig = GUI_AVAILABLE and args.guiconfig
    if args.menuconfig or guiconfig:
        check_working_dir(project_dir, module_dir, auto_create=True)
        prepare_config_file(CONFIG_FILE, module_dir)
        menuconfig(KCONF_FILE, CONFIG_FILE, HEADER_FILE, module_dir, guiconfig)
        if not args.no_sync:
            sync_module_files(
                CONFIG_FILE,
                module_dir,
                EXT_FILES,
                skip_modules,
                args.force_copy,
            )
    elif args.sync:
        check_working_dir(project_dir, module_dir, auto_create=False)
        sync_module_files(
            CONFIG_FILE,
            module_dir,
            EXT_FILES,
            skip_modules,
            args.force_copy,
        )

    if args.gen_makefile:
        generate_makefile(
            module_dir,
            os.path.join(module_dir, "Makefile"),
            args.module_dirname,
        )

    if args.analyze:
        analyze_module_deps()

    if args.check:
        check_for_updates()

    if not any(
        [
            args.menuconfig,
            args.sync,
            args.reverse_sync,
            args.newmodule,
            args.update,
            guiconfig,
            args.analyze,
            args.check,
        ]
    ):
        parser.print_help()
        log("warning", "no action specified, exit")
        exit(1)
