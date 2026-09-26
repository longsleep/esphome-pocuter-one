#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome::mxc4005 {

// Values are the FSR[1:0] field of the control register.
enum MXC4005Range : uint8_t {
  RANGE_2G = 0,
  RANGE_4G = 1,
  RANGE_8G = 2,
};

// MEMSIC MXC4005XC 3-axis thermal accelerometer. Polled only; the INT line
// (shake and orientation events) is left to the pin it is wired to.
class MXC4005Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;

  void set_range(MXC4005Range range) { this->range_ = range; }
  void set_accel_x_sensor(sensor::Sensor *s) { this->accel_x_sensor_ = s; }
  void set_accel_y_sensor(sensor::Sensor *s) { this->accel_y_sensor_ = s; }
  void set_accel_z_sensor(sensor::Sensor *s) { this->accel_z_sensor_ = s; }
  void set_temperature_sensor(sensor::Sensor *s) { this->temperature_sensor_ = s; }

 protected:
  MXC4005Range range_{RANGE_2G};
  uint8_t device_id_{0};
  sensor::Sensor *accel_x_sensor_{nullptr};
  sensor::Sensor *accel_y_sensor_{nullptr};
  sensor::Sensor *accel_z_sensor_{nullptr};
  sensor::Sensor *temperature_sensor_{nullptr};
};

}  // namespace esphome::mxc4005
