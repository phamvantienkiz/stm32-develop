/**
 * ============================================================================
 *  lcd-pixel-bot — Reference Implementation for STM32F429I-DISC1
 * ============================================================================
 *
 *  Board   : STM32F429I-DISC1 (Discovery kit with STM32F429ZI MCU)
 *  MCU     : ARM Cortex-M4 STM32F429ZIT6U, 180 MHz
 *  Display : 2.4" TFT QVGA 240x320, RGB565 via LTDC & External SDRAM
 *  Design  : See output/lcd-pixel-bot-cubemx-config.md
 *            and pixel_bot_animation_design.md
 *
 *  HOW TO USE THIS FILE
 *  --------------------
 *  1. Configure STM32CubeMX according to output/lcd-pixel-bot-cubemx-config.md.
 *  2. Follow Section 5b of the config document to copy ST BSP & HAL drivers.
 *  3. Copy each USER CODE block below into the corresponding section of your
 *     generated Core/Src/main.c.
 *
 *  FEATURES IMPLEMENTED
 *  --------------------
 *  - Full 16x10 cell Pixel Bot with 12x12 px cells (192x120 px sprite).
 *  - Modular layer rendering: Background -> Shadow -> Legs -> Body -> Arms
 *    -> Blush -> Eyes -> Mouth -> FX.
 *  - Double-buffering in external SDRAM with hardware VSYNC shadow reload.
 *  - 12 Rich Animations: BOOT, IDLE, WAVE, JUMP, WALK, DANCE, LAUGH, LOVE,
 *    THINKING, SURPRISED, CONFUSED, SLEEP.
 *  - Natural overlays: Random eye blinking (2.5-5.5s) & looking around.
 *  - Autonomous Random Action Loop: Bot continuously performs random actions
 *    consecutively with natural idle pauses.
 *  - Manual Interactive Trigger: Pressing User Button PA0 triggers excitement.
 *  - Serial Command Shell: Control bot via USART1 (115200 bps).
 * ============================================================================
 */

/* USER CODE BEGIN Header */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "stm32f429i_discovery.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_sdram.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* 1-bit Bitmap descriptor */
typedef struct {
    uint8_t w;
    uint8_t h;
    const uint8_t *rows;
} Bmp;

/* Eye IDs */
typedef enum {
    EYE_NORMAL = 0,
    EYE_HALF,
    EYE_CLOSED,
    EYE_HAPPY,
    EYE_SQUINT,
    EYE_WIDE,
    EYE_HEART,
    EYE_SPARKLE,
    EYE_ANGRY,
    EYE_SAD,
    EYE_DIZZY0,
    EYE_DIZZY1,
    EYE_DIZZY2,
    EYE_DIZZY3,
    EYE_COUNT
} EyeId;

/* Mouth IDs */
typedef enum {
    MOUTH_NONE = 0,
    MOUTH_SMILE,
    MOUTH_OPEN,
    MOUTH_O,
    MOUTH_FLAT,
    MOUTH_FROWN,
    MOUTH_COUNT
} MouthId;

/* FX IDs */
typedef enum {
    FX_NONE = 0,
    FX_HEART,
    FX_QUESTION,
    FX_EXCL,
    FX_Z,
    FX_SPARK,
    FX_WAVE,
    FX_ANGER,
    FX_TEARS,
    FX_COUNT
} FxId;

/* Character Pose structure (one snapshot in time) */
typedef struct {
    int8_t   dx, dy;       /* Offset toàn thân (ô) */
    int8_t   stretch;      /* -1: squash, 0: normal, +1: stretch */
    int8_t   aL_in, aL_dy; /* Tay trái (inward, dy) */
    int8_t   aR_in, aR_dy; /* Tay phải (inward, dy) */
    uint8_t  legs;         /* Mã hóa 4 chân: LEGS(LL,LI,RI,RO) */
    uint8_t  eyeL, eyeR;   /* EyeId */
    int8_t   eyeDx, eyeDy; /* Hướng nhìn quanh (-1, 0, +1) */
    uint8_t  eyeColor;     /* 0: tối (C_EYE), 1: sáng (C_CYAN) */
    uint8_t  mouth;        /* MouthId */
    uint8_t  blush;        /* 0: tắt má hồng, 1: bật */
    uint8_t  fx;           /* FxId */
    uint16_t ms;           /* Thời lượng pose (ms) */
} Pose;

/* Animation descriptor */
typedef struct {
    const char *name;
    const Pose *frames;
    uint8_t     num_frames;
    uint8_t     loop;
    uint8_t     priority;  /* Mức ưu tiên: 0=idle, 1=emotion, 2=action, 3=react, 4=system */
} Anim;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SCR_W        240
#define SCR_H        320
#define CELL         12
#define COLS         (SCR_W / CELL)  /* 20 cột */
#define ROWS         (SCR_H / CELL)  /* 26 hàng */
#define CHAR_X       2
#define CHAR_Y       10
#define GROUND_ROW   (CHAR_Y + 10)   /* Hàng 20: Đổ bóng */

/* Bộ nhớ Framebuffer trong SDRAM (IS42S16400J Bank 2) */
#define FB_SIZE      (SCR_W * SCR_H * 2) /* 153,600 bytes = 0x25800 */
#define FB0_ADDR     0xD0000000UL
#define FB1_ADDR     (FB0_ADDR + FB_SIZE)

/* Bảng màu RGB565 (Theme Tím / Xanh Công nghệ) */
#define C_BG         0x0843  /* #0D0B1E Nền tím đêm */
#define C_SHADOW     0x18A7  /* #1A1538 Bóng đổ */
#define C_BODY       0x7B1F  /* #7B61FF Thân tím chính */
#define C_BODY_HI    0x9C7F  /* #9D8CFF Hàng trên cùng (Highlight) */
#define C_BODY_SH    0x5A3A  /* #5B45D6 Hàng dưới & bóng tay */
#define C_EYE        0x1045  /* #120B2E Mắt tối */
#define C_CYAN       0x269D  /* #22D3EE Mắt phát sáng / Zzz / Sóng âm */
#define C_PINK       0xFB5A  /* #FF6BD6 Má hồng */
#define C_HEART      0xFA71  /* #FF4D8D Trái tim */
#define C_YELLOW     0xFEC7  /* #FFD93D Sao, dấu ?, ! */
#define C_TEAR       0x5D5F  /* #5AA9FF Nước mắt */
#define C_RED        0xF9C7  /* #FF3B3B Dấu giận */
#define C_WHITE      0xFFFF  /* Trắng */

