#include <Arduino.h>
#include "lvgl.h"
#include "lvglDrivers.h"
#include <STM32encoder.h>
#include <HardwareTimer.h>
#include <math.h>

// --- BROCHES MATERIELLES MOTEUR ---
const int pinPWM = 6;  
const int pinSens = 7;
const long TICKS_PAR_TOUR = 703; 

// Initialisation sur TIM3
STM32encoder myEnc(TIM3,0,65535);
volatile bool moteurEnMarche = false;

// --- VARIABLES GLOBALES DE L'INTERFACE ---
lv_obj_t * label_compteur;
lv_obj_t * roue;
lv_obj_t * aiguille;
lv_obj_t * btn_play;

// --- DÉCLARATION DE L'IMAGE ---
LV_IMG_DECLARE(Carnaval);

// --- GESTION DE L'ÉCRAN TACTILE ---
static void event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        moteurEnMarche = lv_obj_has_state(obj, LV_STATE_CHECKED);
    }
}

static lv_point_precise_t points_aiguille[2];

void testLvgl() {
    lv_obj_clean(lv_screen_active());

    // --- AFFICHAGE DE L'IMAGE EN FOND ---
    lv_obj_t * img_fond = lv_img_create(lv_screen_active());
    lv_obj_clear_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);
    lv_img_set_src(img_fond, &Carnaval);
    lv_obj_align(img_fond, LV_ALIGN_CENTER, 0, 0);

    // --- 1. La ROUE ---
    roue = lv_obj_create(lv_screen_active());
    lv_obj_set_size(roue, 200, 200);
    lv_obj_align(roue, LV_ALIGN_LEFT_MID, 40, 0);
    lv_obj_set_style_radius(roue, LV_RADIUS_CIRCLE, 0);
    
    lv_obj_set_style_bg_color(roue, lv_color_hex(0x1B1D36), 0); 
    lv_obj_set_style_border_width(roue, 8, 0);
    lv_obj_set_style_border_color(roue, lv_color_hex(0xF69E53), 0);

    // --- 2. L'AIGUILLE ---
    aiguille = lv_line_create(roue); 
    lv_obj_clear_flag(aiguille, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_line_width(aiguille, 6, 0);
    lv_obj_set_style_line_color(aiguille, lv_color_hex(0xD72A28), 0);
    lv_obj_set_style_line_rounded(aiguille, true, 0);

    // Centrage absolu sur le bord de la roue
    lv_obj_set_pos(aiguille, 0, 0);

    points_aiguille[0].x = (lv_value_precise_t)80.0f;
    points_aiguille[0].y = (lv_value_precise_t)80.0f;
    points_aiguille[1].x = (lv_value_precise_t)80.0f;
    points_aiguille[1].y = (lv_value_precise_t)80.0f;
    
    lv_line_set_points_mutable(aiguille, points_aiguille, 2);

    // --- 3. LE BOUTON PLAY (À droite) ---
    btn_play = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn_play, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_size(btn_play, 120, 60);
    lv_obj_align(btn_play, LV_ALIGN_RIGHT_MID, -60, 0); 
    lv_obj_add_flag(btn_play, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(btn_play, lv_color_hex(0xFF0000), 0); 

    lv_obj_t * label_btn = lv_label_create(btn_play);
    lv_label_set_text(label_btn, "PLAY");
    lv_obj_center(label_btn);

    // --- 4. LE TEXTE DU COMPTEUR ---
    label_compteur = lv_label_create(lv_screen_active());
    lv_label_set_text(label_compteur, "Degres : 0");
    lv_obj_set_style_bg_color(label_compteur, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(label_compteur, 150, 0);
    lv_obj_set_style_text_color(label_compteur, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(label_compteur, LV_ALIGN_BOTTOM_MID, 0, -20);
}

#ifdef ARDUINO

void mySetup() {
    Serial.begin(115200);
    
    // --- LE PATCH ENCODEUR MATERIEL ABSOLU EST ICI ---
    __HAL_RCC_TIM3_CLK_ENABLE(); // Allume l'horloge du Timer 3
    TIM3->SMCR |= TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1;      // Force le STM32 en Mode Encodeur
    TIM3->CCMR1 |= TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0; // Connecte le compteur aux broches
    TIM3->CR1 |= TIM_CR1_CEN;    // Force le compteur à démarrer
    // -------------------------------------------------

    pinMode(D0, INPUT_PULLUP);
    pinMode(D1, INPUT_PULLUP);
    pinMode(pinPWM, OUTPUT);
    pinMode(pinSens, OUTPUT);
    digitalWrite(pinPWM, LOW);
    
    testLvgl();
    
    pin_function(PC_6, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3));
    pin_function(PC_7, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3));

    PinName pinNameToUse = digitalPinToPinName(PH6);
    TIM_TypeDef *Instance = (TIM_TypeDef *)pinmap_peripheral(pinNameToUse, PinMap_PWM);
    
    if (Instance != nullptr) {
      uint8_t timerIndex = get_timer_index(Instance);
      Serial.printf("Timer %d\n", (int)timerIndex);
    } else {
      Serial.printf("No instance\n");
    }
}

void loop() {
    // --- 1. MOTEUR DE L'ÉCRAN TACTILE ---
    lv_timer_handler(); 

    // --- 2. TON BLOC MOTEUR ---
    if (moteurEnMarche) {
        digitalWrite(pinSens, HIGH);
        analogWrite(pinPWM, 195); 
        
        long valeur_angle = TIM3->CNT; // Maintenant, ça va lire les vraies valeurs !
        
        float angle_degres = -(((float)valeur_angle * 360.0f) / (float)TICKS_PAR_TOUR);
        int angle_entier = (int)angle_degres % 360;
        if (angle_entier < 0) angle_entier += 360;

        float angle_radians = (angle_degres - 90.0f) * (M_PI / 180.0f);
        
        lv_value_precise_t nouv_x = (lv_value_precise_t)(80.0f + 80.0f * cosf(angle_radians));
        lv_value_precise_t nouv_y = (lv_value_precise_t)(80.0f + 80.0f * sinf(angle_radians));

        if (aiguille) {
            points_aiguille[0].x = (lv_value_precise_t)80.0f;
            points_aiguille[0].y = (lv_value_precise_t)80.0f;
            points_aiguille[1].x = nouv_x;
            points_aiguille[1].y = nouv_y;
            
            lv_line_set_points_mutable(aiguille, points_aiguille, 2);
        }

        lv_label_set_text_fmt(label_compteur, "Degres : %d", angle_entier);
        
    } else {
        analogWrite(pinPWM, 0);
    }

    // --- 3. DIAGNOSTIC COMPLET (Toutes les 100ms) ---
    static unsigned long tempsPrecedent = 0;
    if (millis() - tempsPrecedent >= 100) {
        tempsPrecedent = millis();
        Serial.printf("Bouton PLAY : %d | Tics encodeur : %ld\n", moteurEnMarche, TIM3->CNT);
    }
}

void myTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
    }
}

#else
#include "app_hal.h"
#include <cstdio>

int main(void) {
    lv_init(); hal_setup(); testLvgl(); hal_loop(); return 0;
}
#endif