#include <Arduino.h>
#include "lvgl.h"
#include "lvglDrivers.h"
#include <STM32encoder.h>
// --- BROCHES MATERIELLES MOTEUR ---

const int pinPWM = 6;  
const int pinSens = 7;
const long TICKS_PAR_TOUR = 527;

// Initialisation sur TIM3

STM32encoder myEnc(TIM3,0,65535);
volatile bool moteurEnMarche = false;
lv_obj_t * label_compteur;

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

void testLvgl() {

    lv_obj_clean(lv_screen_active());

    // --- AFFICHAGE DE L'IMAGE EN FOND ---
    lv_obj_t * img_fond = lv_img_create(lv_screen_active());
    lv_img_set_src(img_fond, &Carnaval);
    lv_obj_align(img_fond, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * btn = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -30);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_size(btn, 220, 80);

    lv_obj_t * label_btn = lv_label_create(btn);
    lv_label_set_text(label_btn, "ON/OFF");
    lv_obj_center(label_btn);

    label_compteur = lv_label_create(lv_screen_active());
    lv_label_set_text(label_compteur, "Degres : 0");
    lv_obj_align(label_compteur, LV_ALIGN_CENTER, 0, 50);

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
}

void loop() {
    // --- 1. MOTEUR DE L'ÉCRAN TACTILE ---
    lv_timer_handler(); 

    // --- 2. GESTION DU HACHEUR MOTEUR ---
    if (moteurEnMarche) {
        digitalWrite(pinSens, HIGH);
        // On teste ton hypothèse : baisse de la puissance à 80
        analogWrite(pinPWM, 50); 
    } else {
        analogWrite(pinPWM, 0);
    }

    // --- 3. DIAGNOSTIC COMPLET (Toutes les 100ms) ---
    static unsigned long tempsPrecedent = 0;
    if (millis() - tempsPrecedent >= 100) {
        tempsPrecedent = millis();
        
        // On affiche l'état du bouton (0 ou 1) ET les tics matériels
        Serial.printf("Bouton ON/OFF : %d | Tics encodeur : %ld\n", moteurEnMarche, TIM3->CNT);
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