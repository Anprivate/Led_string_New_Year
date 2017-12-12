
#include <Adafruit_NeoPixel.h>

// Пин на который цепляются светодиоды
#define LED_PIN 2

// всего пикселей
#define total_pixels 50

// Длительность кадра
#define frame_period 10

// длительность одного эффекта
#define time_per_effect 10000

#define total_cycles (time_per_effect / frame_period)

Adafruit_NeoPixel strip = Adafruit_NeoPixel(total_pixels, LED_PIN, NEO_GRB + NEO_KHZ800);

// яркость
uint8_t cur_br = 50;

void setup() {
  strip.begin();
  strip.show();
  SetBrightness();
}

void loop() {
  static uint8_t prev_mode = 0;

  uint8_t curr_mode;
  do {
    curr_mode = random(3);
  } while (curr_mode == prev_mode);
  prev_mode = curr_mode;

  switch (curr_mode)
  {
    case 0: {
        StarSky();
        break;
      }
    case 1: {
        Comet(false);
        break;
      }
    case 2: {
        Comet(true);
        break;
      }
  }
  delay(300);
}

// ================================================================
// эффекты
// ================================================================

void StarSky(void) {
  // число звёзд
#define star_number (total_pixels * 2 / 3)
  // разгорание за полсекунды
#define speed_cntr (500 / frame_period)

  uint8_t predelay[star_number];
  uint32_t color[star_number];
  uint8_t pixel_num[star_number];

  for (uint8_t i = 0; i < star_number; i++) {
    // первичная задержка до 1 секунды
    predelay[i] = random(100);
    color[i] = 0;
    pixel_num[i] = 0xFF;
  }

  delay_sp(0);
  for (uint16_t time_counter = 0; time_counter < total_cycles; time_counter++) {
    SetBrightness();
    clear_all();

    for (uint8_t curr_star = 0; curr_star < star_number; curr_star++) {
      if (color[curr_star] == 0) { // predelay stage
        if (predelay[curr_star])
          predelay[curr_star]--;
        else
          // predelay закончился, задаём любой цвет
          color[curr_star] = 1;
      } else {
        if (predelay[curr_star] == 0) { // начальная фаза, генерим параметры пикселя
          // ищем свободный пиксель
          uint8_t tmp_num;
          uint8_t busy;
          do {
            tmp_num = random(total_pixels);
            busy = 0;
            for (uint8_t i = 0; i < star_number; i++) {
              if (pixel_num[i] == tmp_num) {
                busy = 1;
                break;
              }
            }
          } while (busy);
          pixel_num[curr_star] = tmp_num;
          color[curr_star] = mask_to_color(random(6) + 1);
          predelay[curr_star]++;
        } else {
          // рабочая фаза, старший бит обозначает направление
          if (predelay[curr_star] & 0x80) { // fade down
            strip.setPixelColor(pixel_num[curr_star], mult_div_color(color[curr_star], predelay[curr_star] & 0x7F, speed_cntr));
            predelay[curr_star]--;
            if ((predelay[curr_star] & 0x7F) == 0) predelay[curr_star] = 0;
          } else { // fade up
            strip.setPixelColor(pixel_num[curr_star], mult_div_color(color[curr_star], predelay[curr_star], speed_cntr));
            predelay[curr_star]++;
            if (predelay[curr_star] >= speed_cntr) predelay[curr_star] |= 0x80;
          }
        }
      }
    }
    strip.show();
    delay_sp(frame_period);
  }
  // финал - плавненько гасим все пиксели
  uint8_t some_action;
  do {
    SetBrightness();
    clear_all();
    some_action = 0;
    for (uint8_t curr_star = 0; curr_star < star_number; curr_star++) {
      predelay[curr_star] &= 0x7F;
      strip.setPixelColor(pixel_num[curr_star], mult_div_color(color[curr_star], predelay[curr_star], speed_cntr));
      if (predelay[curr_star]) {
        predelay[curr_star]--;
        some_action = 1;
      }
    }
    strip.show();
    delay_sp(frame_period);
  } while (some_action);
}

