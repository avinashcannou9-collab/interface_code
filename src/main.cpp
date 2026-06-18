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

// --- VARIABLES DU JEU QTE ---
lv_obj_t * arc_zone;
lv_obj_t * label_score;
int score = 0;
int taille_zone = 90; // La zone commence très large (90 degrés)
int centre_zone = 0;

// --- VARIABLES POUR LE FLASH D'ÉCRAN ---
unsigned long temps_flash = 0;
bool ecran_en_flash = false;

lv_obj_t * ecran_principal;
lv_obj_t * ecran_secondaire;

// --- FONCTION POUR GÉNÉRER LA ZONE VERTE ---
void genererNouvelleZone() {
    centre_zone = random(0, 360);
    
    // On adapte les degrés mathématiques (0=haut) aux degrés LVGL (0=droite)
    int arc_debut = (centre_zone - (taille_zone / 2) + 270) % 360;
    int arc_fin = (centre_zone + (taille_zone / 2) + 270) % 360;
    
    if (arc_debut < 0) arc_debut += 360;
    if (arc_fin < 0) arc_fin += 360;

    lv_arc_set_angles(arc_zone, arc_debut, arc_fin);
}

// --- GESTION DE L'ÉCRAN TACTILE ---
static void event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        moteurEnMarche = lv_obj_has_state(obj, LV_STATE_CHECKED);
    }
}

static lv_point_precise_t points_aiguille[2];

// --- CALLBACKS CHANGEMENT DE PAGE ---
static void aller_page_2_cb(lv_event_t * e) {
    // Charge l'écran 2
    lv_screen_load(ecran_secondaire);
}

static void retour_page_1_cb(lv_event_t * e) {
    // Recharge l'écran du jeu
    lv_screen_load(ecran_principal);
}

