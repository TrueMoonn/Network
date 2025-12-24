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


def generate_serialize_impl(msg_name: str, fields: list) -> str:
    """Generate the serialize method of a struct"""
    output = ""
    output += f"std::vector<uint8_t> {msg_name}::serialize() const {{\n"
    output += "    std::vector<uint8_t> buffer;\n\n"

    output += "    // Write message ID\n"
    output += "    buffer.push_back((ID >> 24) & 0xFF);\n"
    output += "    buffer.push_back((ID >> 16) & 0xFF);\n"
    output += "    buffer.push_back((ID >> 8) & 0xFF);\n"
    output += "    buffer.push_back(ID & 0xFF);\n\n"

    for field in fields:
        field_name = field["name"]
        field_type = field["type"]

        output += f"    // Write {field_name}\n"

        if field_type == "string":
            max_len = field["max_length"]
            output += f"    for (uint32_t i = 0; i < {max_len}; ++i) {{\n"
            output += f"        buffer.push_back({field_name}[i]);\n"
            output += "    }\n"

        elif field_type == "uint8":
            output += f"    buffer.push_back({field_name});\n"

        elif field_type == "uint16":
            output += f"    buffer.push_back(({field_name} >> 8) & 0xFF);\n"
            output += f"    buffer.push_back({field_name} & 0xFF);\n"

        elif field_type == "uint32":
            output += f"    buffer.push_back(({field_name} >> 24) & 0xFF);\n"
            output += f"    buffer.push_back(({field_name} >> 16) & 0xFF);\n"
            output += f"    buffer.push_back(({field_name} >> 8) & 0xFF);\n"
            output += f"    buffer.push_back({field_name} & 0xFF);\n"
        elif field_type == "uint64":
            output += f"    buffer.push_back(({field_name} >> 56) & 0xFF);\n"
            output += f"    buffer.push_back(({field_name} >> 48) & 0xFF);\n"
            output += f"    buffer.push_back(({field_name} >> 40) & 0xFF);\n"
            output += f"    buffer.push_back(({field_name} >> 32) & 0xFF);\n"
            output += f"    buffer.push_back(({field_name} >> 24) & 0xFF);\n"
            output += f"    buffer.push_back(({field_name} >> 16) & 0xFF);\n"
            output += f"    buffer.push_back(({field_name} >> 8) & 0xFF);\n"
            output += f"    buffer.push_back({field_name} & 0xFF);\n"
        elif field_type == "int8":
            output += f"    buffer.push_back({field_name});\n"

        elif field_type == "int16":
            output += f"    buffer.push_back(({field_name} >> 8) & 0xFF);\n"
            output += f"    buffer.push_back({field_name} & 0xFF);\n"

        elif field_type == "int32":
            output += f"    buffer.push_back(({field_name} >> 24) & 0xFF);\n"
            output += f"    buffer.push_back(({field_name} >> 16) & 0xFF);\n"
            output += f"    buffer.push_back(({field_name} >> 8) & 0xFF);\n"
            output += f"    buffer.push_back({field_name} & 0xFF);\n"
        elif field_type == "int64":
            output += f"    buffer.push_back(({field_name} >> 56) & 0xFF);\n"
            output += f"    buffer.push_back(({field_name} >> 48) & 0xFF);\n"
            output += f"    buffer.push_back(({field_name} >> 40) & 0xFF);\n"
            output += f"    buffer.push_back(({field_name} >> 32) & 0xFF);\n"
            output += f"    buffer.push_back(({field_name} >> 24) & 0xFF);\n"
            output += f"    buffer.push_back(({field_name} >> 16) & 0xFF);\n"
            output += f"    buffer.push_back(({field_name} >> 8) & 0xFF);\n"
            output += f"    buffer.push_back({field_name} & 0xFF);\n"
        elif field_type == "float":
            output += "    {{\n"
            output += "        uint32_t temp;\n"
            output += f"        std::memcpy(&temp, &{field_name}, sizeof(float));\n"
            output += "        buffer.push_back((temp >> 24) & 0xFF);\n"
            output += "        buffer.push_back((temp >> 16) & 0xFF);\n"
            output += "        buffer.push_back((temp >> 8) & 0xFF);\n"
            output += "        buffer.push_back(temp & 0xFF);\n"
            output += "    }}\n"
        elif field_type == "double":
            output += "    {\n"
            output += "        uint64_t temp;\n"
            output += f"        std::memcpy(&temp, &{field_name}, sizeof(double));\n"
            output += "        buffer.push_back((temp >> 56) & 0xFF);\n"
            output += "        buffer.push_back((temp >> 48) & 0xFF);\n"
            output += "        buffer.push_back((temp >> 40) & 0xFF);\n"
            output += "        buffer.push_back((temp >> 32) & 0xFF);\n"
            output += "        buffer.push_back((temp >> 24) & 0xFF);\n"
            output += "        buffer.push_back((temp >> 16) & 0xFF);\n"
            output += "        buffer.push_back((temp >> 8) & 0xFF);\n"
            output += "        buffer.push_back(temp & 0xFF);\n"
            output += "    }\n"

        output += "\n"

    output += "    return buffer;\n"
    output += "}\n\n"
    return output


