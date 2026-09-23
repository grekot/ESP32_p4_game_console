#include "board/touch.h"
#include "board/display.h"
#include "board/pins.h"

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_log.h"

namespace board::touch {

namespace {

const char* TAG = "touch";
esp_lcd_touch_handle_t s_tp = nullptr;

// (px, py) - natywne wspolrzedne panelu (pion 480x800) -> logiczny ekran gry (poziom 800x480).
// Odwrotnosc obrotu wykonywanego w display::present().
inline void map_point(uint16_t px, uint16_t py, int16_t& x, int16_t& y)
{
#if CONSOLE_DISPLAY_ROTATION == 90
    x = (int16_t)(display::SCREEN_W - 1 - py);
    y = (int16_t)px;
#else
    x = (int16_t)py;
    y = (int16_t)(display::SCREEN_H - 1 - px);
#endif
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= display::SCREEN_W) x = display::SCREEN_W - 1;
    if (y >= display::SCREEN_H) y = display::SCREEN_H - 1;
}

esp_err_t new_gt911(i2c_master_bus_handle_t bus, uint16_t addr, esp_lcd_touch_handle_t* out)
{
    esp_lcd_panel_io_i2c_config_t io_cfg = {};
    io_cfg.dev_addr            = addr;
    io_cfg.control_phase_bytes = 1;
    io_cfg.dc_bit_offset       = 0;
    io_cfg.lcd_cmd_bits        = 16;
    io_cfg.lcd_param_bits      = 8;
    io_cfg.flags.disable_control_phase = 1;
    io_cfg.scl_speed_hz        = 400000;

    esp_lcd_panel_io_handle_t io = nullptr;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c_v2(bus, &io_cfg, &io), TAG, "panel io i2c");

    esp_lcd_touch_config_t tp_cfg = {};
    tp_cfg.x_max        = display::PANEL_W;
    tp_cfg.y_max        = display::PANEL_H;
    tp_cfg.rst_gpio_num = pins::TOUCH_RST;
    tp_cfg.int_gpio_num = pins::TOUCH_INT;
    tp_cfg.levels.reset     = 0;
    tp_cfg.levels.interrupt = 0;
    tp_cfg.flags.swap_xy  = false;
    tp_cfg.flags.mirror_x = false;
    tp_cfg.flags.mirror_y = false;

    esp_err_t err = esp_lcd_touch_new_i2c_gt911(io, &tp_cfg, out);
    if (err != ESP_OK) {
        esp_lcd_panel_io_del(io);
    }
    return err;
}

}  // namespace

esp_err_t init()
{
    i2c_master_bus_handle_t bus = nullptr;
    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.i2c_port          = I2C_NUM_0;
    bus_cfg.sda_io_num        = pins::TOUCH_SDA;
    bus_cfg.scl_io_num        = pins::TOUCH_SCL;
    bus_cfg.clk_source        = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &bus), TAG, "i2c bus");

    // GT911 zglasza sie pod 0x5D albo (zaleznie od stanu pinu INT przy resecie) pod 0x14.
    esp_err_t err = new_gt911(bus, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS, &s_tp);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "GT911 @0x%02X: %s, probuje 0x%02X", ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,
                 esp_err_to_name(err), ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP);
        err = new_gt911(bus, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP, &s_tp);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GT911 nie odpowiada: %s", esp_err_to_name(err));
        s_tp = nullptr;
        return err;
    }
    ESP_LOGI(TAG, "GT911 gotowy (max %d punktow)", MAX_POINTS);
    return ESP_OK;
}

int read(Point* out, int max_points)
{
    if (!s_tp || !out || max_points <= 0) {
        return 0;
    }
    if (esp_lcd_touch_read_data(s_tp) != ESP_OK) {
        return 0;
    }
    esp_lcd_touch_point_data_t pts[MAX_POINTS] = {};
    uint8_t n = 0;
    if (esp_lcd_touch_get_data(s_tp, pts, &n, (uint8_t)MAX_POINTS) != ESP_OK) {
        return 0;
    }
    int count = 0;
    for (int i = 0; i < n && count < max_points; ++i) {
        map_point(pts[i].x, pts[i].y, out[count].x, out[count].y);
        ++count;
    }
    return count;
}

bool available()
{
    return s_tp != nullptr;
}

}  // namespace board::touch
