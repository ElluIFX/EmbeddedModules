import datetime
import os
import shutil
import sys

from rich.console import Console
from rich.prompt import Confirm, Prompt
from rich.table import Table

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

// Public Variables -------------------------

// Private Variables ------------------------

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

// Exported Variables -----------------------

// Exported Macros --------------------------

// Exported Functions -----------------------

#ifdef __cplusplus
}
#endif

#endif /* __&&&FILE_NAME_UPPER&&&__ */

"""

README_TEMP = """# Module: &&&MODULE_NAME&&&

&&&MODULE_BRIEF&&&

## 1. Introduction

## 2. Notice

## 3. Usage

"""


AVAILABLE_TYPES = [
    "none",
    "algorithm",
    "data",
    "graphics",
    "peripheral",
    "system",
    "test",
    "utility",
]

module_root = os.path.dirname(os.path.abspath(__file__))
con = Console()

con.print(f"[blue]Module root: [green]{module_root}")

module_type = Prompt.ask(
    "[yellow]Please select module type",
    choices=AVAILABLE_TYPES,
    default="none",
)

module_name = Prompt.ask("[yellow]Please input module name")
module_name = module_name.strip().capitalize()

module_brief = Prompt.ask("[yellow]Please input module brief")
module_brief = module_brief.strip().capitalize()

module_author = "Ellu" if os.getenv("USER") is None else str(os.getenv("USER"))
module_email = (
    "ellu.grif@gmail.com" if os.getenv("EMAIL") is None else str(os.getenv("EMAIL"))
)
module_version = "1.0.0" if os.getenv("VERSION") is None else str(os.getenv("VERSION"))

create_readme_conf = Confirm.ask("[yellow]Create README.md?", default=True)

module_date = datetime.datetime.now().strftime("%Y-%m-%d")
module_file_name = module_name.replace(" ", "_").lower()

table = Table(show_header=False, box=None)

table.add_row("[blue]Module name", ":", f"[green]{module_name}")
table.add_row("[blue]Module brief", ":", f"[green]{module_brief}")
table.add_row("[blue]Module type", ":", f"[green]{module_type}")
table.add_row("[blue]Module date", ":", f"[green]{module_date}")
table.add_row("[blue]Module filename", ":", f"[green]{module_file_name}")
table.add_row("[blue]Module author", ":", f"[green]{module_author}")
table.add_row("[blue]Module email", ":", f"[green]{module_email}")
table.add_row("[blue]Module version", ":", f"[green]{module_version}")

con.print(table)

conf = Confirm.ask("[yellow]Confirm to create module?", default=True)
if not conf:
    sys.exit(0)

# Create module folder
if module_type != "none":
    module_path = os.path.join(module_root, module_type, module_file_name)
else:
    module_path = os.path.join(module_root, module_file_name)
if os.path.exists(module_path):
    con.print(f"[red]Module dir already exists: [yellow]{module_path}")
    conf_del = Confirm.ask(
        "[red]Purge existing module? [yellow](DANGER) ", default=False
    )
    if not conf_del:
        sys.exit(0)
    shutil.rmtree(module_path)
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
        "&&&MODULE_BRIEF&&&", module_brief
    )
    with open(readme_file_path, "w", encoding="utf-8") as f:
        f.write(readme_content)

con.print("[green]Module created.")
con.print(f"[green]C file at: [yellow]{c_file_path}")
con.print(f"[green]H file at: [yellow]{h_file_path}")
if create_readme_conf:
    con.print(f"[green]README file at: [yellow]{readme_file_path}")