/* Macro mã hóa chân: mỗi chân 2 bit (chiều cao 0..2 ô) */
#define LEGS(ll, li, ri, ro) ((uint8_t)((ll) | ((li) << 2) | ((ri) << 4) | ((ro) << 6)))
#define LEG_H(legs, idx)     (((legs) >> (2 * (idx))) & 3)

#define BASE_POSE \
    .dx = 0, .dy = 0, .stretch = 0, \
    .aL_in = 0, .aL_dy = 0, .aR_in = 0, .aR_dy = 0, \
    .legs = LEGS(2, 2, 2, 2), \
    .eyeL = EYE_NORMAL, .eyeR = EYE_NORMAL, \
    .eyeDx = 0, .eyeDy = 0, .eyeColor = 0, \
    .mouth = MOUTH_NONE, .blush = 0, .fx = FX_NONE

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
/* Double-buffering pointers */
static uint16_t *front = (uint16_t *)FB0_ADDR;
static uint16_t *back  = (uint16_t *)FB1_ADDR;

/* Cột đặt 4 chân (tọa độ local): LL=3, LI=5, RI=10, RO=12 */
static const uint8_t LEG_COL[4] = {3, 5, 10, 12};

/* Cờ ngắt và giao tiếp */
volatile uint8_t flag_btn = 0;
static uint8_t rx_byte = 0;

/* Bộ sinh số ngẫu nhiên nhẹ (LCG) */
static uint32_t prng_state = 0x12345678;

/* ---------------------------------------------------------------------------
 * BITMAP TABLES (1 bit per pixel, MSB = leftmost pixel)
 * --------------------------------------------------------------------------- */
/* Mắt (3 cột x 4 hàng, vẽ cho mắt trái; mắt phải tự lật ngang) */
static const uint8_t EYE_NORMAL_ROWS[4]  = {0b000, 0b010, 0b010, 0b000};
static const uint8_t EYE_HALF_ROWS[4]    = {0b000, 0b000, 0b010, 0b000};
static const uint8_t EYE_CLOSED_ROWS[4]  = {0b000, 0b000, 0b111, 0b000};
static const uint8_t EYE_HAPPY_ROWS[4]   = {0b010, 0b101, 0b000, 0b000};
static const uint8_t EYE_SQUINT_ROWS[4]  = {0b000, 0b100, 0b010, 0b100}; /* '>' */
static const uint8_t EYE_WIDE_ROWS[4]    = {0b010, 0b010, 0b010, 0b010};
static const uint8_t EYE_HEART_ROWS[4]   = {0b000, 0b101, 0b111, 0b010};
static const uint8_t EYE_SPARKLE_ROWS[4] = {0b000, 0b010, 0b111, 0b010};
static const uint8_t EYE_ANGRY_ROWS[4]   = {0b110, 0b010, 0b010, 0b000};
static const uint8_t EYE_SAD_ROWS[4]     = {0b001, 0b010, 0b010, 0b000};
static const uint8_t EYE_DIZZY0_ROWS[4]  = {0b000, 0b101, 0b101, 0b111};
static const uint8_t EYE_DIZZY1_ROWS[4]  = {0b000, 0b111, 0b100, 0b111};
static const uint8_t EYE_DIZZY2_ROWS[4]  = {0b000, 0b111, 0b101, 0b101};
static const uint8_t EYE_DIZZY3_ROWS[4]  = {0b000, 0b111, 0b001, 0b111};

static const Bmp EYE_BMPS[EYE_COUNT] = {
    {3, 4, EYE_NORMAL_ROWS},  {3, 4, EYE_HALF_ROWS},
    {3, 4, EYE_CLOSED_ROWS},  {3, 4, EYE_HAPPY_ROWS},
    {3, 4, EYE_SQUINT_ROWS},  {3, 4, EYE_WIDE_ROWS},
    {3, 4, EYE_HEART_ROWS},   {3, 4, EYE_SPARKLE_ROWS},
    {3, 4, EYE_ANGRY_ROWS},   {3, 4, EYE_SAD_ROWS},
    {3, 4, EYE_DIZZY0_ROWS},  {3, 4, EYE_DIZZY1_ROWS},
    {3, 4, EYE_DIZZY2_ROWS},  {3, 4, EYE_DIZZY3_ROWS},
};

/* Miệng (4 cột x 3 hàng) */
static const uint8_t MOUTH_SMILE_ROWS[3] = {0b1001, 0b0110, 0b0000};
static const uint8_t MOUTH_OPEN_ROWS[3]  = {0b1111, 0b1001, 0b0110};
static const uint8_t MOUTH_O_ROWS[3]     = {0b0000, 0b0110, 0b0110};
static const uint8_t MOUTH_FLAT_ROWS[3]  = {0b0000, 0b0110, 0b0000};
static const uint8_t MOUTH_FROWN_ROWS[3] = {0b0000, 0b0110, 0b1001};

static const Bmp MOUTH_BMPS[MOUTH_COUNT] = {
    {0, 0, NULL},
    {4, 3, MOUTH_SMILE_ROWS},
    {4, 3, MOUTH_OPEN_ROWS},
    {4, 3, MOUTH_O_ROWS},
    {4, 3, MOUTH_FLAT_ROWS},
    {4, 3, MOUTH_FROWN_ROWS}
};

/* FX Bitmaps */
static const uint8_t FX_HEART_ROWS[5] = {0b01010, 0b11111, 0b11111, 0b01110, 0b00100};
static const uint8_t FX_QUEST_ROWS[5] = {0b111, 0b001, 0b010, 0b000, 0b010};
static const uint8_t FX_EXCL_ROWS[5]  = {0b1, 0b1, 0b1, 0b0, 0b1};
static const uint8_t FX_Z_ROWS[3]     = {0b111, 0b010, 0b111};
static const uint8_t FX_SPARK_ROWS[3] = {0b010, 0b111, 0b010};
static const uint8_t FX_WAVE_ROWS[5]  = {0b10, 0b01, 0b01, 0b01, 0b10};
static const uint8_t FX_ANGER_ROWS[4] = {0b1001, 0b0110, 0b0110, 0b1001};

static const Bmp FX_HEART_BMP = {5, 5, FX_HEART_ROWS};
static const Bmp FX_QUEST_BMP = {3, 5, FX_QUEST_ROWS};
static const Bmp FX_EXCL_BMP  = {1, 5, FX_EXCL_ROWS};
static const Bmp FX_Z_BMP     = {3, 3, FX_Z_ROWS};
static const Bmp FX_SPARK_BMP = {3, 3, FX_SPARK_ROWS};
static const Bmp FX_WAVE_BMP  = {2, 5, FX_WAVE_ROWS};
static const Bmp FX_ANGER_BMP = {4, 4, FX_ANGER_ROWS};

