#!/usr/bin/python
# -*- coding: utf-8 -*-

import json
import sys
import os
import argparse
import glob
#import cStringIO

reload(sys)
sys.setdefaultencoding("utf-8")

try: import simplejson as json
except: import json

class TEMPLATE_CONSTANTS:
    TYPE = "Type"
    NAME = "Name"
    IDENTIFIER = "Identifier"
    DESCRIPTION = "Description"
    SPEC = "Spec"
    DATATYPE = "DataType"
    PROPERTY = "Property"
    EVENT = "Event"
    COMMADN = "Command"
    TEMPLATE = "Template"

class iot_filed:
    def __init__(self, id, index, field_obj, prefix):
        self.default_value = ""
        self.index = index
        self.id = id
        self.type_name = field_obj["DataType"]["Type"]
        self.prefix = prefix

        if self.type_name == "bool":
            self.default_value = 0
        elif self.type_name == "enum":
            self.default_value = 0
        elif self.type_name == "float32":
            self.default_value = 0.0
        elif self.type_name == "float64":
            self.default_value = 0.0
        elif self.type_name == "int32":
            self.default_value = 0
        elif self.type_name == "string":
            if TEMPLATE_CONSTANTS.DATATYPE not in field_obj:
                raise ValueError("error：{} can't find{}".format(id, TEMPLATE_CONSTANTS.DATATYPE))

            string_defs = field_obj["DataType"]["Spec"]
            self.type_length = string_defs["Length"]
            self.default_value = "{'\\0'}"
        elif self.type_name == "date":
            self.default_value = 0
        elif self.type_name == "struct":
            if TEMPLATE_CONSTANTS.DATATYPE not in field_obj:
                raise ValueError("error: {} can't find{}".format(id, TEMPLATE_CONSTANTS.DATATYPE))

            struct_defs = field_obj["DataType"]["Spec"]
            self.struct_member_num = len(struct_defs)
            self.struct_member = struct_defs
        elif self.type_name == "array":
            if TEMPLATE_CONSTANTS.DATATYPE not in field_obj:
                raise ValueError("error: {} can't find{}".format(id, TEMPLATE_CONSTANTS.DATATYPE))
            self.item_type = field_obj["DataType"]["Spec"]["Item"]["Type"]
            self.array_num = field_obj["DataType"]["Spec"]["Size"]
            if self.item_type == "struct":
                array_item = field_obj["DataType"]["Spec"]["Item"]["Spec"]
                self.item_member_num = len(array_item)
                self.array_item = array_item

        else:
            raise ValueError('{} illegal type={} ,effective type：date,string,bool,enum,float64,float32,int32,struct,array'.format(id, field_obj["Type"]))

    def get_property_declar_name(self):
        declar_info = ""
        if self.type_name == "struct":
            declar_info += "DM_Property_t {};\n".format("{}_".format(self.prefix) + self.id)
            declar_info += "DM_Type_Struct_t st_{};\n".format("{}_".format(self.prefix) + self.id)
            declar_info += "DM_Node_t node_{}[{}];\n".format("{}_".format(self.prefix) + self.id, self.struct_member_num)
            return declar_info
        elif self.type_name == "array":
            if self.item_type == "struct":
                declar_info += "DM_Property_t {};\n".format("{}_".format(self.prefix) + self.id)
                declar_info += "DM_Array_Struct_t arr_st_{};\n".format("{}_".format(self.prefix) + self.id)
                declar_info += "DM_Type_Struct_t st_{}[{}];\n".format("{}_".format(self.prefix) + self.id, self.array_num)
                declar_info += "DM_Node_t node_{}[{}][{}];\n".format("{}_".format(self.prefix) + self.id, self.array_num, self.item_member_num)
                return declar_info
            else:
                declar_info += "DM_Property_t {};\n".format("{}_".format(self.prefix) + self.id)
                declar_info += "DM_Array_Base_t arr_base_{};\n".format("{}_".format(self.prefix) + self.id)
                declar_info += "DM_Node_t node_{}[{}];\n".format("{}_".format(self.prefix) + self.id, self.array_num)
                return declar_info
        else:
            declar_info += "DM_Property_t {};\n".format("{}_".format(self.prefix) + self.id)
            declar_info += "DM_Node_t node_{};\n".format("{}_".format(self.prefix) + self.id)
            return declar_info

    def get_property_config_member(self):
        config_info = ""
        loop = 0
        type_enum = {
                        "int32": "TYPE_INT",
                        "float32": "TYPE_FLOAT",
                        "float64": "TYPE_DOUBLE",
                        "bool": "TYPE_BOOL",
                        "enum": "TYPE_ENUM",
                        "string": "TYPE_STRING",
                        "date": "TYPE_DATE"
        }

        default_value_enum = {
                        "int32": 0,
                        "float32": 0.0,
                        "float64": 0.0,
                        "bool": 0,
                        "enum": 0,
                        "string": "NULL",
                        "date": 0
        }

        if self.type_name == "struct":
            for struct_member in self.struct_member:
                config_info += "    #define {} {}\n".format((self.prefix + "_" +struct_member["Identifier"] + "_index").upper(), loop)
                config_info += "    node_{}[{}].base_type = {};\n".format("{}_".format(self.prefix) + self.id, (self.prefix + "_" +struct_member["Identifier"] + "_index").upper(), type_enum[struct_member["DataType"]["Type"]])
                config_info += "    node_{}[{}].key = \"{}\";\n".format("{}_".format(self.prefix) + self.id, (self.prefix + "_" +struct_member["Identifier"] + "_index").upper(), struct_member["Identifier"])
                config_info += "    node_{}[{}].value.{} = {};\n".format("{}_".format(self.prefix) + self.id, (self.prefix + "_" +struct_member["Identifier"] + "_index").upper(), "{}_value".format(struct_member["DataType"]["Type"]), default_value_enum[struct_member["DataType"]["Type"]])
                loop += 1

            config_info += "    st_{}.key = \"{}\";\n".format("{}_".format(self.prefix) + self.id, self.id)
            config_info += "    st_{}.value = node_{};\n".format("{}_".format(self.prefix) + self.id, "{}_".format(self.prefix) + self.id)
            config_info += "    st_{}.num = {};\n".format("{}_".format(self.prefix) + self.id, self.struct_member_num)

            config_info += "    {}.parse_type = TYPE_STRUCT;\n".format("{}_".format(self.prefix) + self.id)
            config_info += "    {}.value.dm_struct = &st_{};\n".format("{}_".format(self.prefix) + self.id, "{}_".format(self.prefix) + self.id)
            return config_info
        elif self.type_name == "array":
            if self.item_type == "struct":
                loop_array = 0
                while loop_array < int(self.array_num):
                    loop = 0
                    for item_member in self.array_item:
                        if loop_array == 0:
                            config_info += "    #define {} {}\n".format((self.prefix + "_" +item_member["Identifier"] + "_index").upper(), loop)
                        config_info += "    node_{}[{}][{}].base_type = {};\n".format("{}_".format(self.prefix) + self.id, loop_array, (self.prefix + "_" +item_member["Identifier"] + "_index").upper(), type_enum[item_member["DataType"]["Type"]])
                        config_info += "    node_{}[{}][{}].key = \"{}\";\n".format("{}_".format(self.prefix) + self.id, loop_array, (self.prefix + "_" +item_member["Identifier"] + "_index").upper(), item_member["Identifier"])
                        config_info += "    node_{}[{}][{}].value.{} = {};\n".format("{}_".format(self.prefix) + self.id, loop_array, (self.prefix + "_" +item_member["Identifier"] + "_index").upper(), "{}_value".format(item_member["DataType"]["Type"]), default_value_enum[item_member["DataType"]["Type"]])
                        loop += 1

                    config_info += "    st_{}[{}].value = node_{}[{}];\n".format("{}_".format(self.prefix) + self.id, loop_array, "{}_".format(self.prefix) + self.id, loop_array)
                    config_info += "    st_{}[{}].num = {};\n".format("{}_".format(self.prefix) + self.id, loop_array, self.item_member_num)
                    loop_array += 1

                config_info += "    arr_st_{}.key = \"{}\";\n".format("{}_".format(self.prefix) + self.id, self.id)
                config_info += "    arr_st_{}.value = st_{};\n".format("{}_".format(self.prefix) + self.id, "{}_".format(self.prefix) + self.id)
                config_info += "    arr_st_{}.num = {};\n".format("{}_".format(self.prefix) + self.id, self.array_num)
                config_info += "    {}.parse_type = TYPE_ARRAY_STRUCT;\n".format("{}_".format(self.prefix) + self.id)
                config_info += "    {}.value.dm_array_struct = &arr_st_{};\n".format("{}_".format(self.prefix) + self.id, "{}_".format(self.prefix) + self.id)
                return config_info
            else:
                loop_array = 0
                while loop_array < int(self.array_num):
                    config_info += "    node_{}[{}].base_type = {};\n".format("{}_".format(self.prefix) + self.id, loop_array, type_enum[self.item_type])
                    config_info += "    node_{}[{}].value.{} = {};\n".format("{}_".format(self.prefix) + self.id, loop_array, "{}_value".format(self.item_type), default_value_enum[self.item_type])
                    loop_array += 1

                config_info += "    arr_base_{}.key = \"{}\";\n".format("{}_".format(self.prefix) + self.id, self.id)
                config_info += "    arr_base_{}.value = node_{};\n".format("{}_".format(self.prefix) + self.id, "{}_".format(self.prefix) + self.id)
                config_info += "    arr_base_{}.num = {};\n".format("{}_".format(self.prefix) + self.id, self.array_num)
                config_info += "    {}.parse_type = TYPE_ARRAY_BASE;\n".format("{}_".format(self.prefix) + self.id)
                config_info += "    {}.value.dm_array_base = &arr_base_{};\n".format("{}_".format(self.prefix) + self.id, "{}_".format(self.prefix) + self.id)
                return config_info
        else:
            config_info += "    node_{}.base_type = {};\n".format("{}_".format(self.prefix) + self.id, type_enum[self.type_name])
            config_info += "    node_{}.key = \"{}\";\n".format("{}_".format(self.prefix) + self.id, self.id)
            config_info += "    node_{}.value.{} = {};\n".format("{}_".format(self.prefix) + self.id, "{}_value".format(self.type_name), default_value_enum[self.type_name])

            config_info += "    {}.parse_type = TYPE_NODE;\n".format("{}_".format(self.prefix) + self.id)
            config_info += "    {}.value.dm_node = &node_{};\n".format("{}_".format(self.prefix) + self.id, "{}_".format(self.prefix) + self.id)
            return config_info

