#!/usr/bin/env python3

import argparse

import os
import sys

try:
    from lxml import etree
except ImportError:
    import xml.etree.cElementTree as etree


class NoneStr(str):

    """
    This is ugly but it serves two purposes:
    1) allows passing None -> ""
    2) allows older versions of python to implement str methods from newer python versions
    """

    def __new__(cls, content):
        if content is None:
            content = ""

        return str.__new__(cls, content)

    def __str__(self):
        return str.__str__(self)

    def removeprefix(self, prefix: str) -> str:
        try:
            return str.__str__(self).removeprefix(prefix)
        except AttributeError:
            pass

        if str.__str__(self).startswith(prefix):
            return str.__str__(self)[len(prefix):]

        return str.__str__(self)


class Generator(object):

    template_h = ""

    template_type = """
using {name} = {value};
"""

    template_enum = """
constexpr {type} {name} = {value};
"""

    template_proc = """
using {proc} = {prototype}(*)({args});
"""

    template_method = ""

    template_platform_include = ""

    template_platform_include_apple = ""

    def __init__(self, args) -> None:

        self.apple = False
        self.xml = None
        self.output_file = args.output

        self.input = self.GeneratorInput()
        self.output = self.GeneratorOutput()

    class GeneratorBase:
        def __init__(self) -> None:

            self.types = []
            self.enums = []
            self.commands = []

        def AddType(self, name: str):
            if name not in self.types:
                self.types.append(name)

        def AddEnum(self, name: str):
            if name not in self.enums:
                self.enums.append(name)

        def AddCommand(self, name: str):
            if name not in self.commands:
                self.commands.append(name)

        def ContainsType(self, type: str):
            return True if type in self.types else False

        def ContainsEnum(self, enum: str):
            return True if enum in self.enums else False

        def ContainsCommand(self, command: str):
            return True if command in self.commands else False

    class GeneratorInput(GeneratorBase):
        def __init__(self) -> None:
            super().__init__()

    class GeneratorOutput(GeneratorBase):
        def __init__(self) -> None:
            super().__init__()

            self.procs = []

        def AddProc(self, name: str):
            if name not in self.procs:
                self.procs.append(name)

        def GetTypeString(self):
            return "".join(self.types)

        def GetEnumString(self):
            return "".join(self.enums)

        def GetCommandString(self):
            return "".join(self.commands)

        def GetProcString(self):
            return "".join(self.procs)

    class BaseInfo(object):
        def __init__(self, element: etree.Element) -> None:

            self.element = None
            self.name = None

        def GetName(self):
            return self.name

    class TypeInfo(BaseInfo):
        def __init__(self, element: etree.Element) -> None:
            super().__init__(element)

            self.name = element.get("name")

            value = NoneStr(element.text).rstrip()

            if element.find("apientry") is not None:
                value += element.find("apientry").tail
                value += element.find("name").tail.strip(";")

            self.value = NoneStr(value).removeprefix("typedef ")

            template_args = {
                "name": self.name,
                "value": self.value,
            }

            self.output = Generator.template_type.format(**template_args)

        def GetValue(self):
            return self.value

        def GetOutput(self):
            return self.output

    class EnumInfo(BaseInfo):
        def __init__(self, element: etree.Element) -> None:
            super().__init__(element)

            self.name = element.get("name")

            value = element.get("value")

            type = element.get("type")

            if type is not None:
                if type == "u":
                    type = "uint32_t"
                if type == "ull":
                    type = "uint64_t"
            else:
                type = "int"

            if value.startswith('"') and value.endswith('"'):
                type = "const char*"

            template_args = {
                "type": type,
                "name": self.name,
                "value": value,
            }

            self.output = Generator.template_enum.format(**template_args)

        def GetOutput(self):
            return self.output

    class CommandInfo(BaseInfo):
        def __init__(self, element: etree.Element) -> None:
            super().__init__(element)

            self.name = element.find("proto/name").text

            proto = element.find("proto")
            proto_str = NoneStr(proto.text)

            if proto.find("ptype") != None:
                proto_str += NoneStr(proto.find("ptype").text)
                proto_str += NoneStr(proto.find("ptype").tail)

            params = []
            for param in element.iter("param"):

                arg_str = NoneStr(param.text)

                if param.find("ptype") != None:
                    arg_str += NoneStr(param.find("ptype").text)
                    arg_str += NoneStr(param.find("ptype").tail)

                arg_str += param.find("name").text

                params.append(arg_str)

            args = ", ".join(params)

            arg_names = ", ".join(
                [arg.split(" ")[-1].strip("*") for arg in args.split(", ")]
            )

            # remove dangling asterisk (see ref: const GLchar *const*string)
            arg_names = ", ".join([arg.split("*")[-1] for arg in arg_names.split(", ")])

            # remove arg names if only arg is void type
            arg_names = "" if "void" in arg_names else arg_names

            # add return if not void (but not void*)
            return_str = (
                "" if "void" in proto_str and not "*" in proto_str else "return "
            )

            proc = "PFN{}PROC".format(self.name.upper())

            template_args = {
                "proc": proc,
                "prototype": proto_str,
                "args": args,
            }

            self.proc = Generator.template_proc.format(**template_args)

            # here we want to move the gl functions into our gl:: namespace
            # let's remove the gl prefix so we get something like gl::TexImage2D
            # instead of gl:glTexImage2D
            short_name = self.name
            if not self.name.startswith("glX"):
                short_name = NoneStr(self.name).removeprefix("gl")

            template_args = {
                "proto": proto_str,
                "name": self.name,
                "short_name": short_name,
                "args": args,
                "proc": proc,
                "return_str": return_str,
                "arg_names": arg_names,
            }

            self.command = Generator.template_method.format(**template_args)

        def GetCommand(self):
            return self.command

        def GetProc(self):
            return self.proc

    def _GetFeatures(self, registry: etree.Element):

        for feature in registry.findall("feature"):
            if feature.get("api") not in self.apis:
                continue

            if feature.get("name") not in self.apis[feature.get("api")]:
                continue

            for require in feature.iter("require"):

                for type in require.iter("type"):
                    name = type.get("name")
                    self.input.AddType(name)

                for enum in require.iter("enum"):
                    name = enum.get("name")
                    self.input.AddEnum(name)

                for command in require.iter("command"):
                    name = command.get("name")
                    self.input.AddCommand(name)

    def _GetExtensions(self, registry: etree.Element):

        for extension in registry.findall("extensions/extension"):

            supported = extension.get("supported").split("|")

            if not any(api in supported for api in self.apis):
                continue

            if extension.get("name") not in self.extensions:
                continue

            for require in extension.iter("require"):

                for type in require.iter("type"):
                    name = type.get("name")
                    self.input.AddType(name)

                for enum in require.iter("enum"):
                    name = enum.get("name")
                    self.input.AddEnum(name)

                for command in require.iter("command"):
                    name = command.get("name")
                    self.input.AddCommand(name)

    def _GenerateCommands(self, registry: etree.Element):

        for method in registry.findall("commands/command"):

            info = self.CommandInfo(method)

            if not self.input.ContainsCommand(info.GetName()):
                continue

            self.output.AddCommand(info.GetCommand())
            self.output.AddProc(info.GetProc())

    def _GenerateEnums(self, registry: etree.Element):

        for element in registry.findall("enums/enum"):

            if element.get("name") == None:
                element.attrib["name"] = element.find("name").text

            info = self.EnumInfo(element)

            if not self.input.ContainsEnum(info.GetName()):
                continue

            self.output.AddEnum(info.GetOutput())

    def _GenerateTypes(self, registry: etree.Element):

        for element in registry.findall("types/type"):

            if element.get("name") == None:
                element.attrib["name"] = element.find("name").text

            info = self.TypeInfo(element)

            if info.GetValue() == "":
                continue

            if info.GetValue().startswith("union"):
                continue

            if info.GetName().startswith("struct"):
                continue

            if info.GetValue().startswith("#include"):
                continue

            if info.GetValue().startswith("#ifdef"):
                continue

            if info.GetValue().startswith("#ifndef"):
                continue

            self.output.AddType(info.GetOutput())

    def Process(self):

        tree = etree.parse(self.xml)
        registry = tree.getroot()

        # Find required typs/enums/commands from features and extensions
        self._GetFeatures(registry)
        self._GetExtensions(registry)

        # Generate types/enums/commands from requirements
        self._GenerateTypes(registry)
        self._GenerateEnums(registry)
        self._GenerateCommands(registry)

        template_args = {
            "platform_include": Generator.template_platform_include,
            "types": self.output.GetTypeString(),
            "enums": self.output.GetEnumString(),
            "procs": self.output.GetProcString(),
            "methods": self.output.GetCommandString(),
        }

        # Generate header from generated types/enums/commands
        self.output_h = Generator.template_h.format(**template_args)

    def Write(self):

        with open(f"{self.output_file}", "w") as f:
            f.write(self.output_h)


