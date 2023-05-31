#!/usr/bin/env python3

import argparse
import os

import yaml
from mako.template import Template


def main():
    parser = argparse.ArgumentParser(
        description="OpenPOWER error map code generator"
    )

    parser.add_argument(
        "-i",
        "--errors_map_yaml",
        dest="errors_map_yaml",
        default="errors_watch.yaml",
        help="input errors watch yaml file to parse",
    )

    parser.add_argument(
        "-c",
        "--cpp_file",
        dest="cpp_file",
        default="errors_map.cpp",
        help="output cpp file",
    )
    args = parser.parse_args()

    with open(os.path.join(script_dir, args.errors_map_yaml), "r") as fd:
        yamlDict = yaml.safe_load(fd)

        # Render the mako template for cpp file
        template = os.path.join(script_dir, "errors_map.mako.cpp")
        t = Template(filename=template)
        with open(args.cpp_file, "w") as fd:
            fd.write(t.render(errDict=yamlDict))

if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.realpath(__file__))
    main()
