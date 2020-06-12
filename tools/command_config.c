DM_Property_t cmd_output_temp_modify_modify_result;
DM_Node_t node_cmd_output_temp_modify_modify_result;

DM_Property_t cmd_output_temp_modify_effect_temp_modify;
DM_Node_t node_cmd_output_temp_modify_effect_temp_modify;

static void _init_command_output_template(){
    node_cmd_output_temp_modify_modify_result.base_type = TYPE_BOOL;
    node_cmd_output_temp_modify_modify_result.key = "modify_result";
    node_cmd_output_temp_modify_modify_result.value.bool_value = 0;
    cmd_output_temp_modify_modify_result.parse_type = TYPE_NODE;
    cmd_output_temp_modify_modify_result.value.dm_node = &node_cmd_output_temp_modify_modify_result;

    node_cmd_output_temp_modify_effect_temp_modify.base_type = TYPE_INT;
    node_cmd_output_temp_modify_effect_temp_modify.key = "effect_temp_modify";
    node_cmd_output_temp_modify_effect_temp_modify.value.int32_value = 0;
    cmd_output_temp_modify_effect_temp_modify.parse_type = TYPE_NODE;
    cmd_output_temp_modify_effect_temp_modify.value.dm_node = &node_cmd_output_temp_modify_effect_temp_modify;

}
