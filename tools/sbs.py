import os
import pipes
import string
import subprocess
import sys
import posixpath
from optparse import OptionParser
from typing import List, Dict
from abc import ABC, abstractmethod
import json
from pathlib import Path

#
# Taken from the Ninja project. (ninja_syntax.py)
#
# Copyright 2011 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Python module for generating .ninja files.

Note that this is emphatically not a required piece of Ninja; it's
just a helpful utility for build-file-generation systems that already
use Python.
"""

import re
import textwrap

def escape_path(word):
    return word.replace('$ ', '$$ ').replace(' ', '$ ').replace(':', '$:')

class NinjaWriter(object):
    def __init__(self, output, width=78):
        self.output = output
        self.width = width

    def newline(self):
        self.output.write('\n')

    def comment(self, text):
        for line in textwrap.wrap(text, self.width - 2, break_long_words=False,
                                  break_on_hyphens=False):
            self.output.write('# ' + line + '\n')

    def variable(self, key, value, indent=0):
        if value is None:
            return
        if isinstance(value, list):
            value = ' '.join(filter(None, value))  # Filter out empty strings.
        self._line('%s = %s' % (key, value), indent)

    def pool(self, name, depth):
        self._line('pool %s' % name)
        self.variable('depth', depth, indent=1)

    def rule(self, name, command, description=None, depfile=None,
             generator=False, pool=None, restat=False, rspfile=None,
             rspfile_content=None, deps=None):
        self._line('rule %s' % name)
        self.variable('command', command, indent=1)
        if description:
            self.variable('description', description, indent=1)
        if depfile:
            self.variable('depfile', depfile, indent=1)
        if generator:
            self.variable('generator', '1', indent=1)
        if pool:
            self.variable('pool', pool, indent=1)
        if restat:
            self.variable('restat', '1', indent=1)
        if rspfile:
            self.variable('rspfile', rspfile, indent=1)
        if rspfile_content:
            self.variable('rspfile_content', rspfile_content, indent=1)
        if deps:
            self.variable('deps', deps, indent=1)

    def build(self, outputs, rule, inputs=None, implicit=None, order_only=None,
              variables=None, implicit_outputs=None, pool=None, dyndep=None):
        outputs = as_list(outputs)
        out_outputs = [escape_path(x) for x in outputs]
        all_inputs = [escape_path(x) for x in as_list(inputs)]

        if implicit:
            implicit = [escape_path(x) for x in as_list(implicit)]
            all_inputs.append('|')
            all_inputs.extend(implicit)
        if order_only:
            order_only = [escape_path(x) for x in as_list(order_only)]
            all_inputs.append('||')
            all_inputs.extend(order_only)
        if implicit_outputs:
            implicit_outputs = [escape_path(x)
                                for x in as_list(implicit_outputs)]
            out_outputs.append('|')
            out_outputs.extend(implicit_outputs)

        self._line('build %s: %s' % (' '.join(out_outputs),
                                     ' '.join([rule] + all_inputs)))
        if pool is not None:
            self._line('  pool = %s' % pool)
        if dyndep is not None:
            self._line('  dyndep = %s' % dyndep)

        if variables:
            if isinstance(variables, dict):
                iterator = iter(variables.items())
            else:
                iterator = iter(variables)

            for key, val in iterator:
                self.variable(key, val, indent=1)

        return outputs

    def include(self, path):
        self._line('include %s' % path)

    def subninja(self, path):
        self._line('subninja %s' % path)

    def default(self, paths):
        self._line('default %s' % ' '.join(as_list(paths)))

    def _count_dollars_before_index(self, s, i):
        """Returns the number of '$' characters right in front of s[i]."""
        dollar_count = 0
        dollar_index = i - 1
        while dollar_index > 0 and s[dollar_index] == '$':
            dollar_count += 1
            dollar_index -= 1
        return dollar_count

    def _line(self, text, indent=0):
        """Write 'text' word-wrapped at self.width characters."""
        leading_space = '  ' * indent
        while len(leading_space) + len(text) > self.width:
            # The text is too wide; wrap if possible.

            # Find the rightmost space that would obey our width constraint and
            # that's not an escaped space.
            available_space = self.width - len(leading_space) - len(' $')
            space = available_space
            while True:
                space = text.rfind(' ', 0, space)
                if (space < 0 or
                        self._count_dollars_before_index(text, space) % 2 == 0):
                    break

            if space < 0:
                # No such space; just use the first unescaped space we can find.
                space = available_space - 1
                while True:
                    space = text.find(' ', space + 1)
                    if (space < 0 or
                            self._count_dollars_before_index(text, space) % 2 == 0):
                        break
            if space < 0:
                # Give up on breaking.
                break

            self.output.write(leading_space + text[0:space] + ' $\n')
            text = text[space+1:]

            # Subsequent lines are continuations, so indent them.
            leading_space = '  ' * (indent+2)

        self.output.write(leading_space + text + '\n')

    def close(self):
        self.output.close()


def as_list(input):
    if input is None:
        return []
    if isinstance(input, list):
        return input
    return [input]


def escape(string):
    """Escape a string such that it can be embedded into a Ninja file without
    further interpretation."""
    assert '\n' not in string, 'Ninja syntax does not allow newlines'
    # We only have one special metacharacter: '$'.
    return string.replace('$', '$$')


def expand(string, vars, local_vars={}):
    """Expand a string containing $vars as Ninja would.

    Note: doesn't handle the full Ninja variable syntax, but it's enough
    to make configure.py's use of it work.
    """
    def exp(m):
        var = m.group(1)
        if var == '$':
            return '$'
        return local_vars.get(var, vars.get(var, ''))
    return re.sub(r'\$(\$|\w*)', exp, string)

# end of ninja_syntax.py

def hassuffix(s: str, suffix):
    return s[-len(suffix):] == suffix

def removesuffix(s: str, suffix):
    if s[-len(suffix):] == suffix:
        return s[:-len(suffix)]
    else:
        return s

def removesuffixes(s: str, suffixes: List[str]):
    for suffix in suffixes:
        s_removed = removesuffix(s, suffix)
        if s != s_removed:
            return s_removed
    return s

def clean_windows_path(s):
    return s.replace(os.sep, posixpath.sep)

def clean_windows_paths(lst):
    return [clean_windows_path(s) for s in lst]

def get_filetype(filename):
    if filename.endswith(".cpp") or filename.endswith(".c"):
        return "source"
    elif filename.endswith(".h") or filename.endswith(".hpp") or filename.endswith(".hh"):
        return "header"
    elif filename.endswith(".o") or filename.endswith(".obj"):
        return "object"
    elif filename.endswith(".a") or filename.endswith(".lib"):
        return "staticlib"
    elif filename.endswith(".so") or filename.endswith(".dll"):
        return "sharedlib"

def get_platform_objfile_ext():
    if project.platform.is_windows(): return ".obj"
    else: return ".o"

class Platform(object):
    """Represents a host/target platform and its specific build attributes."""
    def __init__(self, platform):
        self._platform = platform
        if self._platform is not None:
            return
        self._platform = sys.platform
        if self._platform.startswith('linux'):
            self._platform = 'linux'
        elif self._platform.startswith('darwin'):
            self._platform = 'macos'
        elif self._platform.startswith('win'):
            self._platform = 'windows'

    def __str__(self):
        return self._platform

    @staticmethod
    def known_platforms():
        return ['windows', 'macos', 'linux']

    def platform(self):
        return self._platform

    def is_windows(self):
        return self._platform == 'windows'

    def is_macos(self):
        return self._platform == 'macos'

    def is_linux(self):
        return self._platform == 'linux'

    def can_rebuild_in_place(self):
        return not self.is_windows()

class Rule(ABC):
    @abstractmethod
    def emit(self):
        pass

class CRule(Rule):
    def emit(self):
        if project.platform.is_windows():
            project.ninja.rule(f"cc",
                               command='$cc $cflags -c $in /Fo$out /Fd' + project.built_lib('$pdb'),
                               description='CC $out',
                               deps='msvc'
                               )
        else:
            project.ninja.rule(f"cc",
                               command='$cc -MMD -MT $out -MF $out.d $cflags -c $in -o $out',
                               depfile='$out.d',
                               deps='clang',
                               description='CC $out')


class CxxRule(Rule):
    def emit(self):
        if project.platform.is_windows():
            project.ninja.rule(f"cxx",
                               command='$cxx $cxxflags -c $in /Fo$out /Fd' + project.built_lib('$pdb'),
                               description='CXX $out',
                               deps='msvc'
                               )
        else:
            project.ninja.rule(f"cxx",
                               command='$cxx -MMD -MT $out -MF $out.d $cxxflags -c $in -o $out',
                               depfile='$out.d',
                               deps='clang',
                               description='CXX $out')

class ArchiveRule(Rule):
    def emit(self):
        if project.host.is_windows():
            project.ninja.rule('ar',
                               command='llvm-lib /nologo /out:$out $in',
                               description='LIB $out')
        else:
            project.ninja.rule('ar',
                               command='rm -f $out && $ar crs $out $in',
                               description='AR $out')

class LinkRule(Rule):
    def emit(self):
        if project.host.is_windows():
            project.ninja.rule('link-console',
                               command='$cxx $in $libs /nologo /link /subsystem:console /entry:main $ldflags /out:$out',
                               description='LINK $out')
            project.ninja.rule('link-windows',
                               command='$cxx $in $libs /nologo /link /subsystem:windows /entry:WinMainCRTStartup $ldflags /out:$out',
                               description='LINK $out')
        else:
            project.ninja.rule('link',
                               command='$cxx $ldflags -o $out $in $libs',
                               description='LINK $out')

class CopyFileRule(Rule):
    def emit(self):
        if project.platform.is_windows():
            # We use powershell instead of CMD since it manages both Windows and POSIX paths well
            project.ninja.rule(
                name="copy",
                command='powershell -command "& {cp $in $out}"',
                description="CP $in $out")
        else:
            project.ninja.rule(
                name="copy",
                command="cp $in $out",
                description="CP $in $out")

class Target(ABC):
    @abstractmethod
    def get_name(self):
        pass

    @abstractmethod
    def get_outputs(self):
        pass

    @abstractmethod
    def emit(self):
        pass

class TargetWithDeps(Target):
    def __init__(self, deps: List[str]=None):
        self.deps = deps or []
        self.dep_includepaths = []
        self.dep_defines = []
        self.dep_libs = []
        self.dep_implicits = []

        for dep in self.deps:
            target = project.targets.get(dep)
            if target:
                if isinstance(target, TargetWithDeps):
                    self.dep_includepaths += target.dep_includepaths
                    self.dep_defines += target.dep_defines
                    self.dep_libs += target.dep_libs
                if hasattr(target, "includepaths"):
                    self.dep_includepaths += target.includepaths
                if hasattr(target, "defines"):
                    self.dep_defines += target.dep_defines
                if hasattr(target, "libs"):
                    self.dep_libs += target.libs
            self.dep_implicits.append(dep)

class ExternalLibTarget(TargetWithDeps):
    def __init__(self, name: str,
                 libs: List[str], includepaths: List[str],
                 defines: List[str]=None, deps: List[str]=None,
                 dir: str=None):

        super(ExternalLibTarget, self).__init__(deps)

        self.name = name

        if dir is None:
            dir = posixpath.dirname(str(sys.modules['__main__'].__file__))
        self.dir = dir

        self.libs = [Path(self.dir) / lib for lib in libs]
        self.includepaths = [Path(self.dir) / inc for inc in includepaths]
        self.defines = defines or []

    def get_name(self):
        return self.name

    def get_outputs(self):
        return []

    def emit(self):
        project.ninja.build(self.name, 'phony', None, self.dep_implicits)


class CppTarget(TargetWithDeps):
    def __init__(self, name: str, sources: List[str],
                 includepaths: List[str], defines: List[str]=None,
                 deps: List[str]=None, dir: str=None):

        super(CppTarget, self).__init__(deps)

        self.name = name

        if dir is None:
            dir = posixpath.dirname(str(sys.modules['__main__'].__file__))
        self.dir = dir

        self.sources = [Path(self.dir) / src for src in sources]
        self.includepaths = [Path(self.dir) / inc for inc in includepaths]
        self.defines = defines or []
        self.objs = []

    def get_name(self):
        return self.name

    def src(self, filename):
        return str(Path(self.dir) / filename)

    def cc(self, filename: Path, **kwargs):
        objfile = str(filename.with_suffix(get_platform_objfile_ext()))
        return project.ninja.build(
            project.built_obj(objfile), "cc", str(filename), **kwargs)

    def cxx(self, filename: Path, **kwargs):
        objfile = str(filename.with_suffix(get_platform_objfile_ext()))
        return project.ninja.build(
            project.built_obj(objfile), "cxx", str(filename), **kwargs)

    def get_cflags(self):
        return project.cflags + \
               [f"-I{includepath}" for includepath in self.dep_includepaths] + \
               [f"-I{includepath}" for includepath in self.includepaths] + \
               [f"-D{define}" for define in self.dep_defines] + \
               [f"-D{define}" for define in self.defines]

    def get_cxxflags(self):
        return project.cxxflags + \
               [f"-I{includepath}" for includepath in self.dep_includepaths] + \
               [f"-I{includepath}" for includepath in self.includepaths] + \
               [f"-D{define}" for define in self.dep_defines] + \
               [f"-D{define}" for define in self.defines]

    def get_inputs(self):
        cflags = self.get_cflags()
        cxxflags = self.get_cxxflags()
        inputs = []
        for src in self.sources:
            if src.suffix == '.c':
                inputs += self.cc(src, variables=dict(cflags=cflags))
            elif src.suffix == '.cpp' or src.suffix == '.cc':
                inputs += self.cxx(src, variables=dict(cxxflags=cxxflags))
        return inputs

class StaticLibTarget(CppTarget):
    def __init__(self, name: str, sources: List[str],
                 includepaths: List[str]=None, defines: List[str]=None,
                 deps: List[str]=None,
                 dir: str=None):
        super(StaticLibTarget, self).__init__(name, sources, includepaths or [], defines, deps, dir)
        if project.platform.is_windows():
            libname = f"{self.name}.lib"
        else:
            libname = f"lib{self.name}.a"
        self.libs = [project.built_lib(libname)]

    def get_outputs(self):
        return [self.libfile]

    def emit(self):
        variables = dict(libs=" ".join(str(lib) for lib in self.dep_libs))
        project.ninja.build(outputs=self.libs, rule='ar', inputs=self.get_inputs(), implicit=self.dep_implicits, variables=variables)
        project.ninja.build(outputs=self.name, rule='phony', inputs=self.libs)
        project.ninja.newline()

class HeaderOnlyTarget(CppTarget):
    def __init__(self, name: str, includepaths: List[str],
                 defines: List[str]=None,
                 deps: List[str]=None,
                 dir: str=None):
        super(HeaderOnlyTarget, self).__init__(name, [], includepaths, defines, deps, dir)

    def get_outputs(self):
        return []

    def emit(self):
        project.ninja.build(outputs=self.name, rule='phony', inputs=[])

class ExecutableTarget(CppTarget):
    def __init__(self, name: str, sources: List[str],
                 includepaths: List[str], defines: List[str]=None,
                 deps: List[str]=None, dir: str=None, windows_subsystem="console"):
        super(ExecutableTarget, self).__init__(name, sources, includepaths, defines, deps, dir)

        if project.platform.is_windows():
            self.exe = project.built_bin(self.name + '.exe')
        else:
            self.exe = project.built_bin(self.name)

        self.windows_subsystem = windows_subsystem

    def get_outputs(self):
        return [self.exe]

    def emit(self):
        rule = None
        if project.platform.is_windows():
            if self.windows_subsystem == 'console':
                rule = 'link-console'
            elif self.windows_subsystem == 'windows':
                rule = 'link-windows'
        else:
            rule = 'link'

        libs = project.system_libs
        libs += [str(lib) for lib in self.dep_libs]
        variables = dict(libs=" ".join(libs))
        project.ninja.build(self.name, 'phony', self.exe)
        project.ninja.build(outputs=[self.exe], rule=rule, inputs=self.get_inputs(), implicit=self.dep_implicits, variables=variables)
        project.ninja.newline()

class Project:
    platform: Platform
    ninja: NinjaWriter
    objext: str
    rootdir: str
    targets: Dict[str, Target]
    rules: List[Rule]
    external_libs: List[ExternalLibTarget]
    cflags: List[str]
    cxxflags: List[str]
    ldflags: List[str]

    def built_obj(self, filename: str):
        return posixpath.join('$builddir/obj', filename)

    def built_lib(self, filename: str):
        return posixpath.join('$builddir/lib', filename)

    def built_bin(self, filename: str):
        return posixpath.join('$builddir/bin', filename)

    def __init__(self):
        parser = OptionParser()
        parser.add_option("--platform",
                          help=f"target platform ({'/'.join(Platform.known_platforms())})",
                          choices=Platform.known_platforms())
        parser.add_option("--host",
                          help=f"host platform ({'/'.join(Platform.known_platforms())})",
                          choices=Platform.known_platforms())
        parser.add_option("--mode",
                          help=f"compile mode",
                          choices=["debug", "releasedbg", "release"],
                          default="debug")
        (options, args) = parser.parse_args()
        if args:
            print('Error when parsing command-line arguments: ', args)
            sys.exit(1)

        self.platform = Platform(options.platform)
        if options.host:
            self.host = Platform(options.host)
        else:
            self.host = self.platform

        BUILD_FILENAME = 'build.ninja'
        self.ninja = NinjaWriter(open(BUILD_FILENAME, "w"), width=120)

        self.rootdir = posixpath.dirname(str(sys.modules['__main__'].__file__))

        self.targets = dict()
        self.rules = [CRule(), CxxRule(), ArchiveRule(), LinkRule(), CopyFileRule()]

        self.system_libs = list()
        self.external_libs = list()

        self.mode = options.mode

        self.cflags = []
        self.cxxflags = []
        self.ldflags = []

    def configure(self):
        n = self.ninja

        n.variable('ninja_required_version', '1.3')
        n.newline()

        n.comment('The arguments passed to configure.py, for rerunning it.')
        configure_args = sys.argv[1:]
        n.variable('configure_args', ' '.join(configure_args))
        env_keys = {'CC', 'CXX', 'AR', 'CFLAGS', 'CXXFLAGS', 'LDFLAGS'}
        configure_env = {k: os.environ[k] for k in os.environ if k in env_keys}
        if configure_env:
            config_str = ' '.join([f"{k}={pipes.quote(configure_env[k])}"
                                   for k in configure_env])
            n.variable('configure_env', config_str + '$ ')
        n.newline()

        self.CXX = configure_env.get('CXX', 'c++')
        self.CC = configure_env.get('CC', 'cc')
        self.objext = '.o'
        if self.platform.is_windows():
            self.CXX = 'clang-cl'
            self.CC = 'clang-cl'
            self.objext = '.obj'

        n.variable('builddir', f"build/{self.mode}")
        n.variable('cxx', self.CXX)
        n.variable('cc', self.CC)
        if self.platform.is_windows():
            n.variable('ar', 'link')
        else:
            n.variable('ar', configure_env.get('AR', 'ar'))

        if self.platform.is_windows():
            self.cflags = ['/showIncludes',
                           '/nologo',    # Don't print startup banner.
                           '/GR-',       # Disable RTTI.
                           '/Zi',        # Create pdb with debug info.
                           '/D_CRT_SECURE_NO_WARNINGS', # Disable annoying CRT function warnings
                           ]

            ldflags = ['/libpath:$builddir/lib']
            if self.mode == 'debug':
                self.cflags += ['/MTd']
                self.ldflags += ['/DEBUG']
            elif self.mode == 'releasedbg':
                self.cflags += ['/MT', '/O2', '/DNDEBUG']
                self.ldflags += ['/DEBUG', '/OPT:REF', '/OPT:ICF']
            elif self.mode == 'release':
                self.cflags += ['/MT', '/O2', '/DNDEBUG']
                self.ldflags += ['/OPT:REF', '/OPT:ICF']
        else:
            self.cflags = ['-g']
            if self.mode == 'debug':
                self.cflags += ['-D_GLIBCXX_DEBUG', '-D_GLIBCXX_DEBUG_PEDANTIC']
                self.cflags.remove('-fno-rtti')
            else:
                self.cflags += ['-O2', '-DNDEBUG']
            try:
                proc = subprocess.Popen(
                    [self.CXX, '-fdiagnostics-color', '-c', '-x', 'c++', '/dev/null',
                     '-o', '/dev/null'],
                    stdout=open(os.devnull, 'wb'), stderr=subprocess.STDOUT)
                if proc.wait() == 0:
                    self.cflags += ['-fdiagnostics-color']
            except:
                pass
            ldflags = ['-L$builddir']

        # for target in self.targets.values():
        #     if isinstance(target, CppTarget):
        #         cflags += [f"-I{path}" for path in target.get_includepaths()]
        #         cflags += [f"-D{define}" for define in target.get_defines()]


        if self.platform.is_windows():
            self.cxxflags = self.cflags + [
                '/std:c++17', # Use the C++17 standard.
                '-Xclang -Wno-invalid-offsetof',
            ]
        else:
            self.cxxflags = self.cflags + [
                '-std=c++17',
                '-fno-rtti',
                '-fno-exceptions'
            ]

        def shell_escape(str):
            """Escape str such that it's interpreted as a single argument by
            the shell."""

            # This isn't complete, but it's just enough to make NINJA_PYTHON work.
            if self.platform.is_windows():
                return str
            if '"' in str:
                return "'%s'" % str.replace("'", "\\'")
            return str

        if 'CFLAGS' in configure_env:
            self.cflags.append(configure_env['CFLAGS'])
        n.variable('cflags', ' '.join(shell_escape(flag) for flag in self.cflags))

        if 'CXXFLAGS' in configure_env:
            self.cxxflags.append(configure_env['CXXFLAGS'])
        n.variable('cxxflags', ' '.join(shell_escape(flag) for flag in self.cxxflags))

        if 'LDFLAGS' in configure_env:
            self.ldflags.append(configure_env['LDFLAGS'])
        n.variable('ldflags', ' '.join(shell_escape(flag) for flag in self.ldflags))

        n.newline()

        for rule in self.rules:
            rule.emit()
            project.ninja.newline()

        for target in self.targets.values():
            target.emit()
            project.ninja.newline()

        project.ninja.close()

        # Automatically generate compilation db
        with open("compile_commands.json", "w") as f:
            subprocess.call(["ninja", "-t", "compdb", "cxx"], stdout=f)

    def add_static_lib(self, *args, **kwargs):
        self.add_custom_target(StaticLibTarget(*args, **kwargs))

    def add_headeronly_lib(self, *args, **kwargs):
        self.add_custom_target(HeaderOnlyTarget(*args, **kwargs))

    def add_executable(self, *args, **kwargs):
        self.add_custom_target(ExecutableTarget(*args, **kwargs))

    def add_custom_target(self, target: "Target"):
        self.targets[target.get_name()] = target

    def add_custom_rule(self, rule: "Rule"):
        self.rules.append(rule)

    def add_custom_targets(self, targets: List["Target"]):
        for target in targets:
            self.add_custom_target(target)

    def add_custom_rules(self, rules: List["Rule"]):
        for rule in rules:
            self.add_custom_rule(rule)

    def add_system_libs(self, *libs):
        for lib in libs:
            self.system_libs.append(lib)

    def add_external_lib(self, *args, **kwargs):
        self.add_custom_target(ExternalLibTarget(*args, **kwargs))

project: Project = Project()