void initialiserPages() {
    // 1. Sauvegarde l'écran actuel du jeu comme écran principal
    ecran_principal = lv_screen_active();

    // 2. Crée un bouton sur ton écran de jeu pour changer de page
    lv_obj_t * btn_menu = lv_button_create(ecran_principal);
    lv_obj_set_size(btn_menu, 80, 40);
    lv_obj_align(btn_menu, LV_ALIGN_TOP_RIGHT,0, 10);
    lv_obj_add_event_cb(btn_menu, aller_page_2_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t * label_menu = lv_label_create(btn_menu);
    lv_label_set_text(label_menu, "Page 2");
    lv_obj_center(label_menu);

    // 3. Crée le deuxième écran (NULL = crée un nouvel objet racine/écran)
    ecran_secondaire = lv_obj_create(NULL);
    // Met un fond différent pour bien voir le changement
    lv_obj_set_style_bg_color(ecran_secondaire, lv_color_hex(0x222222), 0); 

    // 4. Crée un bouton de retour sur ce deuxième écran
    lv_obj_t * btn_retour = lv_button_create(ecran_secondaire);
    lv_obj_set_size(btn_retour, 150, 60);
    lv_obj_align(btn_retour, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_retour, retour_page_1_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label_retour = lv_label_create(btn_retour);
    lv_label_set_text(label_retour, "Retour au Jeu");
    lv_obj_center(label_retour);
}

void testLvgl() {
    lv_obj_clean(lv_screen_active());

    // --- FOND DE L'ÉCRAN (Noir par défaut pour voir les flashs) ---
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), 0);

    // --- 1. La ROUE ---
    roue = lv_obj_create(lv_screen_active());
    lv_obj_set_size(roue, 200, 200);
    lv_obj_align(roue, LV_ALIGN_LEFT_MID, 40, 0);
    lv_obj_set_style_radius(roue, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(roue, lv_color_hex(0x1B1D36), 0); 
    lv_obj_set_style_border_width(roue, 8, 0);
    lv_obj_set_style_border_color(roue, lv_color_hex(0xF69E53), 0);

    // --- 1.5 LA ZONE VERTE DU JEU ---
    arc_zone = lv_arc_create(roue);
    lv_obj_set_size(arc_zone, 200, 200);
    lv_obj_center(arc_zone);
    lv_obj_remove_style(arc_zone, NULL, LV_PART_KNOB); 
    lv_obj_clear_flag(arc_zone, LV_OBJ_FLAG_CLICKABLE); 
    lv_obj_set_style_arc_width(arc_zone, 0, LV_PART_MAIN); 
    lv_obj_set_style_arc_width(arc_zone, 12, LV_PART_INDICATOR); 
    lv_obj_set_style_arc_color(arc_zone, lv_color_hex(0x00FF00), LV_PART_INDICATOR);

    genererNouvelleZone(); 

    // --- 2. L'AIGUILLE ---
    aiguille = lv_line_create(roue); 
    lv_obj_clear_flag(aiguille, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_line_width(aiguille, 6, 0);
    lv_obj_set_style_line_color(aiguille, lv_color_hex(0xD72A28), 0);
    lv_obj_set_style_line_rounded(aiguille, true, 0);
    lv_obj_set_pos(aiguille, 0, 0);

    points_aiguille[0].x = (lv_value_precise_t)80.0f;
    points_aiguille[0].y = (lv_value_precise_t)80.0f;
    points_aiguille[1].x = (lv_value_precise_t)80.0f;
    points_aiguille[1].y = (lv_value_precise_t)80.0f;
    lv_line_set_points_mutable(aiguille, points_aiguille, 2);

    // --- 3. LE BOUTON PLAY ---
    btn_play = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn_play, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_size(btn_play, 120, 60);
    lv_obj_align(btn_play, LV_ALIGN_RIGHT_MID, -60, 0); 
    lv_obj_add_flag(btn_play, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(btn_play, lv_color_hex(0xFF0000), 0); 

    lv_obj_t * label_btn = lv_label_create(btn_play);
    lv_label_set_text(label_btn, "PLAY / STOP");
    lv_obj_center(label_btn);

    // --- 4. TEXTES (COMPTEUR ET SCORE) ---
    label_compteur = lv_label_create(lv_screen_active());
    lv_label_set_text(label_compteur, "Degres : 0");
    lv_obj_set_style_bg_color(label_compteur, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(label_compteur, 0, 0); // OPACITÉ À ZÉRO ICI
    lv_obj_set_style_text_color(label_compteur, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(label_compteur, LV_ALIGN_BOTTOM_MID, 0, -20);

    label_score = lv_label_create(lv_screen_active());
    lv_label_set_text(label_score, "SCORE: 0");
    lv_obj_set_style_bg_color(label_score, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(label_score, 0, 0); // OPACITÉ À ZÉRO ICI
    lv_obj_set_style_text_color(label_score, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(label_score, LV_ALIGN_TOP_MID, 0, 20); 

    initialiserPages();
}

#ifdef ARDUINO

void mySetup() {
    Serial.begin(115200);
    
    delay(500); 
    
    pinMode(D0, INPUT_PULLUP);
    pinMode(D1, INPUT_PULLUP);
    pinMode(pinPWM, OUTPUT);
    pinMode(pinSens, OUTPUT);
    digitalWrite(pinPWM, LOW);

    pin_function(PC_6, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3));
    pin_function(PC_7, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3));
    
    __HAL_RCC_TIM3_CLK_ENABLE(); 
    TIM3->SMCR |= TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1;      
    TIM3->CCMR1 |= TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0; 
    TIM3->CR1 |= TIM_CR1_CEN;    

    randomSeed(TIM3->CNT + analogRead(A0)); 
    
    testLvgl();

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

    // --- GESTION DU RETOUR AU NOIR APRES LE FLASH ---
    if (ecran_en_flash && (millis() - temps_flash >= 300)) {
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), 0); // Remet le fond noir
        ecran_en_flash = false;
    }

    static bool ancienEtatMoteur = false;

    // --- 2. LOGIQUE DU JEU EN TEMPS RÉEL (ZÉRO DÉLAI INFORMATIQUE) ---
    if (ancienEtatMoteur == true && moteurEnMarche == false) {
        
        // On coupe le moteur matériellement à l'instant T
        analogWrite(pinPWM, 0);
        
        long valeur_angle = TIM3->CNT;
        float angle_degres = -(((float)valeur_angle * 360.0f) / (float)TICKS_PAR_TOUR);
        int angle_entier = (int)angle_degres % 360;
        if (angle_entier < 0) angle_entier += 360;

        int diff = abs(angle_entier - centre_zone);
        if (diff > 180) diff = 360 - diff;

        if (diff <= (taille_zone / 2)) {
            // VICTOIRE
            score++;
            taille_zone -= 10; 
            if (taille_zone < 15) taille_zone = 15; 
            
            lv_obj_set_style_text_color(label_score, lv_color_hex(0x00FF00), 0); 
            
            // DÉCLENCHE LE FLASH VERT
            lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x00FF00), 0); 
            temps_flash = millis();
            ecran_en_flash = true;

            Serial.printf("GAGNE ! Score: %d | Zone restante: %d degres\n", score, taille_zone);
        } else {
            // DÉFAITE
            score = 0;
            taille_zone = 90; 
            
            lv_obj_set_style_text_color(label_score, lv_color_hex(0xFF0000), 0); 
            
            // DÉCLENCHE LE FLASH ROUGE
            lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xFF0000), 0); 
            temps_flash = millis();
            ecran_en_flash = true;

            Serial.println("PERDU ! Remise a zero.");
        }

        lv_label_set_text_fmt(label_score, "SCORE: %d", score);
        genererNouvelleZone();
    } 
    else if (ancienEtatMoteur == false && moteurEnMarche == true) {
        lv_obj_set_style_text_color(label_score, lv_color_hex(0xFFFFFF), 0);
    }

    ancienEtatMoteur = moteurEnMarche;

    // --- 3. TON BLOC MOTEUR (Gestion continue) ---
    if (moteurEnMarche) {
        digitalWrite(pinSens, HIGH);
        analogWrite(pinPWM, 195); 
        
        long valeur_angle = TIM3->CNT; 
        
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
        // Sécurité continue pour l'arrêt
        analogWrite(pinPWM, 0);
    }

    // --- 4. DIAGNOSTIC COMPLET (Toutes les 100ms) ---
    static unsigned long tempsPrecedent = 0;
    if (millis() - tempsPrecedent >= 100) {
        tempsPrecedent = millis();
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