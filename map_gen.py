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
        "--input_dump_type_yaml",
        dest="input_dump_type_yaml",
        default="example_dump_types.yaml",
        help="input dump type yaml file to parse",
    )

    parser.add_argument(
        "-j",
        "--input_error_type_yaml",
        dest="input_error_type_yaml",
        default="example_errors_watch.yaml",
        help="input error type yaml file to parse",
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

    args = parser.parse_args()

    with open(os.path.join(script_dir, args.input_dump_type_yaml), "r") as fd:
        yaml_dict1 = yaml.safe_load(fd)

    with open(os.path.join(script_dir, args.input_error_type_yaml), "r") as fd:
        yaml_dict2 = yaml.safe_load(fd)

    template = os.path.join(script_dir, args.template)
    t = Template(filename=template)
    with open(args.output_file, "w") as fd:
        fd.write(
            t.render(DUMP_TYPE_TABLE=yaml_dict1, ERROR_TYPE_DICT=yaml_dict2)
        )


if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.realpath(__file__))
    main()