/* ---------------------------------------------------------------------------
 * ANIMATION DEFINITIONS
 * --------------------------------------------------------------------------- */

/* 1. BOOT (Khởi động: ngủ -> hé mắt -> vươn vai -> vẫy tay) */
static const Pose BOOT_FRAMES[] = {
    { BASE_POSE, .stretch = -1, .aL_dy = 2, .aR_dy = 2, .eyeL = EYE_CLOSED, .eyeR = EYE_CLOSED, .fx = FX_Z, .ms = 600 },
    { BASE_POSE, .eyeL = EYE_HALF, .eyeR = EYE_HALF, .ms = 250 },
    { BASE_POSE, .eyeL = EYE_NORMAL, .eyeR = EYE_NORMAL, .eyeDx = -1, .ms = 300 },
    { BASE_POSE, .eyeL = EYE_NORMAL, .eyeR = EYE_NORMAL, .eyeDx = +1, .ms = 300 },
    { BASE_POSE, .stretch = +1, .aL_dy = -2, .aR_dy = -2, .eyeL = EYE_CLOSED, .eyeR = EYE_CLOSED, .mouth = MOUTH_O, .ms = 400 },
    { BASE_POSE, .eyeL = EYE_HAPPY, .eyeR = EYE_HAPPY, .mouth = MOUTH_SMILE, .blush = 1, .aR_in = -1, .aR_dy = -3, .ms = 500 }
};
static const Anim ANIM_BOOT = { "BOOT", BOOT_FRAMES, 6, 0, 4 };

/* 2. IDLE (Thở nhẹ nhàng chu kỳ 1.8s) */
static const Pose IDLE_FRAMES[] = {
    { BASE_POSE, .aL_dy = 0,  .aR_dy = 0,  .ms = 900 },
    { BASE_POSE, .aL_dy = +1, .aR_dy = +1, .ms = 900 }
};
static const Anim ANIM_IDLE = { "IDLE", IDLE_FRAMES, 2, 1, 0 };

/* 3. WAVE (Vẫy tay chào vui vẻ) */
static const Pose WAVE_FRAMES[] = {
    { BASE_POSE, .eyeL = EYE_HAPPY, .eyeR = EYE_HAPPY, .mouth = MOUTH_SMILE, .blush = 1, .aR_in = 0,  .aR_dy = -1, .ms = 120 },
    { BASE_POSE, .eyeL = EYE_HAPPY, .eyeR = EYE_HAPPY, .mouth = MOUTH_SMILE, .blush = 1, .aR_in = 0,  .aR_dy = -2, .ms = 140 },
    { BASE_POSE, .eyeL = EYE_HAPPY, .eyeR = EYE_HAPPY, .mouth = MOUTH_SMILE, .blush = 1, .aR_in = -1, .aR_dy = -3, .ms = 140 },
    { BASE_POSE, .eyeL = EYE_HAPPY, .eyeR = EYE_HAPPY, .mouth = MOUTH_SMILE, .blush = 1, .aR_in = 0,  .aR_dy = -2, .ms = 140 },
    { BASE_POSE, .eyeL = EYE_HAPPY, .eyeR = EYE_HAPPY, .mouth = MOUTH_SMILE, .blush = 1, .aR_in = -1, .aR_dy = -3, .ms = 140 },
    { BASE_POSE, .eyeL = EYE_HAPPY, .eyeR = EYE_HAPPY, .mouth = MOUTH_SMILE, .blush = 1, .aR_in = 0,  .aR_dy = -1, .ms = 140 }
};
static const Anim ANIM_WAVE = { "WAVE", WAVE_FRAMES, 6, 0, 2 };

/* 4. JUMP (Dồn lực -> Bật cao -> Đỉnh -> Đáp nảy đàn hồi) */
static const Pose JUMP_FRAMES[] = {
    { BASE_POSE, .dy = 0,  .stretch = -1, .aL_dy = +1, .aR_dy = +1, .eyeL = EYE_NORMAL,  .eyeR = EYE_NORMAL,  .mouth = MOUTH_NONE, .ms = 120 },
    { BASE_POSE, .dy = -2, .stretch = +1, .aL_dy = -2, .aR_dy = -2, .legs = LEGS(1,1,1,1), .eyeL = EYE_WIDE, .eyeR = EYE_WIDE, .mouth = MOUTH_O, .ms = 80 },
    { BASE_POSE, .dy = -4, .stretch = 0,  .aL_dy = -2, .aR_dy = -2, .legs = LEGS(1,1,1,1), .eyeL = EYE_HAPPY, .eyeR = EYE_HAPPY, .mouth = MOUTH_OPEN, .ms = 100 },
    { BASE_POSE, .dy = -5, .stretch = 0,  .aL_dy = -2, .aR_dy = -2, .legs = LEGS(1,1,1,1), .eyeL = EYE_HAPPY, .eyeR = EYE_HAPPY, .mouth = MOUTH_OPEN, .ms = 140 },
    { BASE_POSE, .dy = -2, .stretch = +1, .aL_dy = -1, .aR_dy = -1, .legs = LEGS(2,2,2,2), .eyeL = EYE_NORMAL, .eyeR = EYE_NORMAL, .mouth = MOUTH_O, .ms = 80 },
    { BASE_POSE, .dy = 0,  .stretch = -1, .aL_dy = +1, .aR_dy = +1, .legs = LEGS(2,2,2,2), .eyeL = EYE_CLOSED, .eyeR = EYE_CLOSED, .mouth = MOUTH_NONE, .ms = 100 },
    { BASE_POSE, .dy = 0,  .stretch = 0,  .aL_dy = 0,  .aR_dy = 0,  .legs = LEGS(2,2,2,2), .eyeL = EYE_HAPPY,  .eyeR = EYE_HAPPY,  .mouth = MOUTH_SMILE, .blush = 1, .ms = 160 }
};
static const Anim ANIM_JUMP = { "JUMP", JUMP_FRAMES, 7, 0, 2 };

