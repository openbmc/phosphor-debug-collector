#!/usr/bin/env python3

import argparse
import os

import yaml
from mako.template import Template

def main():
    parser = argparse.ArgumentParser(
        description="OpenPOWER map code generator"
    )

    parser.add_argument(
        "-i",
        "--input_yaml",
        dest="input_yaml",
        default="example.yaml",
        help="input yaml file to parse",
    )

    parser.add_argument(
        "-t",
        "--template",
        dest="template",
        default="template.mako.cpp",
        help="mako template file to use",
    )

    parser.add_argument(
        "-o",
        "--output_file",
        dest="output_file",
        default="output.cpp",
        help="output cpp file",
    )

    parser.add_argument(
        "-v",
        "--var_name",
        dest="var_name",
        default="mapping",
        help="variable name to use in the template",
    )

    args = parser.parse_args()

    with open(os.path.join(script_dir, args.input_yaml), "r") as fd:
        yaml_dict = yaml.safe_load(fd)

    template = os.path.join(script_dir, args.template)
    t = Template(filename=template)
    with open(args.output_file, "w") as fd:
        if args.var_name == "errDict":
            fd.write(t.render(errDict=yaml_dict))
        else:
            fd.write(t.render(DUMP_TYPE_TABLE=yaml_dict))

if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.realpath(__file__))
    main()
