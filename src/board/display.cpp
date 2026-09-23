#include "board/display.h"
#include "board/pins.h"
#include "board/st7701/esp_lcd_st7701.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/ppa.h"
#include "esp_attr.h"
#include "esp_cache.h"
#include "esp_check.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_ldo_regulator.h"
#include "esp_log.h"

namespace board::display {

namespace {

const char* TAG = "display";

esp_lcd_panel_handle_t s_panel     = nullptr;
void*                  s_fb[2]     = { nullptr, nullptr };
int                    s_back      = 0;          // indeks bufora, do ktorego rysujemy
SemaphoreHandle_t      s_vsync_sem = nullptr;
ppa_client_handle_t    s_ppa       = nullptr;
uint32_t               s_vsync_timeouts = 0;

constexpr size_t FB_BYTES = (size_t)PANEL_W * PANEL_H * sizeof(uint16_t);

// Wywolywane z ISR DMA kontrolera DPI, gdy poprzedni bufor ramki moze byc bezpiecznie ponownie uzyty
// (czyli po przelaczeniu na nowy bufor na granicy ramki).
bool IRAM_ATTR on_frame_buf_complete(esp_lcd_panel_handle_t, esp_lcd_dpi_panel_event_data_t*, void*)
{
    BaseType_t hp = pdFALSE;
    xSemaphoreGiveFromISR(s_vsync_sem, &hp);
    return hp == pdTRUE;
}

// Programowa wersja skalowania + obrotu (awaryjnie, gdy PPA zawiedzie). Wolna, ale poprawna.
void rotate_scale_cpu(const uint16_t* src, int src_w, int src_h, int scale, uint16_t* dst)
{
    for (int py = 0; py < PANEL_H; ++py) {
        uint16_t* drow = dst + (size_t)py * PANEL_W;
        for (int px = 0; px < PANEL_W; ++px) {
            // (px, py) w panelu -> (lx, ly) w obrazie poziomym SCREEN_W x SCREEN_H
#if CONSOLE_DISPLAY_ROTATION == 90
            int lx = SCREEN_W - 1 - py;
            int ly = px;
#else
            int lx = py;
            int ly = SCREEN_H - 1 - px;
#endif
            drow[px] = src[(size_t)(ly / scale) * src_w + (lx / scale)];
        }
    }
}

}  // namespace

esp_err_t init()
{
    // 1. Zasilanie DSI-PHY z wewnetrznego LDO (kanal 3, 2.5 V) - musi byc przed utworzeniem magistrali.
    esp_ldo_channel_handle_t ldo = nullptr;
    esp_ldo_channel_config_t ldo_cfg = {};
    ldo_cfg.chan_id    = pins::MIPI_LDO_CHAN;
    ldo_cfg.voltage_mv = pins::MIPI_LDO_MV;
    ESP_RETURN_ON_ERROR(esp_ldo_acquire_channel(&ldo_cfg, &ldo), TAG, "LDO DSI-PHY");

    // 2. Podswietlenie wylaczone do czasu zainicjowania panelu.
    gpio_config_t bl = {};
    bl.pin_bit_mask = 1ULL << pins::LCD_BACKLIGHT;
    bl.mode         = GPIO_MODE_OUTPUT;
    ESP_RETURN_ON_ERROR(gpio_config(&bl), TAG, "GPIO backlight");
    gpio_set_level(pins::LCD_BACKLIGHT, 0);

    // 3. Magistrala MIPI-DSI: 2 linie danych, 500 Mbps.
    esp_lcd_dsi_bus_handle_t dsi_bus = nullptr;
    esp_lcd_dsi_bus_config_t bus_cfg = ST7701_PANEL_BUS_DSI_2CH_CONFIG();
    ESP_RETURN_ON_ERROR(esp_lcd_new_dsi_bus(&bus_cfg, &dsi_bus), TAG, "DSI bus");

    // 4. Kanal komend DBI (do wyslania sekwencji inicjalizacyjnej ST7701).
    esp_lcd_panel_io_handle_t io = nullptr;
    esp_lcd_dbi_io_config_t dbi_cfg = ST7701_PANEL_IO_DBI_CONFIG();
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_dbi(dsi_bus, &dbi_cfg, &io), TAG, "DBI io");

    // 5. Panel DPI 480x800 RGB565 @ 34 MHz (~60 Hz), DWA bufory ramki w PSRAM.
    esp_lcd_dpi_panel_config_t dpi_cfg = ST7701_480_360_PANEL_60HZ_DPI_CONFIG(LCD_COLOR_PIXEL_FORMAT_RGB565);
    dpi_cfg.num_fbs = 2;

    st7701_vendor_config_t vendor_cfg = {};
    vendor_cfg.mipi_config.dsi_bus    = dsi_bus;
    vendor_cfg.mipi_config.dpi_config = &dpi_cfg;
    vendor_cfg.flags.use_mipi_interface = 1;