/* 5. WALK (Đi bộ tại chỗ nhịp nhàng) */
static const Pose WALK_FRAMES[] = {
    { BASE_POSE, .legs = LEGS(1, 2, 1, 2), .aL_dy = +1, .aR_dy = -1, .eyeDx = +1, .ms = 120 },
    { BASE_POSE, .dy = -1, .legs = LEGS(2, 2, 2, 2), .aL_dy = 0,  .aR_dy = 0,  .eyeDx = +1, .ms = 120 },
    { BASE_POSE, .legs = LEGS(2, 1, 2, 1), .aL_dy = -1, .aR_dy = +1, .eyeDx = +1, .ms = 120 },
    { BASE_POSE, .dy = -1, .legs = LEGS(2, 2, 2, 2), .aL_dy = 0,  .aR_dy = 0,  .eyeDx = +1, .ms = 120 },
    { BASE_POSE, .legs = LEGS(1, 2, 1, 2), .aL_dy = +1, .aR_dy = -1, .eyeDx = +1, .ms = 120 },
    { BASE_POSE, .dy = -1, .legs = LEGS(2, 2, 2, 2), .aL_dy = 0,  .aR_dy = 0,  .eyeDx = +1, .ms = 120 },
    { BASE_POSE, .legs = LEGS(2, 1, 2, 1), .aL_dy = -1, .aR_dy = +1, .eyeDx = +1, .ms = 120 },
    { BASE_POSE, .dy = 0,  .legs = LEGS(2, 2, 2, 2), .aL_dy = 0,  .aR_dy = 0,  .eyeDx = 0,  .ms = 120 }
};
static const Anim ANIM_WALK = { "WALK", WALK_FRAMES, 8, 0, 2 };

/* 6. DANCE (Nhảy múa lắc lư vui nhộn) */
static const Pose DANCE_FRAMES[] = {
    { BASE_POSE, .dx = -1, .stretch = 0,  .aL_dy = -2, .aR_dy = +1, .legs = LEGS(1,2,2,2), .eyeL = EYE_HAPPY, .eyeR = EYE_HAPPY, .mouth = MOUTH_OPEN, .blush = 1, .fx = FX_SPARK, .ms = 220 },
    { BASE_POSE, .dx = 0,  .dy = -1, .stretch = +1, .aL_dy = -2, .aR_dy = -2, .legs = LEGS(1,1,1,1), .eyeL = EYE_SPARKLE, .eyeR = EYE_SPARKLE, .mouth = MOUTH_OPEN, .blush = 1, .ms = 220 },
    { BASE_POSE, .dx = +1, .stretch = 0,  .aL_dy = +1, .aR_dy = -2, .legs = LEGS(2,2,2,1), .eyeL = EYE_HAPPY, .eyeR = EYE_HAPPY, .mouth = MOUTH_OPEN, .blush = 1, .fx = FX_SPARK, .ms = 220 },
    { BASE_POSE, .dx = 0,  .dy = -1, .stretch = +1, .aL_dy = -2, .aR_dy = -2, .legs = LEGS(1,1,1,1), .eyeL = EYE_SPARKLE, .eyeR = EYE_SPARKLE, .mouth = MOUTH_OPEN, .blush = 1, .ms = 220 },
    { BASE_POSE, .dx = -1, .stretch = 0,  .aL_dy = -2, .aR_dy = +1, .legs = LEGS(1,2,2,2), .eyeL = EYE_HAPPY, .eyeR = EYE_HAPPY, .mouth = MOUTH_OPEN, .blush = 1, .fx = FX_SPARK, .ms = 220 },
    { BASE_POSE, .dx = +1, .stretch = 0,  .aL_dy = +1, .aR_dy = -2, .legs = LEGS(2,2,2,1), .eyeL = EYE_HAPPY, .eyeR = EYE_HAPPY, .mouth = MOUTH_OPEN, .blush = 1, .fx = FX_SPARK, .ms = 220 }
};
static const Anim ANIM_DANCE = { "DANCE", DANCE_FRAMES, 6, 0, 2 };

/* 7. LAUGH (Cười tít mắt nhảy nhót) */
static const Pose LAUGH_FRAMES[] = {
    { BASE_POSE, .dy = -1, .aL_dy = -1, .aR_dy = 0,  .eyeL = EYE_SQUINT, .eyeR = EYE_SQUINT, .mouth = MOUTH_OPEN, .blush = 1, .ms = 120 },
    { BASE_POSE, .dy = 0,  .aL_dy = 0,  .aR_dy = -1, .eyeL = EYE_SQUINT, .eyeR = EYE_SQUINT, .mouth = MOUTH_OPEN, .blush = 1, .ms = 120 },
    { BASE_POSE, .dy = -1, .aL_dy = -1, .aR_dy = 0,  .eyeL = EYE_SQUINT, .eyeR = EYE_SQUINT, .mouth = MOUTH_OPEN, .blush = 1, .ms = 120 },
    { BASE_POSE, .dy = 0,  .aL_dy = 0,  .aR_dy = -1, .eyeL = EYE_SQUINT, .eyeR = EYE_SQUINT, .mouth = MOUTH_OPEN, .blush = 1, .ms = 120 },
    { BASE_POSE, .dy = -1, .aL_dy = -1, .aR_dy = 0,  .eyeL = EYE_SQUINT, .eyeR = EYE_SQUINT, .mouth = MOUTH_OPEN, .blush = 1, .ms = 120 },
    { BASE_POSE, .dy = 0,  .aL_dy = 0,  .aR_dy = 0,  .eyeL = EYE_HAPPY,  .eyeR = EYE_HAPPY,  .mouth = MOUTH_SMILE, .blush = 1, .ms = 200 }
};
static const Anim ANIM_LAUGH = { "LAUGH", LAUGH_FRAMES, 6, 0, 2 };

/* 8. LOVE (Mắt trái tim, ôm tay, má hồng) */
static const Pose LOVE_FRAMES[] = {
    { BASE_POSE, .stretch = 0,  .aL_in = 1, .aR_in = 1, .eyeL = EYE_HEART, .eyeR = EYE_HEART, .mouth = MOUTH_SMILE, .blush = 1, .fx = FX_HEART, .ms = 400 },
    { BASE_POSE, .stretch = +1, .aL_in = 1, .aR_in = 1, .eyeL = EYE_HEART, .eyeR = EYE_HEART, .mouth = MOUTH_SMILE, .blush = 1, .fx = FX_HEART, .ms = 350 },
    { BASE_POSE, .stretch = 0,  .aL_in = 1, .aR_in = 1, .eyeL = EYE_HEART, .eyeR = EYE_HEART, .mouth = MOUTH_SMILE, .blush = 1, .fx = FX_HEART, .ms = 400 },
    { BASE_POSE, .stretch = 0,  .aL_in = 0, .aR_in = 0, .eyeL = EYE_HAPPY, .eyeR = EYE_HAPPY, .mouth = MOUTH_SMILE, .blush = 1, .ms = 300 }
};
static const Anim ANIM_LOVE = { "LOVE", LOVE_FRAMES, 4, 0, 2 };

