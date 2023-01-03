from tools.sbs import project, Rule, Target, removesuffix, hassuffix, ExecutableTarget
from glob import glob
from typing import List
import os
import posixpath
from pathlib import Path

#
# Dependencies
#

if project.platform.is_windows():
    project.add_system_libs("advapi32.lib", "user32.lib")

project.add_static_lib(
    name="fmt",
    dir="deps/fmt",
    sources=["src/format.cc", "src/os.cc"],
    defines=["FMT_EXCEPTIONS=0"],
    includepaths=["include"],
)

project.add_static_lib(
    name="glm",
    dir="deps/glm",
    sources=["glm/detail/glm.cpp"],
    includepaths=["."]
)

project.add_static_lib(
    name="imgui",
    dir="deps/imgui",
    sources=["imgui.cpp", "imgui_demo.cpp", "imgui_draw.cpp", "imgui_tables.cpp", "imgui_widgets.cpp"],
    includepaths=["."]
)

project.add_headeronly_lib(
    name="parallel-hashmap",
    dir="deps/parallel-hashmap",
    includepaths=["."]
)

physfs_sources = glob("src/*.c", root_dir="deps/physfs")
if project.platform.is_macos():
    physfs_sources.append("src/physfs_platform_apple.m")

project.add_static_lib(
    name="physfs",
    dir="deps/physfs",
    sources=physfs_sources,
    includepaths=["src"]
)

project.add_headeronly_lib(
    name="sokol",
    dir="deps/sokol",
    includepaths=[".", "util"]
)

project.add_headeronly_lib(
    name="sokol_gp",
    dir="deps/sokol_gp",
    includepaths=["."]
)

project.add_static_lib(
    name="pugixml",
    dir="deps/pugixml",
    sources=["src/pugixml.cpp"],
    defines=["PUGIXML_NO_EXCEPTIONS"],
    includepaths=["src/"],
)

project.add_headeronly_lib(
    name="rapidjson",
    dir="deps/rapidjson",
    includepaths=["include"]
)

project.add_static_lib(
    name="quirrel",
    dir="deps/quirrel",
    sources=glob("squirrel/*.cpp", root_dir="deps/quirrel") + glob("sqstdlib/*.cpp", root_dir="deps/quirrel"),
    includepaths=["include", "squirrel", "sqstdlib"]
)

project.add_headeronly_lib(
    name="doctest",
    dir="deps/doctest",
    includepaths=["doctest"]
)

project.add_static_lib(
    name="tracy",
    dir="deps/tracy",
    sources=["TracyClient.cpp"],
    # defines=["TRACY_ENABLE"],
    includepaths=["."]
)

#
# Engine
#

class ShdcGenRule(Rule):
    def __init__(self):
        if project.platform.is_windows():
            self.sokol_shdc_bin = "deps/sokol-tools-bin/bin/win32/sokol-shdc.exe"
        elif project.platform.is_macos():
            self.sokol_shdc_bin = "deps/sokol-tools-bin/bin/osx/sokol-shdc"
        elif project.platform.is_linux():
            self.sokol_shdc_bin = "deps/sokol-tools-bin/bin/linux/sokol-shdc"
        else:
            raise RuntimeError(f"Platform {str(project.platform)} not supported")

    def emit(self):
        project.ninja.rule(
            name="sokol-shdc",
            command=f"{self.sokol_shdc_bin} --input $in --output $out --slang glsl330:hlsl5:metal_macos",
            description="Generate headers from GLSL files via sokol-shdc"
        )

class ShdcGenTarget(Target):
    def __init__(self, glsl_files: List[str]):
        self.glsl_files = glsl_files
        self.headers = [file + ".h" for file in glsl_files]

    def get_name(self):
        return "shdcgen"

    def get_outputs(self):
        return self.headers

    def emit(self):
        project.ninja.build(
            outputs=self.headers,
            rule="sokol-shdc",
            inputs=self.glsl_files)

class ClassDbRule(Rule):
    def __init__(self):
        if project.platform.is_windows():
            self.mpp_bin = "tools/mpp.exe"
        else:
            raise RuntimeError(f"Platform {str(project.platform)} not supported")

    def emit(self):
        project.ninja.rule(
            name="classdb",
            command=f"{self.mpp_bin} --input $in $out"
        )

class ClassDbTarget(Target):
    def get_name(self):
        return "classdb"

    def get_outputs(self):
        return ["engine/classdb.json"]

    def emit(self):
        project.ninja.build(
            outputs=self.get_outputs(),
            rule="classdb",
            inputs=["engine/reflected_headers.h"],
            implicit=["compile_commands.json"]
        )