class iot_event:
    def __init__(self, id, event_obj):
        self.default_value = ""
        self.id = id
        self.type_name = event_obj["Type"]
        self.output = event_obj["Output"]
        self.output_num = len(event_obj["Output"])

    def get_event_declar_name(self):
        declar_info = ""
        declar_info += "DM_Event_t event_{}_{};\n".format(self.id, self.type_name)
        declar_info += "DM_Property_t {}[{}];\n".format(self.id, self.output_num)
        return declar_info

    def get_event_config_member(self):
        config_info = ""
        loop = 0
        for output_obj in self.output:
            config_info += "    {}[{}] = {};\n".format(self.id, loop, "event_{}_".format(self.id) + output_obj["Identifier"])
            loop += 1
        config_info += "    event_{}_{}.event_identy = \"{}\";\n".format(self.id, self.type_name, self.id)
        config_info += "    event_{}_{}.dm_property = {};\n".format(self.id, self.type_name, self.id)
        config_info += "    event_{}_{}.property_num = {};\n".format(self.id, self.type_name, self.output_num)

        return config_info

class input_json_parse:
    def __init__(self, cmd_id, input):
        self.cmd_id = cmd_id
        self.input = input
        self.type_name = input["DataType"]["Type"]

    def get_input_config(self):
        input_config = ""
        input_config += "    if(0 == strncmp(cmd_id, \"{}\", strlen(\"{}\")))\n".format(self.cmd_id, self.cmd_id)
        input_config += "    {\n"
        input_config += "        char *{} = NULL;\n".format(self.cmd_id + "_" + self.input["Identifier"])
        input_config += "        {} = LITE_json_value_of((char *)\"{}\", (char *)input);\n".format(self.cmd_id + "_" +  self.input["Identifier"], self.input["Identifier"])
        if self.type_name == "struct":
            for input_struct_item in self.input["DataType"]["Spec"]:
                input_config += "        char *{} = NULL;\n".format(self.cmd_id + "_" +  input_struct_item["Identifier"])
                input_config += "        {} = LITE_json_value_of((char *)\"{}\", (char *){});\n".format(self.cmd_id + "_" +  input_struct_item["Identifier"], input_struct_item["Identifier"], self.cmd_id + "_" + self.input["Identifier"])
                if input_struct_item["DataType"]["Type"] == "int32" or input_struct_item["DataType"]["Type"] == "enum" or input_struct_item["DataType"]["Type"] == "bool" or input_struct_item["DataType"]["Type"] == "date":
                    input_config += "        node_cmd_input_{}_{}[{}].value.{} = atoi({});\n".format(self.cmd_id, self.input["Identifier"], "CMD_INPUT_{}_{}_INDEX".format(self.cmd_id, input_struct_item["Identifier"]).upper(), "{}_value".format(input_struct_item["DataType"]["Type"]), self.cmd_id + "_" + input_struct_item["Identifier"])
                elif input_struct_item["DataType"]["Type"] == "float32" or input_struct_item["DataType"]["Type"] == "float64":
                    input_config += "        node_cmd_input_{}_{}[{}].value.{} = atof({});\n".format(self.cmd_id, self.input["Identifier"], "CMD_INPUT_{}_{}_INDEX".format(self.cmd_id, input_struct_item["Identifier"]).upper(), "{}_value".format(input_struct_item["DataType"]["Type"]), self.cmd_id + "_" + input_struct_item["Identifier"])
                else:
                    input_config += "        if(NULL != node_cmd_input_{}_{}[{}].value.{})\n".format(self.cmd_id, self.input["Identifier"], "CMD_INPUT_{}_{}_INDEX".format(self.cmd_id, input_struct_item["Identifier"]).upper(), "{}_value".format(input_struct_item["DataType"]["Type"]))
                    input_config += "            HAL_Free(node_cmd_input_{}_{}[{}].value.{});\n".format( self.cmd_id, self.input["Identifier"], "CMD_INPUT_{}_{}_INDEX".format(self.cmd_id, input_struct_item["Identifier"]).upper(), "{}_value".format(input_struct_item["DataType"]["Type"]))
                    input_config += "        node_cmd_input_{}_{}[{}].value.{} = HAL_Malloc(strlen({}) + 1);\n".format(self.cmd_id, self.input["Identifier"], "CMD_INPUT_{}_{}_INDEX".format(self.cmd_id, input_struct_item["Identifier"]).upper(), "{}_value".format(input_struct_item["DataType"]["Type"]), self.cmd_id + "_" + input_struct_item["Identifier"])
                    input_config += "        strncpy(node_cmd_input_{}_{}[{}].value.{}, {}, strlen({}));\n".format(self.cmd_id, self.input["Identifier"], "CMD_INPUT_{}_{}_INDEX".format(self.cmd_id, input_struct_item["Identifier"]).upper(), "{}_value".format(input_struct_item["DataType"]["Type"]), self.cmd_id + "_" + input_struct_item["Identifier"], self.cmd_id + "_" + input_struct_item["Identifier"])
                    input_config += "        node_cmd_input_{}_{}[{}].value.{}[strlen({})] = \'\\0\';\n".format(self.cmd_id, self.input["Identifier"], "CMD_INPUT_{}_{}_INDEX".format(self.cmd_id, input_struct_item["Identifier"]).upper(), "{}_value".format(input_struct_item["DataType"]["Type"]), self.cmd_id + "_" + input_struct_item["Identifier"])
                input_config += "        HAL_Free({});\n".format(self.cmd_id + "_" + input_struct_item["Identifier"])
        elif self.type_name == "array":
            input_config += "        char *{} = NULL;\n".format(self.cmd_id + "_" + self.input["Identifier"] + "_pos")
            input_config += "        char *{} = NULL;\n".format(self.cmd_id + "_" + self.input["Identifier"] + "_entry")
            input_config += "        int {} = 0;\n".format(self.cmd_id + "_" + self.input["Identifier"] + "_len")
            input_config += "        int {} = 0;\n".format(self.cmd_id + "_" + self.input["Identifier"] + "_type")
            input_config += "        int {}_loop = 0;\n".format(self.cmd_id + "_" + self.input["Identifier"])
            input_config += "        json_array_for_each_entry({}, {}, {}, {}, {})\n".format(self.cmd_id + "_" +  self.input["Identifier"],
                                                                                            self.cmd_id + "_" + self.input["Identifier"] + "_pos",
                                                                                            self.cmd_id + "_" + self.input["Identifier"] + "_entry",
                                                                                            self.cmd_id + "_" + self.input["Identifier"] + "_len",
                                                                                            self.cmd_id + "_" + self.input["Identifier"] + "_type")
            input_config += "        {\n"
            if self.input["DataType"]["Spec"]["Item"]["Type"] != "struct":
                if self.input["DataType"]["Spec"]["Item"]["Type"] == "int32" or self.input["DataType"]["Spec"]["Item"]["Type"] == "enum" or self.input["DataType"]["Spec"]["Item"]["Type"] == "bool" or self.input["DataType"]["Spec"]["Item"]["Type"] == "date":
                    input_config += "            node_cmd_input_{}_{}[{}_loop].value.{} = atoi({});\n".format(self.cmd_id, self.input["Identifier"], self.cmd_id + "_" + self.input["Identifier"], "{}_value".format(self.input["DataType"]["Spec"]["Item"]["Type"]), self.cmd_id + "_" + self.input["Identifier"] + "_entry")
                elif self.input["DataType"]["Spec"]["Item"]["Type"] == "float32" or self.input["DataType"]["Spec"]["Item"]["Type"] == "float64":
                    input_config += "            node_cmd_input_{}_{}[{}_loop].value.{} = atof({});\n".format(self.cmd_id, self.input["Identifier"], self.cmd_id + "_" + self.input["Identifier"], "{}_value".format(self.input["DataType"]["Spec"]["Item"]["Type"]), self.cmd_id + "_" + self.input["Identifier"] + "_entry")
                else:
                    input_config += "            if(NULL != node_cmd_input_{}_{}[{}_loop].value.{})\n".format(self.cmd_id, self.input["Identifier"], self.cmd_id + "_" + self.input["Identifier"], "{}_value".format(self.input["DataType"]["Spec"]["Item"]["Type"]))
                    input_config += "                HAL_Free(node_cmd_input_{}_{}[{}_loop].value.{});\n".format( self.cmd_id, self.input["Identifier"], self.cmd_id + "_" + self.input["Identifier"], "{}_value".format(self.input["DataType"]["Spec"]["Item"]["Type"]))
                    input_config += "            node_cmd_input_{}_{}[{}_loop].value.{} = HAL_Malloc({} + 1);\n".format(self.cmd_id, self.input["Identifier"], self.cmd_id + "_" + self.input["Identifier"], "{}_value".format(self.input["DataType"]["Spec"]["Item"]["Type"]), self.cmd_id + "_" + self.input["Identifier"] + "_len")
                    input_config += "            strncpy(node_cmd_input_{}_{}[{}_loop].value.{}, {}, {});\n".format(self.cmd_id, self.input["Identifier"], self.cmd_id + "_" + self.input["Identifier"], "{}_value".format(self.input["DataType"]["Spec"]["Item"]["Type"]), self.cmd_id + "_" + self.input["Identifier"] + "_entry", self.cmd_id + "_" + self.input["Identifier"] + "_len")
                    input_config += "            node_cmd_input_{}_{}[{}_loop].value.{}[{}] = \'\\0\';\n".format(self.cmd_id, self.input["Identifier"], self.cmd_id + "_" + self.input["Identifier"], "{}_value".format(self.input["DataType"]["Spec"]["Item"]["Type"]), self.cmd_id + "_" + self.input["Identifier"] + "_len")
                input_config += "            {}_loop++;\n".format(self.cmd_id + "_" + self.input["Identifier"])
            else:
                for array_struct_item in self.input["DataType"]["Spec"]["Item"]["Spec"]:
                    input_config += "            char *{} = NULL;\n".format(self.cmd_id + "_" + array_struct_item["Identifier"])
                    input_config += "            {} = LITE_json_value_of((char *)\"{}\", (char *){});\n".format(self.cmd_id + "_" + array_struct_item["Identifier"], array_struct_item["Identifier"],self.cmd_id + "_" + self.input["Identifier"] + "_entry")
                    if array_struct_item["DataType"]["Type"] == "int32" or array_struct_item["DataType"]["Type"] == "enum" or array_struct_item["DataType"]["Type"] == "bool" or array_struct_item["DataType"]["Type"] == "date":
                        input_config += "            node_cmd_input_{}_{}[{}_loop][{}].value.{} = atoi({});\n".format(self.cmd_id, self.input["Identifier"], self.cmd_id + "_" + self.input["Identifier"], "CMD_INPUT_{}_{}_INDEX".format(self.cmd_id, array_struct_item["Identifier"]).upper(), "{}_value".format(array_struct_item["DataType"]["Type"]), self.cmd_id + "_" + array_struct_item["Identifier"])
                    elif array_struct_item["DataType"]["Type"] == "float32" or array_struct_item["DataType"]["Type"] == "float64":
                        input_config += "            node_cmd_input_{}_{}[{}_loop][{}].value.{} = atof({});\n".format(self.cmd_id, self.input["Identifier"], self.cmd_id + "_" + self.input["Identifier"], "CMD_INPUT_{}_{}_INDEX".format(self.cmd_id, array_struct_item["Identifier"]).upper(), "{}_value".format(array_struct_item["DataType"]["Type"]), self.cmd_id + "_" + array_struct_item["Identifier"])
                    else:
                        input_config += "            if(NULL != node_cmd_input_{}_{}[{}_loop][{}].value.{})\n".format(self.cmd_id, self.input["Identifier"], self.cmd_id + "_" + self.input["Identifier"], "CMD_INPUT_{}_{}_INDEX".format(self.cmd_id, array_struct_item["Identifier"]).upper(), "{}_value".format(array_struct_item["DataType"]["Type"]))
                        input_config += "                HAL_Free(node_cmd_input_{}_{}[{}_loop][{}].value.{});\n".format( self.cmd_id, self.input["Identifier"], self.cmd_id + "_" + self.input["Identifier"], "CMD_INPUT_{}_{}_INDEX".format(self.cmd_id, array_struct_item["Identifier"]).upper(), "{}_value".format(array_struct_item["DataType"]["Type"]))
                        input_config += "            node_cmd_input_{}_{}[{}_loop][{}].value.{} = HAL_Malloc(strlen({}) + 1);\n".format(self.cmd_id, self.input["Identifier"], self.cmd_id + "_" + self.input["Identifier"], "CMD_INPUT_{}_{}_INDEX".format(self.cmd_id, array_struct_item["Identifier"]).upper(), "{}_value".format(array_struct_item["DataType"]["Type"]), self.cmd_id + "_" + array_struct_item["Identifier"])
                        input_config += "            strncpy(node_cmd_input_{}_{}[{}_loop][{}].value.{}, {}, strlen({}));\n".format(self.cmd_id, self.input["Identifier"], self.cmd_id + "_" + self.input["Identifier"], "CMD_INPUT_{}_{}_INDEX".format(self.cmd_id, array_struct_item["Identifier"]).upper(), "{}_value".format(array_struct_item["DataType"]["Type"]), self.cmd_id + "_" + array_struct_item["Identifier"], self.cmd_id + "_" + array_struct_item["Identifier"])
                        input_config += "            node_cmd_input_{}_{}[{}_loop][{}].value.{}[strlen({})] = \'\\0\';\n".format(self.cmd_id, self.input["Identifier"], self.cmd_id + "_" + self.input["Identifier"], "CMD_INPUT_{}_{}_INDEX".format(self.cmd_id, array_struct_item["Identifier"]).upper(), "{}_value".format(array_struct_item["DataType"]["Type"]), self.cmd_id + "_" + array_struct_item["Identifier"])
                    input_config += "            HAL_Free({});\n".format(self.cmd_id + "_" + array_struct_item["Identifier"])
                input_config += "            {}_loop++;\n".format(self.cmd_id + "_" + self.input["Identifier"])
            input_config += "        }\n"
        elif self.type_name == "int32" or self.type_name == "enum" or self.type_name == "bool" or self.type_name == "date":
            input_config += "        node_cmd_input_{}_{}.value.{} = atoi({});\n".format(self.cmd_id, self.input["Identifier"], "{}_value".format(self.type_name), self.cmd_id + "_" + self.input["Identifier"])
        elif self.type_name == "float32" or self.type_name == "float64":
            input_config += "        node_cmd_input_{}_{}.value.{} = atof({});\n".format(self.cmd_id, self.input["Identifier"], "{}_value".format(self.type_name), self.cmd_id + "_" + self.input["Identifier"])
        else:
            input_config += "        node_cmd_input_{}_{}.value.{} = {};\n".format(self.cmd_id, self.input["Identifier"], "{}_value".format(self.type_name), self.cmd_id + "_" + self.input["Identifier"])
            input_config += "        if(NULL != node_cmd_input_{}_{}.value.{})\n".format(self.cmd_id, self.input["Identifier"], "{}_value".format(self.input["DataType"]["Type"]))
            input_config += "            HAL_Free(node_cmd_input_{}_{}.value.{});\n".format(self.cmd_id, self.input["Identifier"], "{}_value".format(self.input["DataType"]["Type"]))
            input_config += "        node_cmd_input_{}_{}.value.{} = HAL_Malloc(strlen({}) + 1);\n".format(self.cmd_id, self.input["Identifier"], "{}_value".format(self.input["DataType"]["Type"]), self.cmd_id + "_" + self.input["Identifier"])
            input_config += "        strncpy(node_cmd_input_{}_{}[{}].value.{}, {}, strlen({}));\n".format(self.cmd_id, self.input["Identifier"], "{}_value".format(self.input["DataType"]["Type"]), self.cmd_id + "_" + self.input["Identifier"], self.cmd_id + "_" + self.input["Identifier"])
            input_config += "        node_cmd_input_{}_{}.value.{}[strlen({})] = \'\\0\';\n".format(self.cmd_id, self.input["Identifier"], "{}_value".format(self.input["DataType"]["Type"]), self.cmd_id + "_" + self.input["Identifier"])
        input_config += "        HAL_Free({});\n".format(self.cmd_id + "_" +  self.input["Identifier"])
        input_config += "        return;\n"
        input_config += "    }\n"
        return input_config

