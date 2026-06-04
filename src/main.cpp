#include <Arduino.h>
#include "lvgl.h"
#include "lvglDrivers.h"
#include <STM32encoder.h>
#include <fete.h>

// --- BROCHES MATERIELLES MOTEUR ---
const int pinPWM = 6;  
const int pinSens = 7;
const long TICKS_PAR_TOUR = 527;

// Initialisation matérielle sur TIM3
STM32encoder myEnc(TIM3, 0, 65535);
volatile bool moteurEnMarche = false;

lv_obj_t * label_compteur;
lv_obj_t * btn_promo;
lv_obj_t * fleche_gauche;
lv_obj_t * fleche_droite;

// --- CALLBACKS ANIMATIONS (LVGL 9.2.2) ---
static void anim_scale_cb(void * var, int32_t v) {
    lv_obj_set_style_transform_scale((lv_obj_t *)var, v, 0);
}

static void anim_translate_x_cb(void * var, int32_t v) {
    lv_obj_set_style_translate_x((lv_obj_t *)var, v, 0);
}

static void event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        moteurEnMarche = lv_obj_has_state(obj, LV_STATE_CHECKED);
        
        // Effet flash forain au clic
        if(moteurEnMarche) {
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xFFFF00), 0); // Jaune fluo
            lv_obj_set_style_text_color(lv_obj_get_child(obj, 0), lv_color_hex(0x000000), 0); // Texte Noir
        } else {
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xFF0000), 0); // Rouge braderie
            lv_obj_set_style_text_color(lv_obj_get_child(obj, 0), lv_color_hex(0xFFFFFF), 0); // Texte Blanc
        }
    }
}

void creationInterfaceFoire() {
    lv_obj_clean(lv_screen_active());
    
    // --- image ---
    lv_obj_t * img_fond = lv_img_create(lv_screen_active());
    lv_img_set_src(img_fond, &carnival_bg);
    lv_obj_align(img_fond, LV_ALIGN_CENTER, 0, 0);

    

}

#ifdef ARDUINO

void mySetup() {
    Serial.begin(115200);
    
    // Configuration Moteur
    pinMode(pinPWM, OUTPUT);
    pinMode(pinSens, OUTPUT);
    
    // SÉCURITÉ MATÉRIELLE : Moteur coupé pour test IHM
    digitalWrite(pinPWM, LOW);
    
    creationInterfaceFoire();
    
    // Routage des broches PC6 et PC7 vers le Timer matériel TIM3
    pin_function(PC_6, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3));
    pin_function(PC_7, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3));
}

void loop() {
    // 1. Rafraîchissement tactile et animations
    lv_timer_handler(); 

    // 2. Gestion moteur commentée pour l'instant (isolation IHM)
    /*
    if (moteurEnMarche) {
        digitalWrite(pinSens, HIGH);
        analogWrite(pinPWM, 80); 
    } else {
        analogWrite(pinPWM, 0);
    }
    */

    // 3. Traitement et lecture du STM32Encoder (toutes les 100ms)
    static unsigned long tempsPrecedent = 0;
    if (millis() - tempsPrecedent >= 100) {
        tempsPrecedent = millis();
        
        long positionEncodeur = TIM3->CNT;
        
        long positionDegres = (positionEncodeur * 360) / TICKS_PAR_TOUR;
        positionDegres = positionDegres % 360;
        if (positionDegres < 0) {
            positionDegres += 360;
        }

        // Écran tactile
        lv_label_set_text_fmt(label_compteur, "Degres : %ld", positionDegres);
        
        // Console VSCode
        Serial.printf("Bouton : %d | Tics : %ld | Angle : %ld\n", moteurEnMarche, positionEncodeur, positionDegres);
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
    lv_init(); hal_setup(); creationInterfaceFoire(); hal_loop(); return 0;
}
#endif