/* 9. THINKING (Chống cằm suy nghĩ, mắt nhìn lên phải) */
static const Pose THINKING_FRAMES[] = {
    { BASE_POSE, .aL_in = 0, .aL_dy = 0, .aR_in = 1, .aR_dy = -1, .eyeDx = +1, .eyeDy = -1, .mouth = MOUTH_FLAT, .fx = FX_QUESTION, .ms = 600 },
    { BASE_POSE, .aL_in = 0, .aL_dy = 0, .aR_in = 1, .aR_dy = -1, .eyeDx = +1, .eyeDy = -1, .mouth = MOUTH_FLAT, .fx = FX_QUESTION, .ms = 700 },
    { BASE_POSE, .aL_in = 0, .aL_dy = 0, .aR_in = 0, .aR_dy = 0,  .eyeDx = 0,  .eyeDy = 0,  .mouth = MOUTH_NONE, .ms = 300 }
};
static const Anim ANIM_THINKING = { "THINKING", THINKING_FRAMES, 3, 0, 2 };

/* 10. SURPRISED (Giật mình, mắt tròn xoe, dấu chấm than) */
static const Pose SURPRISED_FRAMES[] = {
    { BASE_POSE, .dy = -1, .stretch = +1, .aL_dy = -1, .aR_dy = -1, .eyeL = EYE_WIDE, .eyeR = EYE_WIDE, .mouth = MOUTH_O, .fx = FX_EXCL, .ms = 250 },
    { BASE_POSE, .dy = 0,  .stretch = 0,  .aL_dy = 0,  .aR_dy = 0,  .eyeL = EYE_WIDE, .eyeR = EYE_WIDE, .mouth = MOUTH_O, .fx = FX_EXCL, .ms = 500 },
    { BASE_POSE, .stretch = -1, .eyeL = EYE_NORMAL, .eyeR = EYE_NORMAL, .mouth = MOUTH_NONE, .ms = 250 }
};
static const Anim ANIM_SURPRISED = { "SURPRISED", SURPRISED_FRAMES, 3, 0, 2 };

/* 11. CONFUSED (Bối rối, gãi đầu, dấu chấm hỏi) */
static const Pose CONFUSED_FRAMES[] = {
    { BASE_POSE, .aL_dy = 0, .aR_in = 1, .aR_dy = -2, .eyeL = EYE_NORMAL, .eyeR = EYE_HALF, .mouth = MOUTH_FROWN, .fx = FX_QUESTION, .ms = 500 },
    { BASE_POSE, .aL_dy = 0, .aR_in = 1, .aR_dy = -2, .eyeL = EYE_NORMAL, .eyeR = EYE_HALF, .mouth = MOUTH_FROWN, .fx = FX_QUESTION, .ms = 600 },
    { BASE_POSE, .aL_dy = 0, .aR_in = 0, .aR_dy = 0,  .eyeL = EYE_NORMAL, .eyeR = EYE_NORMAL, .mouth = MOUTH_NONE,  .ms = 250 }
};
static const Anim ANIM_CONFUSED = { "CONFUSED", CONFUSED_FRAMES, 3, 0, 2 };

/* 12. SLEEP (Ngáp ngủ, mắt nhắm nghiền, chữ Zzz bay) */
static const Pose SLEEP_FRAMES[] = {
    { BASE_POSE, .stretch = -1, .aL_dy = +2, .aR_dy = +2, .eyeL = EYE_CLOSED, .eyeR = EYE_CLOSED, .mouth = MOUTH_NONE, .fx = FX_Z, .ms = 700 },
    { BASE_POSE, .stretch = -1, .aL_dy = +2, .aR_dy = +2, .eyeL = EYE_CLOSED, .eyeR = EYE_CLOSED, .mouth = MOUTH_NONE, .fx = FX_Z, .ms = 800 },
    { BASE_POSE, .stretch = 0,  .aL_dy = +1, .aR_dy = +1, .eyeL = EYE_HALF,   .eyeR = EYE_HALF,   .mouth = MOUTH_O,    .ms = 400 }
};
static const Anim ANIM_SLEEP = { "SLEEP", SLEEP_FRAMES, 3, 0, 2 };

/* Danh sách các hành động đặc biệt để chọn ngẫu nhiên liên tiếp */
#define NUM_ACTIONS 10
static const Anim *RANDOM_ACTIONS[NUM_ACTIONS] = {
    &ANIM_WAVE,
    &ANIM_JUMP,
    &ANIM_WALK,
    &ANIM_DANCE,
    &ANIM_LAUGH,
    &ANIM_LOVE,
    &ANIM_THINKING,
    &ANIM_SURPRISED,
    &ANIM_CONFUSED,
    &ANIM_SLEEP
};

/* Animation engine state */
static const Anim *anim_cur = &ANIM_IDLE;
static uint8_t     anim_idx = 0;
static uint32_t    anim_t_next = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);

/* USER CODE BEGIN PFP */
static void LCD_Init_RGB565(uint32_t fb_addr);
static uint32_t prng_next(void);
static uint32_t prng_range(uint32_t min, uint32_t max);

static void anim_play(const Anim *a);
static const Pose *anim_tick(uint32_t now);
static void overlay_blink(Pose *p, uint32_t now);
static void overlay_look(Pose *p, uint32_t now);