class iot_json_parse:
    def __init__(self, model):
        self.fields = []
        self.field_id = 0
        self.events_properties = []
        self.events_property_id = 0
        self.events_property_index = 0
        self.events = []
        self.event_id = 0
        self.input_parse = []
        self.input = []
        self.input_id = 0
        self.output = []
        self.output_id = 0
        for field_define in model["Property"]:
            if TEMPLATE_CONSTANTS.NAME not in field_define:
                raise ValueError("error: can,t find Name")
            self.fields.append(iot_filed(field_define["Identifier"], self.field_id, field_define, "property"))
            self.field_id += 1

        for event in model["Event"]:
            if TEMPLATE_CONSTANTS.NAME not in event:
                raise ValueError("error: can,t find Name")
            for event_item in event["Output"]:
                self.events_properties.append(iot_filed(event_item["Identifier"], self.events_property_id, event_item, "event_" + event["Identifier"]))
                self.events_property_id += 1

            self.events.append(iot_event(event["Identifier"], event))

            self.event_id += 1

        for command_input in model["Command"]:
            if TEMPLATE_CONSTANTS.NAME not in command_input:
                raise ValueError("error: can,t find Name")
            for input_item in command_input["Input"]:
                self.input.append(iot_filed(input_item["Identifier"], self.input_id, input_item, "cmd_input_" + command_input["Identifier"]))
                self.input_parse.append(input_json_parse(command_input["Identifier"], input_item))
                self.input_id += 1

        for command_output in model["Command"]:
            if TEMPLATE_CONSTANTS.NAME not in command_output:
                raise ValueError("error: can,t find Name")
            for output_item in command_output["Output"]:
                self.output.append(iot_filed(output_item["Identifier"], self.output_id, output_item, "cmd_output_" + command_output["Identifier"]))
                self.output_id += 1

    def gen_data_config(self):
        declar = ""
        config = ""
        config += "static void _init_data_template(){\n"
        for field in self.fields:
            declar += "{}\n".format(field.get_property_declar_name())
            config += "{}\n".format(field.get_property_config_member())
        config += "}\n"
        result = declar + config
        return result

    def gen_event_config(self):
        properties_declar = ""
        properties_config = ""
        properties_config += "static void _init_event_property_template(){\n"
        for event_properties in self.events_properties:
            properties_declar += "{}\n".format(event_properties.get_property_declar_name())
            properties_config += "{}\n".format(event_properties.get_property_config_member())
        properties_config += "}\n"

        event_declar = ""
        event_config = ""
        event_config += "static void _init_event_template(){\n"
        event_config += "    _init_event_property_template();\n"
        for event_obj in self.events:
            event_declar += "{}\n".format(event_obj.get_event_declar_name())
            event_config += "{}\n".format(event_obj.get_event_config_member())
        event_config += "}\n"
        result = properties_declar + properties_config + event_declar + event_config
        return result

    def gen_command_config(self):
        cmd_input_declar = ""
        cmd_input_config = ""
        cmd_input_config += "static void _init_command_input_template(){\n"
        for command_input_property in self.input:
            cmd_input_declar += "{}\n".format(command_input_property.get_property_declar_name())
            cmd_input_config += "{}\n".format(command_input_property.get_property_config_member())
        cmd_input_config += "}\n"

        cmd_input_parse = ""
        cmd_input_parse += "static void _input_parse_config(const char *cmd_id, const char *input){\n"
        for cmd_input_parse_item in self.input_parse:
            cmd_input_parse += "{}\n".format(cmd_input_parse_item.get_input_config())
        cmd_input_parse += "}\n"

        cmd_output_declar = ""
        cmd_output_config = ""
        cmd_output_config += "static void _init_command_output_template(){\n"
        for command_output_property in self.output:
            cmd_output_declar += "{}\n".format(command_output_property.get_property_declar_name())
            cmd_output_config += "{}\n".format(command_output_property.get_property_config_member())
        cmd_output_config += "}\n"
        result = cmd_input_declar + cmd_input_config + cmd_input_parse + cmd_output_declar + cmd_output_config
        return result