class GeneratorGL(Generator):
    def __init__(self, args) -> None:
        super().__init__(args)

        self.xml = args.gl_h_xml
        self.apple = args.gl_apple

        self.apis = {
            "gl": [
                "GL_VERSION_1_0",
                "GL_VERSION_1_1",
                "GL_VERSION_1_2",
                "GL_VERSION_1_3",
                "GL_VERSION_1_4",
                "GL_VERSION_1_5",
                "GL_VERSION_2_0",
                "GL_VERSION_2_1",
                "GL_VERSION_3_0",
                "GL_VERSION_3_1",
            ],
            "gles2": [
                "GL_ES_VERSION_2_0",
            ],
        }

        self.extensions = [
            "GL_OES_EGL_image",
            "GL_OES_EGL_image_external",
            "GL_EXT_texture_format_BGRA8888",
            "GL_KHR_debug",
            "GL_EXT_color_buffer_half_float",
            "GL_EXT_unpack_subimage",
            "GL_EXT_texture_compression_s3tc",
            "GL_ARB_sync",
            "GL_NV_vdpau_interop",
        ]

        if self.apple:
            Generator.template_platform_include = (
                Generator.template_platform_include_apple
            )

            apple_extensions = [
                "GL_APPLE_sync",
                "GL_APPLE_fence",
                "GL_APPLE_texture_format_BGRA8888",
            ]

            self.extensions.extend(apple_extensions)

        Generator.template_h = """
#pragma once

#include "windowing/WinSystem.h"
#include "ServiceBroker.h"

#include <stdexcept>

{platform_include}

{types}

{enums}

{procs}

namespace gl
{{
{methods}
}} // namespace gl
"""

        Generator.template_method = """
inline {proto} {short_name}({args})
{{
  static auto winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
    throw std::runtime_error("no window system available!");

  static {proc} p = reinterpret_cast<{proc}>(winSystem->GetProcAddressGL("{name}"));

  if (!p)
    throw std::runtime_error("attempted to use unresolved gl method: {name}");

  {return_str}p({arg_names});
}}
"""

    Generator.template_platform_include = """
extern "C"
{
#include <KHR/khrplatform.h>
}
"""

    Generator.template_platform_include_apple = """
#include <cstdint>

using khronos_int32_t = int32_t;
using khronos_uint32_t = uint32_t;
using khronos_int64_t = int64_t;
using khronos_uint64_t = uint64_t;

using khronos_int8_t = signed char;
using khronos_uint8_t = unsigned char;
using khronos_int16_t = signed short int;
using khronos_uint16_t = unsigned short int;

using khronos_intptr_t = signed long int;
using khronos_uintptr_t = unsigned long int;
using khronos_ssize_t = signed long int;
using khronos_usize_t = unsigned long int;

using khronos_float_t = float;
"""


