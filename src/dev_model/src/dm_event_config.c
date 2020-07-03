#include "dm_event.h"
DM_Property_t event_high_temp_temperature;
DM_Node_t node_event_high_temp_temperature;

void _init_event_property_template(){
    node_event_high_temp_temperature.base_type = TYPE_FLOAT;
    node_event_high_temp_temperature.key = "temperature";
    node_event_high_temp_temperature.value.float32_value = 0.0;
    event_high_temp_temperature.parse_type = TYPE_NODE;
    event_high_temp_temperature.value.dm_node = &node_event_high_temp_temperature;

}
DM_Event_t event_high_temp_warning;
DM_Property_t high_temp[1];

void _init_event_template(){
    _init_event_property_template();
    high_temp[0] = event_high_temp_temperature;
    event_high_temp_warning.event_identy = "high_temp";
    event_high_temp_warning.dm_property = high_temp;
    event_high_temp_warning.property_num = 1;

}