def main():
    parser = argparse.ArgumentParser(description='Iothub datatemplate events and command config code generator.', usage='use "./codegen.py -c xx/config.json" gen config code')
    parser.add_argument('-c', '--config', dest='config', metavar='xxx.json', required=False, default='xxx.json',
                        help='copy the generated file (data_config.c events_config.c and command_config.c) to datatemplate_sample dir '
                             'or your own code dir with datatemplate. ')
    parser.add_argument('-d', '--dest', dest='dest', required=False, default='.',
                        help='Dest directory for generated code files, no / at the end.')

    args = parser.parse_args()

    config_path = args.config
    if not os.path.exists(config_path):
        print(u"error: config file not exist ./codegen.py -c xx/data_template.json".format(config_path))
        return 1

    config_dir = os.path.dirname(config_path)
    if config_dir:
        config_dir += "/"

    f = open(config_path, "r")
    try:
        thingmodel = json.load(f)
        if 'Property' not in thingmodel:
            thingmodel.properties = []

        if 'Event' not in thingmodel:
            thingmodel.events = []

        if 'Command' not in thingmodel:
            thingmodel.command = []
        print(u"load {} file successfully".format(config_path))
    except ValueError:
        print(u"error: illegal JSON file{}".format(config_path))
        return 1

    try:
        snippet = iot_json_parse(thingmodel)

        output_data_config_file_name = args.dest + "/property_config.c"
        output_file = open(output_data_config_file_name, "w")
        output_file.write("{}".format(snippet.gen_data_config()))
        output_file.close()

        output_event_config_file_name = args.dest + "/event_config.c"
        output_file = open(output_event_config_file_name, "w")
        output_file.write("{}".format(snippet.gen_event_config()))
        output_file.close()

        output_command_config_file_name = args.dest + "/command_config.c"
        output_file = open(output_command_config_file_name, "w")
        output_file.write("{}".format(snippet.gen_command_config()))
        output_file.close()

        print(u"file {} release successful".format(output_data_config_file_name))
        print(u"file {} release successful".format(output_event_config_file_name))
        print(u"file {} release successful".format(output_command_config_file_name))

        return 0
    except ValueError as e:
        print(e)
        return 1


if __name__ == '__main__':
    sys.exit(main())