void Comet(boolean direction_is_up)
{
  // длина кометы
#define comet_length  5
  // интервал кометы
#define comet_interval  15
  // oversample
#define comet_oversample  5

#define comet_qty (total_pixels / comet_interval + 1)

  uint8_t comet_mask[comet_qty];
  int16_t comet_start[comet_qty];

  delay_sp(0);
  for (uint8_t comet_num = 0; comet_num < comet_qty; comet_num++)
  {
    comet_mask[comet_num] = random(6) + 1;
    comet_start[comet_num] = ((int16_t)comet_num - comet_qty + 1) * comet_interval * comet_oversample;
  }

  uint16_t time_counter = 0;
  while (true)
  {
    clear_all();
    boolean was_indication = false;
    for (uint8_t comet_num = 0; comet_num < comet_qty; comet_num++)
    {
      if (comet_start[comet_num] >= 0) {
        uint16_t pix_pos = comet_start[comet_num] / comet_oversample;
        uint16_t pix_sub = comet_start[comet_num] % comet_oversample;
        uint32_t cur_color = mask_to_color(comet_mask[comet_num]);
        if (pixel_num_is_valid(pix_pos))
          if (direction_is_up)
            strip.setPixelColor(pix_pos, mult_div_color(cur_color, pix_sub, comet_oversample));
          else
            strip.setPixelColor(total_pixels - 1 - pix_pos, mult_div_color(cur_color, pix_sub, comet_oversample));
        for (uint8_t i = 0; i < comet_length; i++)
        {
          pix_pos--;
          if (!pixel_num_is_valid(pix_pos)) break;
          if (direction_is_up)
            strip.setPixelColor(pix_pos, mult_div_color(cur_color, (comet_length - i) * comet_oversample - pix_sub, comet_length * comet_oversample));
          else
            strip.setPixelColor(total_pixels - 1 - pix_pos, mult_div_color(cur_color, (comet_length - i) * comet_oversample - pix_sub, comet_length * comet_oversample));
          was_indication = true;
        }
      }
      comet_start[comet_num]++;
      if (comet_start[comet_num] == 0) comet_mask[comet_num] = random(6) + 1;
    }
    if (time_counter < total_cycles) {
      if ((comet_start[comet_qty - 1] / comet_oversample) > (total_pixels + comet_length))
      {
        for (uint8_t i = comet_qty - 1; i > 0; i--)
        {
          comet_start[i] = comet_start[i - 1];
          comet_mask[i] = comet_mask[i - 1];
        }
        comet_start[0] -= (comet_interval * comet_oversample);
      }
    }

    strip.show();
    delay_sp(frame_period);

    time_counter++;
    if ((time_counter > total_cycles) && !was_indication) break;
  }
}

// ================================================================
// вспомогательные функции
// ================================================================

// задержка на заданное число миллисекунд от последнего вызова (0 - выйти сразу, но зафиксировать вызов)
void delay_sp(uint16_t value) {
  static uint32_t last_call_time;
  if (value) {
    uint32_t between = millis() - last_call_time;
    if ((between < 10000) && (between < value)) delay(value - between);
  }
  last_call_time = millis();
}

// очистка всего
void clear_all(void) {
  for (uint16_t i = 0; i < total_pixels; i++) {
    strip.setPixelColor(i, 0x00000000);
  }
}

// маска цветов в цвет
uint32_t mask_to_color(uint8_t mask) {
  uint32_t tmp_color = 0;
  uint32_t maskbit = 0x000000FF;
  if (mask & 0x01) tmp_color += maskbit;
  maskbit <<= 8;
  if (mask & 0x02) tmp_color += maskbit;
  (maskbit <<= 8);
  if (mask & 0x04) tmp_color += maskbit;
  return tmp_color;
}

// берём RGB цвета, умножаем на mult и делим на divider
uint32_t mult_div_color(uint32_t in_color, uint16_t mult, uint16_t divider) {
  uint32_t rn = ((in_color >> 16) & 0xFF) * mult / divider;
  uint32_t gn = ((in_color >> 8) & 0xFF) * mult / divider;
  uint32_t bn = (in_color & 0xFF) * mult / divider;
  return (rn << 16) | (gn << 8) | bn;
}

// задать яркость
// сюда можно запихнуть чтение значения яркости от потенциометра по любому аналоговому входу
void SetBrightness(void) {
  strip.setBrightness(cur_br);
}

// номер пикселя допустим?
inline boolean pixel_num_is_valid(int16_t pixel_num) {
  return (pixel_num >= 0) && (pixel_num < total_pixels);
}