static void fb_clear(uint16_t *fb, uint16_t c);
static void cell_fill(uint16_t *fb, int cx, int cy, uint16_t c);
static uint16_t cell_get(const uint16_t *fb, int cx, int cy);
static void draw_bmp(uint16_t *fb, const Bmp *b, int cx, int cy, uint16_t color, int flipX);
static void draw_arm(uint16_t *fb, int left, int in, int dy, int ox, int oy);
static void eye_draw(uint16_t *fb, uint8_t eyeId, int flipX, int cx, int cy, const Pose *p);
static void mouth_draw(uint16_t *fb, uint8_t mouthId, int cx, int cy);
static void fx_draw(uint16_t *fb, uint8_t fxId);
static void render(uint16_t *fb, const Pose *p);
static void present(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* ST BSP declares LtdcHandler in stm32f429i_discovery_lcd.c */
extern LTDC_HandleTypeDef LtdcHandler;

/**
 * @brief  Khởi tạo LCD và cấu hình Layer 0 sang RGB565 cho Pixel Bot
 */
static void LCD_Init_RGB565(uint32_t fb_addr)
{
    /* 1. Gọi khởi tạo phần cứng từ ST BSP (SPI5, SDRAM, LTDC timing) */
    BSP_LCD_Init();

    /* 2. Cấu hình lại Layer 0 sang chuẩn RGB565 thay vì ARGB8888 */
    LCD_LayerCfgTypeDef Layercfg;
    Layercfg.WindowX0 = 0;
    Layercfg.WindowX1 = BSP_LCD_GetXSize();
    Layercfg.WindowY0 = 0;
    Layercfg.WindowY1 = BSP_LCD_GetYSize();
    Layercfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
    Layercfg.FBStartAdress = fb_addr;
    Layercfg.Alpha = 255;
    Layercfg.Alpha0 = 0;
    Layercfg.Backcolor.Blue = 0;
    Layercfg.Backcolor.Green = 0;
    Layercfg.Backcolor.Red = 0;
    Layercfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    Layercfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
    Layercfg.ImageWidth = BSP_LCD_GetXSize();
    Layercfg.ImageHeight = BSP_LCD_GetYSize();

    HAL_LTDC_ConfigLayer(&LtdcHandler, &Layercfg, LCD_BACKGROUND_LAYER);
    BSP_LCD_SelectLayer(LCD_BACKGROUND_LAYER);
}

/**
 * @brief  Sinh số ngẫu nhiên 32-bit (LCG)
 */
static uint32_t prng_next(void)
{
    prng_state = prng_state * 1664525UL + 1013904223UL;
    return prng_state;
}

static uint32_t prng_range(uint32_t min, uint32_t max)
{
    if (min >= max) return min;
    return min + (prng_next() % (max - min + 1));
}

/**
 * @brief  Xóa toàn bộ Framebuffer bằng màu 16-bit
 */
static void fb_clear(uint16_t *fb, uint16_t c)
{
    uint32_t v = (uint32_t)c | ((uint32_t)c << 16);
    uint32_t *p = (uint32_t *)fb;
    uint32_t count = (SCR_W * SCR_H) / 2;
    for (uint32_t i = 0; i < count; i++) {
        p[i] = v;
    }
}

/**
 * @brief  Tô màu một ô (CELL x CELL pixel) trên lưới
 */
static inline void cell_fill(uint16_t *fb, int cx, int cy, uint16_t c)
{
    if ((unsigned)cx >= COLS || (unsigned)cy >= ROWS) return; /* Clipping */
    uint16_t *p = fb + (cy * CELL) * SCR_W + (cx * CELL);
    for (int y = 0; y < CELL; y++, p += SCR_W) {
        for (int x = 0; x < CELL; x++) {
            p[x] = c;
        }
    }
}

/**
 * @brief  Đọc màu điểm ảnh gốc của ô (cx, cy)
 */
static inline uint16_t cell_get(const uint16_t *fb, int cx, int cy)
{
    if ((unsigned)cx >= COLS || (unsigned)cy >= ROWS) return 0;
    return fb[(cy * CELL) * SCR_W + (cx * CELL)];
}

/**
 * @brief  Vẽ bitmap 1-bit lên lưới ô, hỗ trợ lật ngang (flipX)
 */
static void draw_bmp(uint16_t *fb, const Bmp *b, int cx, int cy, uint16_t color, int flipX)
{
    if (!b || !b->rows) return;
    for (int y = 0; y < b->h; y++) {
        for (int x = 0; x < b->w; x++) {
            int bit = flipX ? x : (b->w - 1 - x);
            if (b->rows[y] & (1 << bit)) {
                cell_fill(fb, cx + x, cy + y, color);
            }
        }
    }
}

/**
 * @brief  Vẽ cánh tay 2x2 ô
 */
static void draw_arm(uint16_t *fb, int left, int in, int dy, int ox, int oy)
{
    int x0 = left ? (0 + in) : (14 - in);
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 2; i++) {
            int cx = CHAR_X + ox + x0 + i;
            int cy = CHAR_Y + oy + 4 + dy + j;
            uint16_t under = cell_get(fb, cx, cy);
            /* Nếu ô tay đè lên thân -> dùng màu tối C_BODY_SH để tạo chiều sâu */
            uint16_t color = (under == C_BODY || under == C_BODY_HI || under == C_BODY_SH)
                             ? C_BODY_SH : C_BODY;
            cell_fill(fb, cx, cy, color);
        }
    }
}

/**
 * @brief  Vẽ mắt
 */
static void eye_draw(uint16_t *fb, uint8_t eyeId, int flipX, int cx, int cy, const Pose *p)
{
    if (eyeId >= EYE_COUNT) eyeId = EYE_NORMAL;
    uint16_t color;

    if (eyeId == EYE_HEART) {
        color = C_HEART;
    } else if (eyeId == EYE_SPARKLE) {
        color = C_YELLOW;
    } else {
        color = (p->eyeColor == 1) ? C_CYAN : C_EYE;
    }

    int ox = (eyeId == EYE_NORMAL || eyeId == EYE_WIDE) ? p->eyeDx : 0;
    int oy = (eyeId == EYE_NORMAL || eyeId == EYE_WIDE) ? p->eyeDy : 0;

    draw_bmp(fb, &EYE_BMPS[eyeId], cx + ox, cy + oy, color, flipX);
}

/**
 * @brief  Vẽ miệng
 */
static void mouth_draw(uint16_t *fb, uint8_t mouthId, int cx, int cy)
{
    if (mouthId == MOUTH_NONE || mouthId >= MOUTH_COUNT) return;
    draw_bmp(fb, &MOUTH_BMPS[mouthId], cx, cy, C_EYE, 0);
}

/**
 * @brief  Vẽ hiệu ứng FX phía trên nhân vật
 */