    esp_lcd_panel_dev_config_t panel_cfg = {};
    panel_cfg.reset_gpio_num = pins::LCD_RESET;
    panel_cfg.rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_cfg.bits_per_pixel = 16;
    panel_cfg.vendor_config  = &vendor_cfg;

    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7701(io, &panel_cfg, &s_panel), TAG, "new panel");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel), TAG, "panel reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel), TAG, "panel init");

    // 6. Bufory ramki przydzielone przez sterownik DPI (PSRAM, wyrownane do DMA).
    ESP_RETURN_ON_ERROR(esp_lcd_dpi_panel_get_frame_buffer(s_panel, 2, &s_fb[0], &s_fb[1]), TAG, "get fbs");
    memset(s_fb[0], 0, FB_BYTES);
    memset(s_fb[1], 0, FB_BYTES);

    s_vsync_sem = xSemaphoreCreateBinary();
    ESP_RETURN_ON_FALSE(s_vsync_sem, ESP_ERR_NO_MEM, TAG, "semaphore");

    esp_lcd_dpi_panel_event_callbacks_t cbs = {};
    cbs.on_frame_buf_complete = on_frame_buf_complete;
    ESP_RETURN_ON_ERROR(esp_lcd_dpi_panel_register_event_callbacks(s_panel, &cbs, nullptr), TAG, "callbacks");

    // 7. PPA (Pixel Processing Accelerator) - klient operacji SRM (scale/rotate/mirror).
    ppa_client_config_t ppa_cfg = {};
    ppa_cfg.oper_type             = PPA_OPERATION_SRM;
    ppa_cfg.max_pending_trans_num = 1;
    esp_err_t ppa_err = ppa_register_client(&ppa_cfg, &s_ppa);
    if (ppa_err != ESP_OK) {
        ESP_LOGW(TAG, "PPA niedostepne (%s) - obrot programowy (wolny)", esp_err_to_name(ppa_err));
        s_ppa = nullptr;
    }

    // 8. Pierwsza (czarna) ramka jest juz w buforze 0 -> wlaczamy podswietlenie.
    s_back = 1;
    gpio_set_level(pins::LCD_BACKLIGHT, 1);
    ESP_LOGI(TAG, "ST7701 %dx%d gotowy, 2 bufory ramki, rotacja %d, PPA %s",
             PANEL_W, PANEL_H, CONSOLE_DISPLAY_ROTATION, s_ppa ? "on" : "off");
    return ESP_OK;
}

esp_err_t present(const uint16_t* src, int src_w, int src_h)
{
    ESP_RETURN_ON_FALSE(s_panel && src, ESP_ERR_INVALID_STATE, TAG, "not initialised");
    ESP_RETURN_ON_FALSE(src_w > 0 && src_h > 0 && SCREEN_W % src_w == 0 &&
                        SCREEN_W / src_w == SCREEN_H / src_h,
                        ESP_ERR_INVALID_ARG, TAG, "zly rozmiar klatki %dx%d", src_w, src_h);
    const int scale = SCREEN_W / src_w;
    uint16_t* dst = static_cast<uint16_t*>(s_fb[s_back]);

    // --- Skalowanie + obrot do tylnego bufora ---
    bool done = false;
    if (s_ppa) {
        ppa_srm_oper_config_t op = {};
        op.in.buffer         = src;
        op.in.pic_w          = src_w;
        op.in.pic_h          = src_h;
        op.in.block_w        = src_w;
        op.in.block_h        = src_h;
        op.in.block_offset_x = 0;
        op.in.block_offset_y = 0;
        op.in.srm_cm         = PPA_SRM_COLOR_MODE_RGB565;

        op.out.buffer         = dst;
        op.out.buffer_size    = FB_BYTES;
        op.out.pic_w          = PANEL_W;
        op.out.pic_h          = PANEL_H;
        op.out.block_offset_x = 0;
        op.out.block_offset_y = 0;
        op.out.srm_cm         = PPA_SRM_COLOR_MODE_RGB565;

#if CONSOLE_DISPLAY_ROTATION == 90
        op.rotation_angle = PPA_SRM_ROTATION_ANGLE_90;
#else
        op.rotation_angle = PPA_SRM_ROTATION_ANGLE_270;
#endif
        op.scale_x           = (float)scale;
        op.scale_y           = (float)scale;
        op.mirror_x          = false;
        op.mirror_y          = false;
        op.rgb_swap          = false;
        op.byte_swap         = false;
        op.alpha_update_mode = PPA_ALPHA_NO_CHANGE;
        op.mode              = PPA_TRANS_MODE_BLOCKING;

        esp_err_t err = ppa_do_scale_rotate_mirror(s_ppa, &op);
        if (err == ESP_OK) {
            done = true;
        } else {
            static uint32_t warned = 0;
            if (warned++ < 5) {
                ESP_LOGW(TAG, "PPA SRM: %s - fallback CPU", esp_err_to_name(err));
            }
        }
    }
    if (!done) {
        rotate_scale_cpu(src, src_w, src_h, scale, dst);
        // Bufor DPI czyta DMA - dane zapisane przez CPU musza zostac wypchniete z cache do pamieci.
        esp_cache_msync(dst, FB_BYTES, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    }

    // --- Przelaczenie buforow: draw_bitmap z adresem wlasnego bufora sterownika = zamiana bez kopiowania ---
    xSemaphoreTake(s_vsync_sem, 0);  // wyczysc zalegly sygnal z poprzedniej ramki
    ESP_RETURN_ON_ERROR(esp_lcd_panel_draw_bitmap(s_panel, 0, 0, PANEL_W, PANEL_H, dst), TAG, "draw_bitmap");

    // Czekamy, az kontroler zacznie wysylac nowa ramke (koniec biezacej = VSYNC).
    if (xSemaphoreTake(s_vsync_sem, pdMS_TO_TICKS(50)) != pdTRUE) {
        ++s_vsync_timeouts;
    }
    s_back ^= 1;
    return ESP_OK;
}

void set_backlight(bool on)
{
    gpio_set_level(pins::LCD_BACKLIGHT, on ? 1 : 0);
}

uint32_t vsync_timeouts()
{
    return s_vsync_timeouts;
}

}  // namespace board::display
