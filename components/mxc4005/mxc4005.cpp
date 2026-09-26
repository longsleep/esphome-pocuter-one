#include "mxc4005.h"
#include "esphome/core/log.h"

namespace esphome::mxc4005 {

static const char *const TAG = "mxc4005";

static const uint8_t REG_XOUT_UPPER = 0x03;  // X, Y, Z upper/lower, then TOUT
static const uint8_t REG_CONTROL = 0x0D;     // FSR[1:0] at 6:5, PD at 0
static const uint8_t REG_DEVICE_ID = 0x0E;

static const float GRAVITY = 9.80665f;
// 12 bit two's complement, 1024 LSB/g at +-2g, halving per range step.
static const float LSB_PER_G_2G = 1024.0f;
// TOUT is two's complement, 0 at 25 C.
static const float TEMP_PER_LSB = 0.586f;
static const float TEMP_ZERO = 25.0f;

void MXC4005Component::setup() {
  if (!this->read_byte(REG_DEVICE_ID, &this->device_id_)) {
    this->mark_failed();
    return;
  }
  // Power on (PD = 0) with the requested full scale range.
  if (!this->write_byte(REG_CONTROL, static_cast<uint8_t>(this->range_) << 5)) {
    this->mark_failed();
    return;
  }
}

void MXC4005Component::dump_config() {
  ESP_LOGCONFIG(TAG,
                "MXC4005:\n"
                "  Device ID: 0x%02X\n"
                "  Range: +-%ug",
                this->device_id_, 2u << this->range_);
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
  }
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Acceleration X", this->accel_x_sensor_);
  LOG_SENSOR("  ", "Acceleration Y", this->accel_y_sensor_);
  LOG_SENSOR("  ", "Acceleration Z", this->accel_z_sensor_);
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
}

void MXC4005Component::update() {
  uint8_t raw[7];
  if (!this->read_bytes(REG_XOUT_UPPER, raw, sizeof(raw))) {
    this->status_set_warning();
    return;
  }
  this->status_clear_warning();

  // Upper register holds bits 11:4, lower holds 3:0 in its top nibble, so the
  // pair is a left aligned 16 bit value and an arithmetic shift sign extends.
  const float scale = GRAVITY / (LSB_PER_G_2G / static_cast<float>(1u << this->range_));
  auto axis = [&](uint8_t i) { return static_cast<int16_t>((raw[i] << 8) | raw[i + 1]) >> 4; };

  if (this->accel_x_sensor_ != nullptr)
    this->accel_x_sensor_->publish_state(axis(0) * scale);
  if (this->accel_y_sensor_ != nullptr)
    this->accel_y_sensor_->publish_state(axis(2) * scale);
  if (this->accel_z_sensor_ != nullptr)
    this->accel_z_sensor_->publish_state(axis(4) * scale);
  if (this->temperature_sensor_ != nullptr)
    this->temperature_sensor_->publish_state(static_cast<int8_t>(raw[6]) * TEMP_PER_LSB + TEMP_ZERO);
}

}  // namespace esphome::mxc4005