static void fx_draw(uint16_t *fb, uint8_t fxId)
{
    switch (fxId) {
        case FX_HEART:
            draw_bmp(fb, &FX_HEART_BMP, CHAR_X + 6, CHAR_Y - 4, C_HEART, 0);
            break;
        case FX_QUESTION:
            draw_bmp(fb, &FX_QUEST_BMP, CHAR_X + 13, CHAR_Y - 4, C_YELLOW, 0);
            break;
        case FX_EXCL:
            draw_bmp(fb, &FX_EXCL_BMP, CHAR_X + 8, CHAR_Y - 5, C_YELLOW, 0);
            break;
        case FX_Z:
            draw_bmp(fb, &FX_Z_BMP, CHAR_X + 13, CHAR_Y - 2, C_CYAN, 0);
            draw_bmp(fb, &FX_Z_BMP, CHAR_X + 15, CHAR_Y - 4, C_CYAN, 0);
            break;
        case FX_SPARK:
            draw_bmp(fb, &FX_SPARK_BMP, CHAR_X + 1, CHAR_Y - 2, C_YELLOW, 0);
            draw_bmp(fb, &FX_SPARK_BMP, CHAR_X + 14, CHAR_Y - 3, C_YELLOW, 0);
            break;
        case FX_WAVE:
            draw_bmp(fb, &FX_WAVE_BMP, CHAR_X - 1, CHAR_Y + 2, C_CYAN, 0);
            draw_bmp(fb, &FX_WAVE_BMP, CHAR_X + 16, CHAR_Y + 2, C_CYAN, 0);
            break;
        case FX_ANGER:
            draw_bmp(fb, &FX_ANGER_BMP, CHAR_X + 12, CHAR_Y - 3, C_RED, 0);
            break;
        default:
            break;
    }
}

/**
 * @brief  Render toàn bộ nhân vật theo đúng thứ tự Layer (Painter's Algorithm)
 */
static void render(uint16_t *fb, const Pose *p)
{
    int ox = p->dx, oy = p->dy;
    int up = -p->stretch;  /* Phần trên nâng/hạ theo độ co giãn thân */
    int top = -p->stretch; /* Hàng đỉnh của thân */

    /* 1. Xóa nền màn hình */
    fb_clear(fb, C_BG);

    /* 2. Lớp bóng đổ (cố định ở mặt đất, co nhỏ khi bot nhảy cao) */
    int h = -p->dy;
    int w = (h <= 1) ? 12 : (h <= 3) ? 10 : 8;
    int shadow_col = CHAR_X + 3 + (12 - w) / 2 + p->dx;
    for (int i = 0; i < w; i++) {
        cell_fill(fb, shadow_col + i, GROUND_ROW, C_SHADOW);
    }

    /* 3. Lớp chân (4 chân tại cột 3, 5, 10, 12) */
    for (int i = 0; i < 4; i++) {
        int height = LEG_H(p->legs, i);
        for (int r = 0; r < height; r++) {
            cell_fill(fb, CHAR_X + ox + LEG_COL[i], CHAR_Y + oy + 8 + r, C_BODY_SH);
        }
    }

    /* 4. Lớp thân (Cột 2..13, hàng top..7) */
    for (int r = top; r <= 7; r++) {
        uint16_t color = (r == top) ? C_BODY_HI : (r == 7 ? C_BODY_SH : C_BODY);
        for (int c = 2; c <= 13; c++) {
            cell_fill(fb, CHAR_X + ox + c, CHAR_Y + oy + r, color);
        }
    }

    /* 5. Lớp tay (Khối 2x2) */
    draw_arm(fb, 1, p->aL_in, p->aL_dy + up, ox, oy); /* Tay trái */
    draw_arm(fb, 0, p->aR_in, p->aR_dy + up, ox, oy); /* Tay phải */

    /* 6. Lớp má hồng (Tô màu hồng ở hàng 4) */
    if (p->blush) {
        for (int c = 3; c <= 4; c++)   cell_fill(fb, CHAR_X + ox + c, CHAR_Y + oy + 4 + up, C_PINK);
        for (int c = 11; c <= 12; c++) cell_fill(fb, CHAR_X + ox + c, CHAR_Y + oy + 4 + up, C_PINK);
    }

    /* 7. Lớp mắt (Mắt trái và mắt phải lật ngang) */
    eye_draw(fb, p->eyeL, 0, CHAR_X + ox + 3,  CHAR_Y + oy + 1 + up, p);
    eye_draw(fb, p->eyeR, 1, CHAR_X + ox + 10, CHAR_Y + oy + 1 + up, p);

    /* 8. Lớp miệng */
    mouth_draw(fb, p->mouth, CHAR_X + ox + 6, CHAR_Y + oy + 4 + up);

    /* 9. Lớp hiệu ứng FX */
    fx_draw(fb, p->fx);
}

/**
 * @brief  Tráo Framebuffer đồng bộ VSYNC (Chống xé hình)
 */
static void present(void)
{
    /* Trỏ phần cứng LTDC Layer 0 vào Backbuffer */
    HAL_LTDC_SetAddress_NoReload(&LtdcHandler, (uint32_t)back, LCD_BACKGROUND_LAYER);
    /* Yêu cầu nạp thanh ghi tại thời điểm Vertical Blanking */
    LTDC->SRCR = LTDC_SRCR_VBR;

    /* Hoán đổi con trỏ Front và Back */
    uint16_t *t = front;
    front = back;
    back = t;
}

/**
 * @brief  Kích hoạt một Animation mới
 */
static void anim_play(const Anim *a)
{
    if (anim_cur && a->priority < anim_cur->priority) return;
    anim_cur = a;
    anim_idx = 0;
    anim_t_next = HAL_GetTick() + a->frames[0].ms;
    printf("[BOT] Action: %s (Priority %d)\r\n", a->name, a->priority);
}

/**
 * @brief  Cập nhật tiến độ Animation theo thời gian
 */
static const Pose *anim_tick(uint32_t now)
{
    if (now >= anim_t_next) {
        anim_idx++;
        if (anim_idx >= anim_cur->num_frames) {
            if (anim_cur->loop) {
                anim_idx = 0;
            } else {
                /* Kết thúc hành động -> Tự động quay về IDLE */
                anim_cur = &ANIM_IDLE;
                anim_idx = 0;
            }
        }
        anim_t_next = now + anim_cur->frames[anim_idx].ms;
    }
    return &anim_cur->frames[anim_idx];
}

/**
 * @brief  Lớp phủ chớp mắt tự nhiên (Overlay Blink)
 */
static void overlay_blink(Pose *p, uint32_t now)
{
    static uint32_t t_blink_start = 0;
    static uint32_t t_next_blink  = 3000;
    static uint8_t  blinking      = 0;

    /* Chỉ chớp mắt khi mắt đang là NORMAL */
    if (p->eyeL != EYE_NORMAL || p->eyeR != EYE_NORMAL) return;

    if (!blinking && (now >= t_next_blink)) {
        blinking = 1;
        t_blink_start = now;
        /* Chu kỳ chớp mắt kế tiếp: ngẫu nhiên 2500 - 5500 ms */
        t_next_blink = now + prng_range(2500, 5500);
    }

    if (blinking) {
        uint32_t dt = now - t_blink_start;
        if (dt < 50) {
            p->eyeL = EYE_HALF;   p->eyeR = EYE_HALF;
        } else if (dt < 130) {
            p->eyeL = EYE_CLOSED; p->eyeR = EYE_CLOSED;
        } else if (dt < 180) {
            p->eyeL = EYE_HALF;   p->eyeR = EYE_HALF;
        } else {
            blinking = 0;
        }
    }
}