class ClassDbCodegenRule(Rule):
    def emit(self):
        project.ninja.rule(
            name="classdb-codegen",
            command=f"python tools/codegen.py $in"
        )

class ClassDbCodegenTarget(Target):
    def __init__(self, templates: List[str]):
        self.templates = templates
        self.generated_files = [removesuffix(template, ".jinja")
                                for template in templates if hassuffix(template, ".jinja")]

    def get_name(self):
        return "classdb-codegen"

    def get_outputs(self):
        return self.generated_files

    def emit(self):
        for tmpl, gen in zip(self.templates, self.generated_files):
            project.ninja.build(
                outputs=[gen],
                rule="classdb-codegen",
                inputs=["engine/classdb.json", tmpl],
            )
        project.ninja.build(
            outputs="classdb-codegen",
            rule="phony",
            inputs=self.generated_files
        )

class ZipRule(Rule):
    def emit(self):
        project.ninja.rule(
            name="zip",
            command=f"7z a $out ./$in/*"
        )

class CompileAssetsTarget(Target):
    def get_name(self):
        return "compile-assets"

    def get_outputs(self):
        return [f"$builddir/assets.zip"]

    def emit(self):
        project.ninja.build(
            outputs=self.get_outputs(),
            rule="zip",
            inputs=["assets"]
        )
        project.ninja.build(self.get_name(), 'phony', self.get_outputs())

class AddAssetsToExeRule(Rule):
    def emit(self):
        project.ninja.rule(
            name="add-data-to-exe",
            command=f"python tools/exe_packer.py $in $builddir/assets.zip $out"
        )

class AddAssetsToExeTarget(Target):
    def __init__(self, exe_file):
        self.exe_path = Path(exe_file)
        self.modified_exe_path = self.exe_path.with_stem(self.exe_path.stem + "_standalone")

    def get_name(self):
        return "add-data-to-exe"

    def get_outputs(self):
        return [str(self.modified_exe_path)]

    def emit(self):
        project.ninja.build(
            outputs=self.get_outputs(),
            rule="add-data-to-exe",
            inputs=[str(self.exe_path)]
        )

project.add_custom_rules(
    [ShdcGenRule(), ClassDbRule(), ClassDbCodegenRule(), ZipRule(), AddAssetsToExeRule()]
)

project.add_custom_targets([
    ClassDbTarget(),
    ShdcGenTarget(glsl_files=[
        "engine/shaders/sprite.glsl",
    ]),
    ClassDbCodegenTarget(templates=[
        "engine/reflection.h.jinja",
        "engine/resources.h.jinja",
        "engine/resources.cpp.jinja"]
    ),
    CompileAssetsTarget(),
])

engine_sources = \
    glob("*.cpp", root_dir="engine") + \
    glob("core/*.c", root_dir="engine") + \
    glob("core/*.cpp", root_dir="engine") + \
    glob("render/*.cpp", root_dir="engine") + \
    glob("collision/*.cpp", root_dir="engine") + \
    glob("squirrel/*.cpp", root_dir="engine") + \
    glob("stb/*.c", root_dir="engine")

if project.platform.is_macos():
    engine_sources.append("sokol/sokol_impl.mm")
else:
    engine_sources.append("sokol/sokol_impl.cpp")

project.add_static_lib(
    name="engine",
    dir="engine",
    sources=engine_sources,
    includepaths=["."],
    deps=["sokol", "sokol_gp", "glm", "fmt", "parallel-hashmap",
          "physfs", "pugixml", "rapidjson", "quirrel", "imgui", "tracy",
          "classdb-codegen"
          ]
)

# project.add_executable(
#     name="engine_tests",
#     dir="engine",
#     sources=["tests/test_main.cpp", "tests/test_resource_pool.cpp"],
#     includepaths=["."],
#     deps=["engine", "doctest"],
#     windows_subsystem="console"
# )

thesystem_exe_target = ExecutableTarget(
    name="thesystem",
    dir="game",
    sources=["main.cpp"],
    includepaths=["."],
    deps=["engine"],
    defines=["EXE_EMBEDDED_ASSETS"] if project.mode == "release" else [],
    windows_subsystem="windows"
)
project.add_custom_target(thesystem_exe_target)

if project.mode == "release":
    thesystem_exe = thesystem_exe_target.get_outputs()[0]
    project.add_custom_target(
        AddAssetsToExeTarget(thesystem_exe)
    )

project.configure()