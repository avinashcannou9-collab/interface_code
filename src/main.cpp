#include <Arduino.h>

#include "lvgl.h"

#include "lvglDrivers.h"

#include <STM32encoder.h>

#include <HardwareTimer.h>



// --- BROCHES MATERIELLES MOTEUR ---



const int pinPWM = 6;  

const int pinSens = 7;

const long TICKS_PAR_TOUR = 527;



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
        
        // Commande moteur directement ici, une seule fois
        if (moteurEnMarche) {
            digitalWrite(pinSens, HIGH);
            analogWrite(pinPWM, 195);
            Serial.printf("Moteur ON\n");
        } else {
            analogWrite(pinPWM, 0);
            Serial.printf("Moteur OFF\n");
        }
    }
}



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

   

    // Fond : Bleu nuit très sombre (couleur de l'eau et des ombres en bas)

    lv_obj_set_style_bg_color(roue, lv_color_hex(0x1B1D36), 0);

   

    // Bordure : Orange/Doré (couleur du ciel au centre et des guirlandes lumineuses)

    lv_obj_set_style_border_width(roue, 8, 0);

    lv_obj_set_style_border_color(roue, lv_color_hex(0xF69E53), 0);



    // --- 2. L'AIGUILLE ---

    aiguille = lv_obj_create(roue);

    lv_obj_clear_flag(aiguille, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_size(aiguille, 80, 6);

    lv_obj_align(aiguille, LV_ALIGN_CENTER, 40, 0);

   

   

    // Couleur : Rouge brique vif

    lv_obj_set_style_bg_color(aiguille, lv_color_hex(0xD72A28), 0);

    lv_obj_set_style_border_width(aiguille, 0, 0);

   

    // Pivot au centre de la roue pour la future rotation

    lv_obj_set_style_transform_pivot_x(aiguille, 0, 0);

    lv_obj_set_style_transform_pivot_y(aiguille, 3, 0);



    // --- 3. LE BOUTON PLAY (À droite) ---

    btn_play = lv_button_create(lv_screen_active());

    lv_obj_add_event_cb(btn_play, event_handler, LV_EVENT_ALL, NULL);

    lv_obj_set_size(btn_play, 120, 60);

    lv_obj_align(btn_play, LV_ALIGN_RIGHT_MID, -60, 0); // Décalé sur la droite

    lv_obj_add_flag(btn_play, LV_OBJ_FLAG_CHECKABLE);

    lv_obj_set_style_bg_color(btn_play, lv_color_hex(0xFF0000), 0); // Rouge au repos



    lv_obj_t * label_btn = lv_label_create(btn_play);

    lv_label_set_text(label_btn, "PLAY");

    lv_obj_center(label_btn);



    // --- 4. LE TEXTE DU COMPTEUR ---

    label_compteur = lv_label_create(lv_screen_active());

    lv_label_set_text(label_compteur, "Degres : 0");

    // On met un petit fond sombre pour qu'il soit lisible sur l'image

    lv_obj_set_style_bg_color(label_compteur, lv_color_hex(0x000000), 0);

    lv_obj_set_style_bg_opa(label_compteur, 150, 0);

    lv_obj_set_style_text_color(label_compteur, lv_color_hex(0xFFFFFF), 0);

    lv_obj_align(label_compteur, LV_ALIGN_BOTTOM_MID, 0, -20);

}



#ifdef ARDUINO



void mySetup() {

    Serial.begin(115200);

    pinMode(D0, INPUT_PULLUP);

    pinMode(D1, INPUT_PULLUP);

    pinMode(pinPWM, OUTPUT);

    pinMode(pinSens, OUTPUT);

    digitalWrite(pinPWM, LOW);

    testLvgl();

    pin_function(PC_6, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3));

    pin_function(PC_7, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3));



    // Using pin = PA0, PA1, etc.

    PinName pinNameToUse = digitalPinToPinName(PH6);



    // Automatically retrieve TIM instance and channel associated to pin

    TIM_TypeDef *Instance = (TIM_TypeDef *)pinmap_peripheral(pinNameToUse, PinMap_PWM);

   

    if (Instance != nullptr) {

      uint8_t timerIndex = get_timer_index(Instance);

      Serial.printf("Timer %d\n", (int)timerIndex);

    } else {

      Serial.printf("No instance\n");

    }

}



void loop() {
    static unsigned long tempsLvgl = 0;
    if (millis() - tempsLvgl >= 5) {
        tempsLvgl = millis();
        lv_timer_handler();
    }

    // PLUS DE SECTION MOTEUR ICI

    // --- MISE À JOUR AIGUILLE (toutes les 50ms) ---
    static unsigned long tempsAiguille = 0;
    if (millis() - tempsAiguille >= 50) {
        tempsAiguille = millis();

        uint16_t tics_bruts = TIM3->CNT;
        long degres = ((long)tics_bruts % TICKS_PAR_TOUR) * 360L / TICKS_PAR_TOUR;
        lv_obj_set_style_transform_rotation(aiguille, (int16_t)(degres * 10), 0);

        char buf[32];
        snprintf(buf, sizeof(buf), "Degres : %ld", degres);
        lv_label_set_text(label_compteur, buf);
    }

    // --- DIAGNOSTIC (toutes les 500ms) ---
    static unsigned long tempsDiag = 0;
    if (millis() - tempsDiag >= 500) {
        tempsDiag = millis();
        Serial.printf("Bouton PLAY : %d | Tics : %u\n", moteurEnMarche, TIM3->CNT);
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