/**
 * @brief  Lớp phủ liếc nhìn xung quanh (Overlay Look Around)
 */
static void overlay_look(Pose *p, uint32_t now)
{
    static uint32_t t_look_start = 0;
    static uint32_t t_next_look  = 8000;
    static uint8_t  looking      = 0;

    if (anim_cur != &ANIM_IDLE) return;

    if (!looking && (now >= t_next_look)) {
        looking = 1;
        t_look_start = now;
        t_next_look = now + prng_range(8000, 15000);
    }

    if (looking) {
        uint32_t dt = now - t_look_start;
        if (dt < 500) {
            p->eyeDx = -1; /* Nhìn sang trái */
        } else if (dt < 1000) {
            p->eyeDx = +1; /* Nhìn sang phải */
        } else {
            p->eyeDx = 0;
            looking = 0;
        }
    }
}

/**
 * @brief  Chuyển hướng hàm printf qua cổng USART1
 */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 10);
    return ch;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* Configure the system clock to 180 MHz */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  printf("\r\n==============================================\r\n");
  printf("  STM32F429I-DISC1 ANIMATED PIXEL BOT INITIALIZED\r\n");
  printf("==============================================\r\n");

  /* Khởi tạo màn hình LCD 240x320 RGB565 qua ST BSP */
  LCD_Init_RGB565(FB0_ADDR);

  /* Bắt đầu lắng nghe lệnh điều khiển từ UART */
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

  /* Khởi chạy hoạt cảnh chào mừng BOOT */
  anim_play(&ANIM_BOOT);

  uint32_t last_render = 0;
  uint32_t t_next_action = HAL_GetTick() + 4000; /* Lên lịch hành động ngẫu nhiên đầu tiên */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint32_t now = HAL_GetTick();

    /* -----------------------------------------------------------------------
     * 1. ĐIỀU PHỐI ĐỒ HỌA 30 FPS (~33 ms / khung hình)
     * ----------------------------------------------------------------------- */
    /* Chỉ vẽ khi đến chu kỳ 33 ms VÀ khối LTDC đã hoàn tất reload VSYNC trước đó */
    if ((now - last_render >= 33) && !(LTDC->SRCR & LTDC_SRCR_VBR))
    {
        last_render = now;

        /* Lấy Pose hiện tại từ Animation Engine */
        Pose p = *anim_tick(now);

        /* Áp dụng các lớp phủ tự nhiên (Chớp mắt, Liếc nhìn) */
        overlay_blink(&p, now);
        overlay_look(&p, now);

        /* Vẽ nhân vật lên Backbuffer */
        render(back, &p);

        /* Tráo Backbuffer ra màn hình đồng bộ VSYNC */
        present();

        /* Toggle LED xanh PG13 làm nhịp tim Heartbeat */
        HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_13);
    }

    /* -----------------------------------------------------------------------
     * 2. MÁY TRẠNG THÁI HÀNH ĐỘNG NGẪU NHIÊN LIÊN TỤC
     * ----------------------------------------------------------------------- */
    /* Khi bot đang rảnh rỗi ở trạng thái IDLE và đã hết thời gian nghỉ (2 - 4s):
     * Tự động chọn ngẫu nhiên một trong các hành động đặc biệt để diễn */
    if (anim_cur == &ANIM_IDLE && now >= t_next_action)
    {
        uint32_t rand_idx = prng_range(0, NUM_ACTIONS - 1);
        anim_play(RANDOM_ACTIONS[rand_idx]);

        /* Lên lịch cho hành động kế tiếp: Nghỉ 2000 - 4500 ms */
        t_next_action = now + prng_range(2000, 4500);
    }

    /* -----------------------------------------------------------------------
     * 3. PHẢN HỒI NÚT NHẤN NGƯỜI DÙNG PA0
     * ----------------------------------------------------------------------- */
    if (flag_btn)
    {
        flag_btn = 0;
        prng_state ^= now; /* Trộn seed ngẫu nhiên từ thời điểm bấm nút */
        printf("[BTN] User button pressed! Triggering excited reaction...\r\n");

        /* Nhấn nút sẽ kích hoạt hành động nhảy JUMP hoặc DANCE */
        if (prng_range(0, 1) == 0) {
            anim_play(&ANIM_JUMP);
        } else {
            anim_play(&ANIM_DANCE);
        }
        t_next_action = now + 4000;
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration (180 MHz from 8 MHz HSE Bypass)
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /* Configure PG13 (Green LED) & PG14 (Red LED) */
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /* Configure PA0 (User Button) as EXTI0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init */
  HAL_NVIC_SetPriority(EXTI0_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

/* USER CODE BEGIN 4 */

/**
 * @brief  Callback ngắt ngoài cho nút nhấn PA0 (Debounce 150 ms)
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        static uint32_t last_press = 0;
        uint32_t now = HAL_GetTick();
        if (now - last_press > 150)
        {
            last_press = now;
            flag_btn = 1;
        }
    }
}

/**
 * @brief  Callback ngắt nhận dữ liệu qua UART1
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        /* Điều khiển bot bằng phím gõ từ terminal */
        switch (rx_byte)
        {
            case 'w': anim_play(&ANIM_WAVE);      break;
            case 'j': anim_play(&ANIM_JUMP);      break;
            case 'k': anim_play(&ANIM_WALK);      break;
            case 'd': anim_play(&ANIM_DANCE);     break;
            case 'l': anim_play(&ANIM_LAUGH);     break;
            case 'h': anim_play(&ANIM_LOVE);      break;
            case 't': anim_play(&ANIM_THINKING);  break;
            case '!': anim_play(&ANIM_SURPRISED); break;
            case '?': anim_play(&ANIM_CONFUSED);  break;
            case 's': anim_play(&ANIM_SLEEP);     break;
            case 'r': /* Kích hoạt ngẫu nhiên */
                anim_play(RANDOM_ACTIONS[prng_range(0, NUM_ACTIONS - 1)]);
                break;
            default:
                break;
        }

        /* Tái kích hoạt nhận byte tiếp theo */
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* Bật đèn LED đỏ PG14 để báo hiệu lỗi phần cứng */
  __disable_irq();
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_SET);
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