class GeneratorGLX(Generator):
    def __init__(self, args) -> None:
        super().__init__(args)

        self.xml = args.glx_h_xml

        self.apis = {
            "glx": [
                "GLX_VERSION_1_0",
                "GLX_VERSION_1_1",
                "GLX_VERSION_1_2",
                "GLX_VERSION_1_3",
                "GLX_VERSION_1_4",
            ],
        }

        self.extensions = [
            "GLX_EXT_swap_control",
        ]

        Generator.template_h = """
#pragma once

extern "C"
{{
#include <X11/Xlib.h>
#include <X11/Xutil.h>
}}

#include "RenderingGL.hpp"

{types}

{enums}

{procs}

extern "C"
{{
{methods}
}}
"""

        Generator.template_method = """
extern {proto} {name}({args});
"""


if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        description="Create OpenGL c++ bindings.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--gl", dest="gl_h_xml", required=False, type=str, help="path to gl.xml"
    )
    group.add_argument(
        "--glx", dest="glx_h_xml", required=False, type=str, help="path to glx.xml"
    )

    parser.add_argument("--gl-apple", dest="gl_apple", action="store_true")

    parser.add_argument("output", type=str, help="output file name")

    args = parser.parse_args()

    gen = None

    if args.gl_h_xml is not None:
        if not os.path.exists(args.gl_h_xml):
            sys.exit(f"gl.xml doesn't exist: {args.gl_h_xml}")
        elif os.path.basename(args.gl_h_xml) != "gl.xml":
            sys.exit(f"file doesn't look like gl.xml: {args.gl_h_xml}")
        else:
            gen = GeneratorGL(args)

    if args.glx_h_xml is not None:
        if not os.path.exists(args.glx_h_xml):
            sys.exit(f"glx.xml doesn't exist: {args.glx_h_xml}")
        elif os.path.basename(args.glx_h_xml) != "glx.xml":
            sys.exit(f"file doesn't look like glx.xml: {args.glx_h_xml}")
        else:
            gen = GeneratorGLX(args)

    gen.Process()

    gen.Write()
