#!/usr/bin/env python3
import json
import sys
from pathlib import Path

TYPE_MAP = {
    "uint8": "uint8_t",
    "uint16": "uint16_t",
    "uint32": "uint32_t",
    "uint64": "uint64_t",
    "int8": "int8_t",
    "int16": "int16_t",
    "int32": "int32_t",
    "int64": "int64_t",
    "float": "float",
    "double": "double",
}


def load_protocol(json_path: str) -> dict:
    """Open the file and validate if the json is well written"""
    try:
        file = open(json_path)
        content = file.read()
        return json.loads(content)
    except ValueError as e:
        print("invalid file: %s" % e)
        sys.exit(1)


def is_valid_msg(msg: dict, list_id: list) -> bool:
    """Check if a packet definition is valid"""
    if "id" not in msg or not isinstance(msg["id"], int):
        return False

    if msg["id"] in list_id:
        return False

    if "fields" not in msg or not isinstance(msg["fields"], list):
        return False

    for field in msg["fields"]:
        if "name" not in field or not isinstance(field["name"], str):
            return False

        if "type" not in field or not isinstance(field["type"], str):
            return False

        if field["type"] not in TYPE_MAP and field["type"] != "string":
            return False

        if field["type"] == "string":
            if (
                "max_length" not in field
                or not isinstance(field["max_length"], int)
                or field["max_length"] <= 0
            ):
                return False
    return True


def validate_protocol(protocol: dict):
    """Check if the whole protocol is valid"""
    messages = protocol["messages"]
    if messages is None:
        print("Error: File doesn't contain 'messages' field")
        sys.exit(1)
    list_id = []
    for msg_name, msg_data in messages.items():
        if not is_valid_msg(msg_data, list_id):
            print(f"Error: {msg_name} invalid")
            sys.exit(1)
        list_id.append(msg_data["id"])


def get_endianness(protocol: dict) -> str:
    """Extract and validate endianness from protocol"""
    if "endianness" not in protocol:
        print("Warning: 'endianness' not specified, defaulting to 'big'")
        return "big"

    endianness = protocol["endianness"].lower()
    if endianness not in ["big", "little"]:
        print(f"Error: Invalid endianness '{endianness}'. Must be 'big' or 'little'")
        sys.exit(1)

    return endianness


def generate_struct_declaration(msg_name: str, msg_data: dict) -> str:
    """Generate a struct from a packet definition"""
    output = ""

    output += f"struct {msg_name} {{\n"

    output += f"    static constexpr uint32_t ID = {msg_data['id']};\n\n"

    for field in msg_data["fields"]:
        field_name = field["name"]
        field_type = field["type"]

        if field_type == "string":
            output += f"    char {field_name}[{field['max_length']}];\n"
        else:
            output += f"    {TYPE_MAP[field_type]} {field_name};\n"

    output += "\n"
    output += "    std::vector<uint8_t> serialize() const;\n"
    output += f"    static {msg_name} deserialize(const std::vector<uint8_t>& data);\n"
    output += "};\n\n"

    return output


def generate_header(protocol: dict) -> str:
    """Generate a file with all structs definition"""
    output = ""
    output += "#pragma once\n"
    output += "#include <cstdint>\n"
    output += "#include <vector>\n"
    output += "#include <cstring>\n\n"
    output += "namespace net {\n\n"

    for msg_name, msg_data in protocol["messages"].items():
        output += generate_struct_declaration(msg_name, msg_data)

    output += "}  // namespace net\n"

    return output


def write_uint_bytes(field_name: str, num_bytes: int, endianness: str) -> str:
    """Generate code to write unsigned integer in specified endianness"""
    output = ""

    if endianness == "big":
        for i in range(num_bytes - 1, -1, -1):
            output += f"    buffer.push_back(({field_name} >> {i * 8}) & 0xFF);\n"
    else:
        for i in range(num_bytes):
            output += f"    buffer.push_back(({field_name} >> {i * 8}) & 0xFF);\n"

    return output


def read_uint_bytes(
    field_name: str, num_bytes: int, type_name: str, endianness: str
) -> str:
    """Generate code to read unsigned integer in specified endianness"""
    output = ""
    if endianness == "big":
        output += f"    msg.{field_name} = "
        for i in range(num_bytes):
            if i > 0:
                output += " |\n        "
            shift = (num_bytes - 1 - i) * 8
            output += f"(static_cast<{type_name}>(data[offset + {i}]) << {shift})"
        output += ";\n"
    else:
        output += f"    msg.{field_name} = "
        for i in range(num_bytes):
            if i > 0:
                output += " |\n        "
            shift = i * 8
            output += f"(static_cast<{type_name}>(data[offset + {i}]) << {shift})"
        output += ";\n"

    output += f"    offset += {num_bytes};\n"
    return output


