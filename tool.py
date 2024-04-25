import argparse
import datetime
import os
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
    log("warning", "guiconfig not available, tkinter not found")


@dataclass
class Module:
    name: str
    type: str
    path: str
    Mconfig: bool
    Kconfig: bool


@lru_cache()
def list_module_types() -> List[str]:
    module_types = [
        d
        for d in os.listdir(".")
        if (os.path.isdir(d) and not d.startswith("_") and not d.startswith("."))
    ]
    module_types.sort()
    return module_types


@lru_cache()
def list_modules() -> List[Module]:
    # list 1st level dirs
    modules = []
    module_types = list_module_types()
    for module_type in module_types:
        module_dirs = [
            d
            for d in os.listdir(module_type)
            if (
                os.path.isdir(f"{module_type}/{d}")
                and not d.startswith("_")
                and not d.startswith(".")
            )
        ]
        module_dirs.sort()
        for module_dir in module_dirs:
            full_path = f"{module_type}/{module_dir}"
            Mconfig = os.path.exists(os.path.join(full_path, "Mconfig"))
            Kconfig = os.path.exists(os.path.join(full_path, "Kconfig"))
            modules.append(Module(module_dir, module_type, full_path, Mconfig, Kconfig))
    return modules


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

    temp_list = [m.name for m in list_modules() if m.type == module_type]
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
                        cfg[key] = eval(value)
                    except Exception:
                        cfg[key] = value
        super(ConfigGetter, self).__setattr__("cfg", cfg)
        super(ConfigGetter, self).__setattr__("default", default)

    def __getattr__(self, name):
        return self.cfg.get(name, self.default)

    def get(self, name, default=None):
        return self.cfg.get(name, default if default is not None else self.default)

    def __setattr__(self, name, value) -> None:
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


def copy_file(src, dst, history: Optional[List[Tuple[str, str]]] = None):
    if history is not None:
        history.append((src, dst))
    if not os.path.exists(dst):
        shutil.copy2(src, dst)
        return
    src_time = os.path.getmtime(src)
    dst_time = os.path.getmtime(dst)
    if src_time != dst_time:
        shutil.copy2(src, dst)


SYNC_IGNORES = [
    "Kconfig",
    "Mconfig",
    "*.gitignore",
    ".git",
    ".*",
    "*.md",
    "LICENSE",
]


def copy_module(module: Module, src_dir: str, dst_dir: str, cfg: ConfigGetter):
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
        copy_function=lambda src, dst: copy_file(src, dst, history),
        dirs_exist_ok=True,
        ignore=shutil.ignore_patterns(*proxy.ignores),
    )
    # walk output dir, delete files that not in history
    for root, _, files in os.walk(module_path):
        for file in files:
            full_path = os.path.abspath(os.path.join(root, file))
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
):
    config_file_path = os.path.join(output_dir, config_file)
    enabled = read_enabled_modules(config_file_path)
    log("debug", f"enabled modules: {enabled}")
    log("debug", f"skip modules: {skip_modules}")
    log("info", "syncing module files...")
    modules = list_modules()
    module_types = list_module_types()
    module_names = [m.name for m in list_modules()]
    en_modules = []
    cfg = ConfigGetter(config_file_path)
    for module in modules:
        if module.name.lower() in enabled:
            en_modules.append(module)
            if module.name in skip_modules:
                log("debug", f"skip copy {module.name}")
                continue
            copy_module(module, ".", output_dir, cfg)
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
        copy_file(ext_file, os.path.join(output_dir, ext_file))
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
        "-ns",
        "--nosync",
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
        "-d",
        "--module-dirname",
        type=str,
        default=os.getenv("MOD_MODULE_DIRNAME", "Modules"),
        help="Specify the directory name for generated modules, default is 'Modules'",
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
    SKIP_MODULES = []
    HEADER_FILE = "modules_config.h"
    KCONF_FILE = "Kconfig"
    CONFIG_FILE = ".config"
    GUI_CONFIG = GUI_AVAILABLE and args.guiconfig
    PROJECT_DIR = os.path.abspath(args.project_dir)
    if os.path.samefile(PROJECT_DIR, os.path.dirname(os.path.abspath(__file__))):
        log("error", "PROJECT_DIR can't be the same as the tool directory")
        exit(1)
    MODULE_DIR = os.path.join(PROJECT_DIR, args.module_dirname)
    if os.path.exists(os.path.join(MODULE_DIR, ".mfreeze")):
        with open(os.path.join(MODULE_DIR, ".mfreeze"), "r") as f:
            SKIP_MODULES = f.read().splitlines()

    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    if args.update:
        pull_latest()

    if args.newmodule:
        module_wizard(list_module_types())

    if args.menuconfig or GUI_CONFIG:
        check_working_dir(PROJECT_DIR, MODULE_DIR, auto_create=True)
        prepare_config_file(CONFIG_FILE, MODULE_DIR)
        menuconfig(KCONF_FILE, CONFIG_FILE, HEADER_FILE, MODULE_DIR, GUI_CONFIG)
        if not args.nosync:
            sync_module_files(CONFIG_FILE, MODULE_DIR, EXT_FILES, SKIP_MODULES)
        log("success", "menuconfig success")
    elif args.sync:
        check_working_dir(PROJECT_DIR, MODULE_DIR, auto_create=False)
        sync_module_files(CONFIG_FILE, MODULE_DIR, EXT_FILES, SKIP_MODULES)

    if not any([args.menuconfig, args.sync, args.newmodule, args.update, GUI_CONFIG]):
        parser.print_help()
        log("warning", "no action specified, exit")
        exit(1)
