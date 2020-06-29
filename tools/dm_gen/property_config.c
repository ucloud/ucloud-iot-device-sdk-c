DM_Property_t property_humidity;
DM_Node_t node_property_humidity;

DM_Property_t property_temperature;
DM_Node_t node_property_temperature;

static void _init_data_template(){
    node_property_humidity.base_type = TYPE_FLOAT;
    node_property_humidity.key = "humidity";
    node_property_humidity.value.float32_value = 0.0;
    property_humidity.parse_type = TYPE_NODE;
    property_humidity.value.dm_node = &node_property_humidity;

    node_property_temperature.base_type = TYPE_FLOAT;
    node_property_temperature.key = "temperature";
    node_property_temperature.value.float32_value = 0.0;
    property_temperature.parse_type = TYPE_NODE;
    property_temperature.value.dm_node = &node_property_temperature;

}