def generate_serialize_impl(msg_name: str, fields: list, endianness: str) -> str:
    """Generate the serialize method of a struct"""
    output = ""
    output += f"std::vector<uint8_t> {msg_name}::serialize() const {{\n"
    output += "    std::vector<uint8_t> buffer;\n\n"

    output += "    // Write message ID\n"
    output += write_uint_bytes("ID", 4, endianness)
    output += "\n"

    for field in fields:
        field_name = field["name"]
        field_type = field["type"]

        output += f"    // Write {field_name}\n"

        if field_type == "string":
            max_len = field["max_length"]
            output += f"    for (uint32_t i = 0; i < {max_len}; ++i) {{\n"
            output += f"        buffer.push_back({field_name}[i]);\n"
            output += "    }\n"

        elif field_type in ["uint8", "int8"]:
            output += f"    buffer.push_back({field_name});\n"

        elif field_type in ["uint16", "int16"]:
            output += write_uint_bytes(field_name, 2, endianness)

        elif field_type in ["uint32", "int32"]:
            output += write_uint_bytes(field_name, 4, endianness)

        elif field_type in ["uint64", "int64"]:
            output += write_uint_bytes(field_name, 8, endianness)

        elif field_type == "float":
            output += "    {\n"
            output += "        uint32_t temp;\n"
            output += f"        std::memcpy(&temp, &{field_name}, sizeof(float));\n"
            output += write_uint_bytes("temp", 4, endianness).replace(
                "    ", "        "
            )
            output += "    }\n"

        elif field_type == "double":
            output += "    {\n"
            output += "        uint64_t temp;\n"
            output += f"        std::memcpy(&temp, &{field_name}, sizeof(double));\n"
            output += write_uint_bytes("temp", 8, endianness).replace(
                "    ", "        "
            )
            output += "    }\n"

        output += "\n"

    output += "    return buffer;\n"
    output += "}\n\n"
    return output


def generate_deserialize_impl(msg_name: str, fields: list, endianness: str) -> str:
    """Generate the deserialize method of a struct"""
    output = ""

    output += (
        f"{msg_name} {msg_name}::deserialize(const std::vector<uint8_t>& data) {{\n"
    )
    output += f"    {msg_name} msg;\n"
    output += "    size_t offset = 0;\n\n"

    output += "    // Skip message ID\n"
    output += "    offset += 4;\n\n"

    for field in fields:
        field_name = field["name"]
        field_type = field["type"]

        output += f"    // Read {field_name}\n"

        if field_type == "string":
            max_len = field["max_length"]
            output += (
                f"    std::memcpy(msg.{field_name}, data.data() + offset, {max_len});\n"
            )
            output += f"    offset += {max_len};\n"

        elif field_type in ["uint8", "int8"]:
            output += f"    msg.{field_name} = data[offset];\n"
            output += "    offset += 1;\n"

        elif field_type == "uint16":
            output += read_uint_bytes(field_name, 2, "uint16_t", endianness)

        elif field_type == "int16":
            output += read_uint_bytes(field_name, 2, "int16_t", endianness)

        elif field_type == "uint32":
            output += read_uint_bytes(field_name, 4, "uint32_t", endianness)

        elif field_type == "int32":
            output += read_uint_bytes(field_name, 4, "int32_t", endianness)

        elif field_type == "uint64":
            output += read_uint_bytes(field_name, 8, "uint64_t", endianness)

        elif field_type == "int64":
            output += read_uint_bytes(field_name, 8, "int64_t", endianness)

        elif field_type == "float":
            output += "    {\n"
            output += "        uint32_t temp;\n"
            temp_read = read_uint_bytes("temp", 4, "uint32_t", endianness).replace(
                "msg.", ""
            )
            output += "    " + temp_read.replace("\n", "\n    ")
            output += f"        std::memcpy(&msg.{field_name}, &temp, sizeof(float));\n"
            output += "    }\n"

        elif field_type == "double":
            output += "    {\n"
            output += "        uint64_t temp;\n"
            temp_read = read_uint_bytes("temp", 8, "uint64_t", endianness).replace(
                "msg.", ""
            )
            output += "    " + temp_read.replace("\n", "\n    ")
            output += (
                f"        std::memcpy(&msg.{field_name}, &temp, sizeof(double));\n"
            )
            output += "    }\n"

        output += "\n"

    output += "    return msg;\n"
    output += "}\n\n"

    return output


def generate_source(protocol: dict, endianness: str) -> str:
    """Generate the complete .cpp file containing all definitions of"""
    """ serialize and deserialize methods of protocol struct"""
    output = ""
    output += '#include "Network/generated_messages.hpp"\n\n'
    output += "namespace net {\n\n"

    for msg_name, msg_data in protocol["messages"].items():
        output += generate_serialize_impl(msg_name, msg_data["fields"], endianness)
        output += generate_deserialize_impl(msg_name, msg_data["fields"], endianness)

    output += "}  // namespace net\n"
    return output


def write_file(filepath: str, content: str):
    Path(filepath).parent.mkdir(parents=True, exist_ok=True)
    with open(filepath, "w") as f:
        f.write(content)


def main():
    if len(sys.argv) != 2:
        print("Usage: ./generate_protocol.py path_to.json")
        sys.exit(1)

    protocol = load_protocol(sys.argv[1])
    validate_protocol(protocol)
    endianness = get_endianness(protocol)

    header = generate_header(protocol)
    write_file("include/Network/generated_messages.hpp", header)
    source = generate_source(protocol, endianness)
    write_file("src/generated_messages.cpp", source)


if __name__ == "__main__":
    main()
