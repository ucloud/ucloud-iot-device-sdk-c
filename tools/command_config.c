DM_Property_t cmd_input_set_temp_correction_temp_correction;
DM_Node_t node_cmd_input_set_temp_correction_temp_correction;

static void _init_command_input_template(){
    node_cmd_input_set_temp_correction_temp_correction.base_type = TYPE_INT;
    node_cmd_input_set_temp_correction_temp_correction.key = "temp_correction";
    node_cmd_input_set_temp_correction_temp_correction.value.int32_value = 0;
    cmd_input_set_temp_correction_temp_correction.parse_type = TYPE_NODE;
    cmd_input_set_temp_correction_temp_correction.value.dm_node = &node_cmd_input_set_temp_correction_temp_correction;

}
DM_Property_t cmd_output_set_temp_correction_correction_result;
DM_Node_t node_cmd_output_set_temp_correction_correction_result;

DM_Property_t cmd_output_set_temp_correction_effect_temp_correction;
DM_Node_t node_cmd_output_set_temp_correction_effect_temp_correction;

static void _init_command_output_template(){
    node_cmd_output_set_temp_correction_correction_result.base_type = TYPE_BOOL;
    node_cmd_output_set_temp_correction_correction_result.key = "correction_result";
    node_cmd_output_set_temp_correction_correction_result.value.bool_value = 0;
    cmd_output_set_temp_correction_correction_result.parse_type = TYPE_NODE;
    cmd_output_set_temp_correction_correction_result.value.dm_node = &node_cmd_output_set_temp_correction_correction_result;

    node_cmd_output_set_temp_correction_effect_temp_correction.base_type = TYPE_INT;
    node_cmd_output_set_temp_correction_effect_temp_correction.key = "effect_temp_correction";
    node_cmd_output_set_temp_correction_effect_temp_correction.value.int32_value = 0;
    cmd_output_set_temp_correction_effect_temp_correction.parse_type = TYPE_NODE;
    cmd_output_set_temp_correction_effect_temp_correction.value.dm_node = &node_cmd_output_set_temp_correction_effect_temp_correction;

}