def generate_source(protocol: dict) -> str:
    """Generate the complete .cpp file containing all definitions of"""
    """ serialize and deserialize methods of protocol struct"""
    output = ""
    output += '#include "Network/generated_messages.hpp"\n\n'
    output += "namespace net {\n\n"

    for msg_name, msg_data in protocol["messages"].items():
        output += generate_serialize_impl(msg_name, msg_data["fields"])
        output += generate_deserialize_impl(msg_name, msg_data["fields"])
    output += "}  // namespace net\n"

    return output


def generate_deserialize_impl(msg_name: str, fields: list) -> str:
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
        elif field_type == "uint8":
            output += f"    msg.{field_name} = data[offset];\n"
            output += "    offset += 1;\n"
        elif field_type == "uint16":
            output += f"    msg.{field_name} = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];\n"
            output += "    offset += 2;\n"
        elif field_type == "uint32":
            output += f"    msg.{field_name} = (static_cast<uint32_t>(data[offset]) << 24) |\n"
            output += "        (static_cast<uint32_t>(data[offset + 1]) << 16) |\n"
            output += "        (static_cast<uint32_t>(data[offset + 2]) << 8) |\n"
            output += "        static_cast<uint32_t>(data[offset + 3]);\n"
            output += "    offset += 4;\n"
        elif field_type == "uint64":
            output += f"    msg.{field_name} = (static_cast<uint64_t>(data[offset]) << 56) |\n"
            output += "        (static_cast<uint64_t>(data[offset + 1]) << 48) |\n"
            output += "        (static_cast<uint64_t>(data[offset + 2]) << 40) |\n"  # ← | pas ;
            output += "        (static_cast<uint64_t>(data[offset + 3]) << 32) |\n"
            output += "        (static_cast<uint64_t>(data[offset + 4]) << 24) |\n"
            output += "        (static_cast<uint64_t>(data[offset + 5]) << 16) |\n"
            output += "        (static_cast<uint64_t>(data[offset + 6]) << 8) |\n"
            output += "        static_cast<uint64_t>(data[offset + 7]);\n"
            output += "    offset += 8;\n"
        elif field_type == "int8":
            output += f"    msg.{field_name} = data[offset];\n"
            output += "    offset += 1;\n"
        elif field_type == "int16":
            output += f"    msg.{field_name} = (static_cast<int16_t>(data[offset]) << 8) | data[offset + 1];\n"
            output += "    offset += 2;\n"
        elif field_type == "int32":
            output += (
                f"    msg.{field_name} = (static_cast<int32_t>(data[offset]) << 24) |\n"
            )
            output += "        (static_cast<int32_t>(data[offset + 1]) << 16) |\n"
            output += "        (static_cast<int32_t>(data[offset + 2]) << 8) |\n"
            output += "        static_cast<int32_t>(data[offset + 3]);\n"
            output += "    offset += 4;\n"
        elif field_type == "int64":
            output += (
                f"    msg.{field_name} = (static_cast<int64_t>(data[offset]) << 56) |\n"
            )
            output += "        (static_cast<int64_t>(data[offset + 1]) << 48) |\n"
            output += "        (static_cast<int64_t>(data[offset + 2]) << 40) |\n"  # ← | pas ;
            output += "        (static_cast<int64_t>(data[offset + 3]) << 32) |\n"
            output += "        (static_cast<int64_t>(data[offset + 4]) << 24) |\n"
            output += "        (static_cast<int64_t>(data[offset + 5]) << 16) |\n"
            output += "        (static_cast<int64_t>(data[offset + 6]) << 8) |\n"
            output += "        static_cast<int64_t>(data[offset + 7]);\n"
            output += "    offset += 8;\n"
        elif field_type == "float":
            output += (
                "    uint32_t temp = (static_cast<uint32_t>(data[offset]) << 24) |\n"
            )
            output += "        (static_cast<uint32_t>(data[offset + 1]) << 16) |\n"
            output += "        (static_cast<uint32_t>(data[offset + 2]) << 8) |\n"
            output += "        static_cast<uint32_t>(data[offset + 3]);\n"
            output += f"    std::memcpy(&msg.{field_name}, &temp, sizeof(float));\n"
            output += "    offset += 4;\n"
        elif field_type == "double":
            output += (
                "    uint64_t temp = (static_cast<uint64_t>(data[offset]) << 56) |\n"
            )
            output += "        (static_cast<uint64_t>(data[offset + 1]) << 48) |\n"
            output += "        (static_cast<uint64_t>(data[offset + 2]) << 40) |\n"
            output += "        (static_cast<uint64_t>(data[offset + 3]) << 32) |\n"
            output += "        (static_cast<uint64_t>(data[offset + 4]) << 24) |\n"
            output += "        (static_cast<uint64_t>(data[offset + 5]) << 16) |\n"
            output += "        (static_cast<uint64_t>(data[offset + 6]) << 8) |\n"
            output += "        static_cast<uint64_t>(data[offset + 7]);\n"
            output += f"    std::memcpy(&msg.{field_name}, &temp, sizeof(double));\n"
            output += "    offset += 8;\n"

        output += "\n"

    output += "    return msg;\n"
    output += "}\n\n"

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

    header = generate_header(protocol)
    write_file("include/Network/generated_messages.hpp", header)
    source = generate_source(protocol)
    write_file("src/generated_messages.cpp", source)


if __name__ == "__main__":
    main()
