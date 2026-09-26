import esphome.codegen as cg
from esphome.components import i2c, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ACCELERATION_X,
    CONF_ACCELERATION_Y,
    CONF_ACCELERATION_Z,
    CONF_ID,
    CONF_RANGE,
    CONF_TEMPERATURE,
    DEVICE_CLASS_TEMPERATURE,
    ICON_ACCELERATION_X,
    ICON_ACCELERATION_Y,
    ICON_ACCELERATION_Z,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_METER_PER_SECOND_SQUARED,
)

DEPENDENCIES = ["i2c"]

mxc4005_ns = cg.esphome_ns.namespace("mxc4005")
MXC4005Component = mxc4005_ns.class_(
    "MXC4005Component", cg.PollingComponent, i2c.I2CDevice
)

# FSR[1:0] in the control register, bits 6:5. Sensitivity halves per step.
MXC4005Range = mxc4005_ns.enum("MXC4005Range")
RANGES = {
    "2G": MXC4005Range.RANGE_2G,
    "4G": MXC4005Range.RANGE_4G,
    "8G": MXC4005Range.RANGE_8G,
}


def _accel_schema(icon):
    return sensor.sensor_schema(
        unit_of_measurement=UNIT_METER_PER_SECOND_SQUARED,
        icon=icon,
        accuracy_decimals=2,
        state_class=STATE_CLASS_MEASUREMENT,
    )


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MXC4005Component),
            cv.Optional(CONF_RANGE, default="2G"): cv.enum(RANGES, upper=True),
            cv.Optional(CONF_ACCELERATION_X): _accel_schema(ICON_ACCELERATION_X),
            cv.Optional(CONF_ACCELERATION_Y): _accel_schema(ICON_ACCELERATION_Y),
            cv.Optional(CONF_ACCELERATION_Z): _accel_schema(ICON_ACCELERATION_Z),
            # The on-die sensor. It tracks the board, not the room: the
            # ESP32-C3 and the charger sit right next to it.
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x15))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.set_range(config[CONF_RANGE]))
    for key, setter in (
        (CONF_ACCELERATION_X, "set_accel_x_sensor"),
        (CONF_ACCELERATION_Y, "set_accel_y_sensor"),
        (CONF_ACCELERATION_Z, "set_accel_z_sensor"),
        (CONF_TEMPERATURE, "set_temperature_sensor"),
    ):
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(var, setter)(sens))
