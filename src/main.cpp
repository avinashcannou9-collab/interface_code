
#include <Arduino.h>
#include "lvgl.h"
#include "lvglDrivers.h"
#include <STM32encoder.h>
#include <HardwareTimer.h>
#include <math.h>

//  BROCHES MATÉRIELLES
const int pinPWM         = 6;
const int pinSens        = 7;
const int pinCapteurA0   = A0;   // CNY70 — lecture analogique
const long TICKS_PAR_TOUR = 703;


//  ENCODEUR
STM32encoder myEnc(TIM3, 0, 65535);

//  ÉTAT GLOBAL
typedef enum { ECRAN_MENU, ECRAN_JEU, ECRAN_CHRONO } EcranActif;
static EcranActif ecranActuel = ECRAN_MENU;

volatile bool moteurEnMarche = false;

//  COULEURS 
#define COL_BG      lv_color_hex(0x0D0F1A)
#define COL_CARD    lv_color_hex(0x1B1D36)
#define COL_GOLD    lv_color_hex(0xF69E53)
#define COL_RED     lv_color_hex(0xD72A28)
#define COL_GREEN   lv_color_hex(0x00C44F)
#define COL_WHITE   lv_color_hex(0xE8E8F0)
#define COL_GREY    lv_color_hex(0x3A3C55)

//  VARIABLES JEU QTE
lv_obj_t * roue_jeu           = NULL;
lv_obj_t * aiguille_jeu       = NULL;
lv_obj_t * btn_play_jeu       = NULL;
lv_obj_t * arc_zone_jeu       = NULL;
lv_obj_t * label_score_jeu    = NULL;

int score        = 0;
int taille_zone  = 90;
int centre_zone  = 0;

unsigned long temps_flash  = 0;
bool ecran_en_flash        = false;

static lv_point_precise_t points_aiguille[2];


//  VARIABLES CHRONOMÈTRE
lv_obj_t * roue_chrono        = NULL;
lv_obj_t * aiguille_chrono    = NULL;
lv_obj_t * label_chrono       = NULL;
lv_obj_t * label_statut_chrono= NULL;
lv_obj_t * btn_start_chrono   = NULL;
lv_obj_t * btn_reset_chrono   = NULL;
lv_obj_t * arc_duree_chrono   = NULL;

unsigned long duree_chrono_ms  = 60000UL;  
bool chrono_en_cours           = false;
bool chrono_calibre            = false;    
bool chrono_homing             = false;    
unsigned long chrono_debut_ms  = 0;
static lv_point_precise_t pts_aiguille_chrono[2];

//  DÉCLARATIONS DES FONCTIONS
void afficherMenu();
void afficherJeu();
void afficherChrono();
void genererNouvelleZone();
static void event_handler_jeu(lv_event_t * e);
static void event_handler_menu_jeu(lv_event_t * e);
static void event_handler_menu_chrono(lv_event_t * e);
static void event_handler_retour(lv_event_t * e);
static void event_handler_start_chrono(lv_event_t * e);
static void event_handler_reset_chrono(lv_event_t * e);
static void event_handler_duree_plus(lv_event_t * e);
static void event_handler_duree_moins(lv_event_t * e);


// bouton 
lv_obj_t * creerBouton(lv_obj_t * parent, const char * texte, lv_color_t couleur, int w, int h)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_bg_color(btn, couleur, 0);
    lv_obj_set_style_bg_color(btn, lv_color_darken(couleur, 40), LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_t * lbl = lv_label_create(btn);
    lv_label_set_text(lbl, texte);
    lv_obj_set_style_text_color(lbl, COL_WHITE, 0);
    lv_obj_center(lbl);
    return btn;
}


//  ÉCRAN MENU PRINCIPAL
void afficherMenu()
{
    ecranActuel = ECRAN_MENU;
    
    // Nettoie l'écran et met une couleur de fond
    lv_obj_clean(lv_screen_active());
    lv_obj_set_style_bg_color(lv_screen_active(), COL_BG, 0);

    // Titre de la page
    lv_obj_t * titre = lv_label_create(lv_screen_active());
    lv_label_set_text(titre, "MOTEUR CONTROL");
    lv_obj_set_style_text_color(titre, COL_GOLD, 0);
    lv_obj_align(titre, LV_ALIGN_TOP_MID, 0, 14);

    // Sous-titre
    lv_obj_t * sous_titre = lv_label_create(lv_screen_active());
    lv_label_set_text(sous_titre, "Choisir un mode");
    lv_obj_set_style_text_color(sous_titre, COL_GREY, 0);
    lv_obj_align(sous_titre, LV_ALIGN_TOP_MID, 0, 40);

    //JEU QTE
    lv_obj_t * carte_jeu = lv_obj_create(lv_screen_active());
    lv_obj_set_size(carte_jeu, 185, 155);
    lv_obj_align(carte_jeu, LV_ALIGN_LEFT_MID, 12, 10);
    lv_obj_set_style_bg_color(carte_jeu, COL_CARD, 0);
    lv_obj_set_style_border_color(carte_jeu, COL_RED, 0);
    lv_obj_set_style_border_width(carte_jeu, 3, 0);
    lv_obj_clear_flag(carte_jeu, LV_OBJ_FLAG_SCROLLABLE);

    // Image décorative QTE
    lv_obj_t * lbl_jeu = lv_label_create(carte_jeu);
    lv_label_set_text(lbl_jeu, "JEU QTE");
    lv_obj_set_style_text_color(lbl_jeu, COL_WHITE, 0);
    lv_obj_align(lbl_jeu, LV_ALIGN_BOTTOM_MID, 0, -40);

    // Bouton pour entrer dans le jeu
    lv_obj_t * btn_jeu = creerBouton(carte_jeu, "JOUER", COL_RED, 140, 32);
    lv_obj_align(btn_jeu, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_add_event_cb(btn_jeu, event_handler_menu_jeu, LV_EVENT_CLICKED, NULL);

    // CHRONOMÈTRE
    lv_obj_t * carte_chrono = lv_obj_create(lv_screen_active());
    lv_obj_set_size(carte_chrono, 185, 155);
    lv_obj_align(carte_chrono, LV_ALIGN_RIGHT_MID, -12, 10);
    lv_obj_set_style_bg_color(carte_chrono, COL_CARD, 0);
    lv_obj_set_style_border_color(carte_chrono, COL_GOLD, 0);
    lv_obj_set_style_border_width(carte_chrono, 3, 0);
    lv_obj_clear_flag(carte_chrono, LV_OBJ_FLAG_SCROLLABLE);

    // Image décorative Chrono
    lv_obj_t * lbl_c = lv_label_create(carte_chrono);
    lv_label_set_text(lbl_c, "CHRONO");
    lv_obj_set_style_text_color(lbl_c, COL_WHITE, 0);
    lv_obj_align(lbl_c, LV_ALIGN_BOTTOM_MID, 0, -40);

    // Bouton pour entrer dans le chrono
    lv_obj_t * btn_chrono = creerBouton(carte_chrono, "OUVRIR", COL_GOLD, 140, 32);
    lv_obj_align(btn_chrono, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_text_color(lv_obj_get_child(btn_chrono, 0), COL_BG, 0);
    lv_obj_add_event_cb(btn_chrono, event_handler_menu_chrono, LV_EVENT_CLICKED, NULL);
}

//  ÉCRAN JEU QTE

// Calcule au hasard la position de la zone verte
void genererNouvelleZone()
{
    centre_zone = random(0, 360);
    int arc_debut = (centre_zone - (taille_zone / 2) + 270) % 360;
    int arc_fin   = (centre_zone + (taille_zone / 2) + 270) % 360;
    if (arc_debut < 0) arc_debut += 360;
    if (arc_fin   < 0) arc_fin   += 360;
    lv_arc_set_angles(arc_zone_jeu, arc_debut, arc_fin);
}

// Action quand on appuie sur le bouton PLAY du jeu
static void event_handler_jeu(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);
    
    // Si l'état du bouton change (enfoncé / relâché)
    if (code == LV_EVENT_VALUE_CHANGED) {
        moteurEnMarche = lv_obj_has_state(obj, LV_STATE_CHECKED);
        
        // --- PRINTF POUR VÉRIFIER LE MOTEUR ---
        if(moteurEnMarche) {
            Serial.println(">>> MOTEUR EN MARCHE ! <<<");
        } else {
            Serial.println(">>> MOTEUR A L'ARRET ! <<<");
        }
    }
}

void afficherJeu()
{
    ecranActuel = ECRAN_JEU;
    score       = 0;
    taille_zone = 90;

    // Reset l'écran
    lv_obj_clean(lv_screen_active());
    lv_obj_set_style_bg_color(lv_screen_active(), COL_BG, 0);

    // Bouton de retour au menu
    lv_obj_t * btn_retour = creerBouton(lv_screen_active(), "< Menu", COL_GREY, 80, 32);
    lv_obj_align(btn_retour, LV_ALIGN_TOP_LEFT, 8, 8);
    lv_obj_add_event_cb(btn_retour, event_handler_retour, LV_EVENT_CLICKED, NULL);

    // Cercle de fond du jeu (La roue)
    roue_jeu = lv_obj_create(lv_screen_active());
    lv_obj_set_size(roue_jeu, 200, 200);
    lv_obj_align(roue_jeu, LV_ALIGN_LEFT_MID, 25, 0);
    lv_obj_set_style_radius(roue_jeu, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(roue_jeu, COL_CARD, 0);
    lv_obj_set_style_border_width(roue_jeu, 8, 0);
    lv_obj_set_style_border_color(roue_jeu, COL_GOLD, 0);
    lv_obj_clear_flag(roue_jeu, LV_OBJ_FLAG_SCROLLABLE);

    // Arc de cercle vert (La zone cible)
    arc_zone_jeu = lv_arc_create(roue_jeu);
    lv_obj_set_size(arc_zone_jeu, 200, 200);
    lv_obj_center(arc_zone_jeu);
    lv_obj_remove_style(arc_zone_jeu, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_zone_jeu, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc_zone_jeu, 0, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_zone_jeu, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_zone_jeu, COL_GREEN, LV_PART_INDICATOR);
    genererNouvelleZone();

    // L'aiguille rouge qui tourne
    aiguille_jeu = lv_line_create(roue_jeu);
    lv_obj_clear_flag(aiguille_jeu, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_line_width(aiguille_jeu, 6, 0);
    lv_obj_set_style_line_color(aiguille_jeu, COL_RED, 0);
    lv_obj_set_style_line_rounded(aiguille_jeu, true, 0);
    points_aiguille[0] = {80, 80};
    points_aiguille[1] = {80, 80};
    lv_line_set_points_mutable(aiguille_jeu, points_aiguille, 2);

    // Le gros bouton PLAY/STOP à droite
    btn_play_jeu = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn_play_jeu, event_handler_jeu, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_size(btn_play_jeu, 110, 55);
    lv_obj_align(btn_play_jeu, LV_ALIGN_RIGHT_MID, -50, 0);
    lv_obj_add_flag(btn_play_jeu, LV_OBJ_FLAG_CHECKABLE); // Bouton à bascule (on/off)
    lv_obj_set_style_bg_color(btn_play_jeu, COL_RED, 0);
    lv_obj_set_style_radius(btn_play_jeu, 10, 0);
    
    // Texte dans le bouton play
    lv_obj_t * lbl_btn = lv_label_create(btn_play_jeu);
    lv_label_set_text(lbl_btn, "PLAY\nSTOP");
    lv_obj_center(lbl_btn);
    lv_obj_set_style_text_color(lbl_btn, COL_WHITE, 0);

    // Affichage texte du SCORE
    label_score_jeu = lv_label_create(lv_screen_active());
    lv_label_set_text(label_score_jeu, "SCORE: 0");
    lv_obj_set_style_text_color(label_score_jeu, COL_WHITE, 0);
    lv_obj_align(label_score_jeu, LV_ALIGN_TOP_MID, 0, 14);
}

// ============================================================
//  ÉCRAN CHRONOMÈTRE
// ============================================================

// Met à jour le texte qui affiche le temps cible (ex: 01:00)
void mettreAJourLabelChrono()
{
    unsigned long total_sec  = duree_chrono_ms / 1000UL;
    unsigned int  minutes    = total_sec / 60;
    unsigned int  secondes   = total_sec % 60;
    lv_label_set_text_fmt(label_chrono, "%02d:%02d", minutes, secondes);
}

void afficherChrono()
{
    ecranActuel       = ECRAN_CHRONO;
    chrono_en_cours   = false;
    chrono_calibre    = false;
    chrono_homing     = false;
    duree_chrono_ms   = 60000UL;

    lv_obj_clean(lv_screen_active());
    lv_obj_set_style_bg_color(lv_screen_active(), COL_BG, 0);

    // Bouton de retour
    lv_obj_t * btn_retour = creerBouton(lv_screen_active(), "< Menu", COL_GREY, 80, 32);
    lv_obj_align(btn_retour, LV_ALIGN_TOP_LEFT, 8, 8);
    lv_obj_add_event_cb(btn_retour, event_handler_retour, LV_EVENT_CLICKED, NULL);

    // Titre de la page chrono
    lv_obj_t * titre = lv_label_create(lv_screen_active());
    lv_label_set_text(titre, "CHRONOMETRE / 1 TOUR");
    lv_obj_set_style_text_color(titre, COL_GOLD, 0);
    lv_obj_align(titre, LV_ALIGN_TOP_MID, 0, 14);

    // Cercle de fond
    roue_chrono = lv_obj_create(lv_screen_active());
    lv_obj_set_size(roue_chrono, 200, 200);
    lv_obj_align(roue_chrono, LV_ALIGN_LEFT_MID, 25, 5);
    lv_obj_set_style_radius(roue_chrono, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(roue_chrono, COL_CARD, 0);
    lv_obj_set_style_border_width(roue_chrono, 8, 0);
    lv_obj_set_style_border_color(roue_chrono, COL_GOLD, 0);
    lv_obj_clear_flag(roue_chrono, LV_OBJ_FLAG_SCROLLABLE);

    // Arc jaune qui indique le temps écoulé
    arc_duree_chrono = lv_arc_create(roue_chrono);
    lv_obj_set_size(arc_duree_chrono, 200, 200);
    lv_obj_center(arc_duree_chrono);
    lv_obj_remove_style(arc_duree_chrono, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_duree_chrono, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc_duree_chrono, 0, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_duree_chrono, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_duree_chrono, COL_GOLD, LV_PART_INDICATOR);
    lv_arc_set_angles(arc_duree_chrono, 270, 270);

    // Aiguille jaune du chrono
    aiguille_chrono = lv_line_create(roue_chrono);
    lv_obj_set_style_line_width(aiguille_chrono, 5, 0);
    lv_obj_set_style_line_color(aiguille_chrono, COL_GOLD, 0);
    lv_obj_set_style_line_rounded(aiguille_chrono, true, 0);
    pts_aiguille_chrono[0] = {80, 80};
    pts_aiguille_chrono[1] = {80, 8}; 
    lv_line_set_points_mutable(aiguille_chrono, pts_aiguille_chrono, 2);

    // Label pour afficher le temps configuré
    label_chrono = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_color(label_chrono, COL_WHITE, 0);
    lv_obj_align(label_chrono, LV_ALIGN_TOP_RIGHT, -60, 68);
    mettreAJourLabelChrono();

    // Boutons Plus et Moins
    lv_obj_t * btn_plus = creerBouton(lv_screen_active(), "+30s", COL_GREY, 60, 30);
    lv_obj_align(btn_plus, LV_ALIGN_TOP_RIGHT, -25, 96);
    lv_obj_add_event_cb(btn_plus, event_handler_duree_plus, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_moins = creerBouton(lv_screen_active(), "-30s", COL_GREY, 60, 30);
    lv_obj_align(btn_moins, LV_ALIGN_TOP_RIGHT, -100, 96);
    lv_obj_add_event_cb(btn_moins, event_handler_duree_moins, LV_EVENT_CLICKED, NULL);

    // Label du statut (En attente, Zéro OK...)
    label_statut_chrono = lv_label_create(lv_screen_active());
    lv_label_set_text(label_statut_chrono, "En attente...");
    lv_obj_set_style_text_color(label_statut_chrono, COL_GREY, 0);
    lv_obj_align(label_statut_chrono, LV_ALIGN_RIGHT_MID, -20, -20);

    // Bouton START
    btn_start_chrono = creerBouton(lv_screen_active(), "START", COL_GOLD, 120, 42);
    lv_obj_align(btn_start_chrono, LV_ALIGN_RIGHT_MID, -50, 25);
    lv_obj_set_style_text_color(lv_obj_get_child(btn_start_chrono, 0), COL_BG, 0);
    lv_obj_add_event_cb(btn_start_chrono, event_handler_start_chrono, LV_EVENT_CLICKED, NULL);

    // Bouton de Homing (Recherche point Zéro)
    btn_reset_chrono = creerBouton(lv_screen_active(), "ZERO / RESET", COL_GREY, 110, 32);
    lv_obj_align(btn_reset_chrono, LV_ALIGN_RIGHT_MID, -40, 72);
    lv_obj_add_event_cb(btn_reset_chrono, event_handler_reset_chrono, LV_EVENT_CLICKED, NULL);
}

// ============================================================
//  ACTIONS DES BOUTONS DE NAVIGATION
// ============================================================
static void event_handler_menu_jeu(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        moteurEnMarche = false;
        afficherJeu(); // Ouvre la page Jeu
    }
}

static void event_handler_menu_chrono(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        moteurEnMarche = false;
        afficherChrono(); // Ouvre la page Chrono
    }
}

static void event_handler_retour(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        moteurEnMarche  = false;
        chrono_en_cours = false;
        chrono_homing   = false;
        analogWrite(pinPWM, 0); // Sécurité : coupe le moteur
        afficherMenu(); // Retourne à l'accueil
    }
}

// ============================================================
//  ACTIONS DES BOUTONS DU CHRONO
// ============================================================
static void event_handler_start_chrono(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    
    // On empêche de lancer si la machine est en train de chercher son zéro
    if (chrono_homing) return; 

    // Lance le chrono
    if (!chrono_en_cours) {
        TIM3->CNT = 0; // Remise à zero parfaite de l'encodeur
        chrono_debut_ms   = millis();
        chrono_en_cours   = true;
        lv_label_set_text(lv_obj_get_child(btn_start_chrono, 0), "STOP");
        lv_obj_set_style_bg_color(btn_start_chrono, COL_RED, 0);
        lv_label_set_text(label_statut_chrono, "En cours...");
        lv_obj_set_style_text_color(label_statut_chrono, COL_GREEN, 0);
    } 
    // Arrête le chrono manuellement
    else {
        chrono_en_cours = false;
        analogWrite(pinPWM, 0);
        lv_label_set_text(lv_obj_get_child(btn_start_chrono, 0), "START");
        lv_obj_set_style_bg_color(btn_start_chrono, COL_GOLD, 0);
        lv_label_set_text(label_statut_chrono, "Arrete.");
        lv_obj_set_style_text_color(label_statut_chrono, COL_GREY, 0);
    }
}

static void event_handler_reset_chrono(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    
    // On lance la procédure de HOMING (Moteur tourne doucement jusqu'à la ligne blanche)
    chrono_en_cours = false;
    chrono_calibre  = false;
    chrono_homing   = true;
    
    // Remise à zéro visuelle (Graphismes)
    lv_arc_set_angles(arc_duree_chrono, 270, 270);
    pts_aiguille_chrono[0] = {80, 80};
    pts_aiguille_chrono[1] = {80, 8};
    lv_line_set_points_mutable(aiguille_chrono, pts_aiguille_chrono, 2);
    lv_label_set_text(lv_obj_get_child(btn_start_chrono, 0), "START");
    lv_obj_set_style_bg_color(btn_start_chrono, COL_GOLD, 0);
    
    lv_label_set_text(label_statut_chrono, "Recherche point 0...");
    lv_obj_set_style_text_color(label_statut_chrono, COL_GOLD, 0);
}

static void event_handler_duree_plus(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && !chrono_en_cours && !chrono_homing) {
        duree_chrono_ms += 30000UL;
        if (duree_chrono_ms > 600000UL) duree_chrono_ms = 600000UL; // max 10 min
        mettreAJourLabelChrono();
    }
}

static void event_handler_duree_moins(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && !chrono_en_cours && !chrono_homing) {
        if (duree_chrono_ms > 30000UL) duree_chrono_ms -= 30000UL;
        mettreAJourLabelChrono();
    }
}


// ============================================================
//  SETUP INITIALISATION DU CODE
// ============================================================
void mySetup()
{
    Serial.begin(115200);
    delay(500);

    // Initialisation des broches matérielles
    pinMode(pinCapteurA0, INPUT);
    pinMode(pinPWM,  OUTPUT);
    pinMode(pinSens, OUTPUT);
    digitalWrite(pinPWM, LOW);

    // Activation matérielle de l'encodeur sur les broches PC6 et PC7
    pin_function(PC_6, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3));
    pin_function(PC_7, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF2_TIM3));

    __HAL_RCC_TIM3_CLK_ENABLE();
    TIM3->SMCR |= TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1;
    TIM3->CCMR1 |= TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0;
    TIM3->CR1   |= TIM_CR1_CEN;

    randomSeed(TIM3->CNT + analogRead(A0));

    // Affiche le premier écran au démarrage
    afficherMenu();
}

// ============================================================
//  BOUCLE PRINCIPALE EN CONTINU
// ============================================================
void loop()
{
    // C'est ce qui fait tourner les animations de l'écran LVGL
    lv_timer_handler();

    // ============================================================
    //  LOGIQUE : SI ON EST DANS LE JEU QTE
    // ============================================================
    if (ecranActuel == ECRAN_JEU)
    {
        // Enlève le flash de victoire/défaite après un délai
        if (ecran_en_flash && (millis() - temps_flash >= 300)) {
            lv_obj_set_style_bg_color(lv_screen_active(), COL_BG, 0);
            ecran_en_flash = false;
        }

        static bool ancienEtatMoteur = false;

        // VÉRIFICATION DU JEU : Si on vient tout juste d'appuyer sur STOP
        if (ancienEtatMoteur == true && moteurEnMarche == false)
        {
            // 1. On coupe l'alimentation du moteur
            analogWrite(pinPWM, 0);
            
            // 2. On lit la valeur de l'encodeur
            long  valeur_angle = TIM3->CNT;
            
            // 3. On convertit cette valeur en degrés
            float angle_degres = -(((float)valeur_angle * 360.0f) / (float)TICKS_PAR_TOUR);
            int   angle_entier = (int)angle_degres % 360;
            if (angle_entier < 0) angle_entier += 360;

            // 4. On calcule l'écart avec la zone cible verte
            int diff = abs(angle_entier - centre_zone);
            if (diff > 180) diff = 360 - diff;

            // 5. Conditions de Victoire ou Défaite
            if (diff <= (taille_zone / 2)) {
                // VICITOIRE : le joueur a arrêté dans la zone verte
                score++;
                taille_zone -= 10;
                if (taille_zone < 15) taille_zone = 15; // Limite de difficulté
                lv_obj_set_style_text_color(label_score_jeu, COL_GREEN, 0);
                lv_obj_set_style_bg_color(lv_screen_active(), COL_GREEN, 0); // Flash écran Vert
                temps_flash    = millis();
                ecran_en_flash = true;
                Serial.printf("GAGNE ! Score: %d | Zone: %d deg\n", score, taille_zone);
            } else {
                // DÉFAITE : le joueur a raté la zone
                score       = 0;
                taille_zone = 90;
                lv_obj_set_style_text_color(label_score_jeu, COL_RED, 0);
                lv_obj_set_style_bg_color(lv_screen_active(), COL_RED, 0); // Flash écran Rouge
                temps_flash    = millis();
                ecran_en_flash = true;
                Serial.println("PERDU ! Remise a zero.");
            }
            
            // 6. Mise à jour de l'interface après coup
            lv_label_set_text_fmt(label_score_jeu, "SCORE: %d", score);
            genererNouvelleZone(); // Prépare le prochain tour
        }
        else if (ancienEtatMoteur == false && moteurEnMarche == true) {
            lv_obj_set_style_text_color(label_score_jeu, COL_WHITE, 0);
        }

        // Mémorisation de l'état pour détecter les clics la prochaine fois
        ancienEtatMoteur = moteurEnMarche;

        // SI LE JEU EST EN COURS : On fait tourner le moteur et l'aiguille
        if (moteurEnMarche) {
            digitalWrite(pinSens, HIGH);
            analogWrite(pinPWM, 195); // Vitesse du moteur

            // Lecture de l'encodeur pour positionner l'aiguille
            long  valeur_angle = TIM3->CNT;
            float angle_degres = -(((float)valeur_angle * 360.0f) / (float)TICKS_PAR_TOUR);
            int   angle_entier = (int)angle_degres % 360;
            if (angle_entier < 0) angle_entier += 360;

            // Calcul trigonométrique (SOH CAH TOA) pour les coordonnées (X,Y) de l'aiguille
            float angle_radians = (angle_degres - 90.0f) * (M_PI / 180.0f);
            lv_value_precise_t nouv_x = (lv_value_precise_t)(80.0f + 80.0f * cosf(angle_radians));
            lv_value_precise_t nouv_y = (lv_value_precise_t)(80.0f + 80.0f * sinf(angle_radians));

            // Déplacement graphique de l'aiguille
            if (aiguille_jeu) {
                points_aiguille[0] = {80, 80}; // Point central
                points_aiguille[1] = {nouv_x, nouv_y}; // Point extérieur calculé
                lv_line_set_points_mutable(aiguille_jeu, points_aiguille, 2);
            }
        } else {
            analogWrite(pinPWM, 0);
        }
    }

//    LE CHRONOMÈTRE
    else if (ecranActuel == ECRAN_CHRONO)
    {
        //1. RECHERCHE DU POINT ZERO
        if (chrono_homing) {
            digitalWrite(pinSens, HIGH);
            analogWrite(pinPWM, 130); // Vitesse modérée pour chercher la ligne blanche

            int valCNY = analogRead(pinCapteurA0);
            if (valCNY > 800) { // Si le capteur CNY70 détecte (seuil > 800)
                analogWrite(pinPWM, 0); // Coupe le moteur immédiatement
                TIM3->CNT = 0; // Réinitialisation de l'encodeur à la position 0
                chrono_homing  = false;
                chrono_calibre = true;
                
                lv_label_set_text(label_statut_chrono, "Zero OK ! Pret.");
                lv_obj_set_style_text_color(label_statut_chrono, COL_GREEN, 0);
                Serial.println(">>> POINT 0 TROUVE ET CALIBRE ! <<<");
            }
        }
        
        // --- 2. CHRONOMÈTRE ET ASSERVISSEMENT : 1 TOUR EXACT 
        else if (chrono_en_cours) {
            // Calcul du temps qui s'est écoulé
            unsigned long temps_ecoule = millis() - chrono_debut_ms;
            float progression = (float)temps_ecoule / (float)duree_chrono_ms; // ex: 0.5 si on est à la moitié
            if (progression > 1.0f) progression = 1.0f;

            // RÉGULATEUR PROPORTIONNEL : Calcule où le moteur DOIT être
            long ticks_bruts = TIM3->CNT;
            if (ticks_bruts > 32768) ticks_bruts = 65536 - ticks_bruts; // Anti-débordement
            long ticks_actuels = abs(ticks_bruts); // Position réelle

            long ticks_cibles = (long)(progression * TICKS_PAR_TOUR); // Position idéale
            long erreur = ticks_cibles - ticks_actuels; // Le retard ou l'avance

            int pwm_consigne = 0;
            int PWM_MINIMAL = 110; // Tension minimum pour que le moteur bouge

            if (erreur > 0) {
                // S'il est en retard, on envoie le PWM minimal + on accélère selon l'erreur
                pwm_consigne = PWM_MINIMAL + (erreur * 2);
            } else {
                // S'il est en avance ou parfait, il freine
                pwm_consigne = 0;
            }

            // Sécurité pour ne pas brûler la carte (max 255)
            if (pwm_consigne > 255) pwm_consigne = 255;

            // Envoi de la vitesse ajustée au moteur
            digitalWrite(pinSens, HIGH);
            analogWrite(pinPWM, pwm_consigne);

            // Aiguille animée graphiquement
            float angle_degres  = progression * 360.0f;
            float angle_radians = (angle_degres - 90.0f) * (M_PI / 180.0f);

            lv_value_precise_t x2 = (lv_value_precise_t)(80.0f + 72.0f * cosf(angle_radians));
            lv_value_precise_t y2 = (lv_value_precise_t)(80.0f + 72.0f * sinf(angle_radians));

            pts_aiguille_chrono[0] = {80, 80};
            pts_aiguille_chrono[1] = {x2, y2};
            lv_line_set_points_mutable(aiguille_chrono, pts_aiguille_chrono, 2);

            // Arc de progression (s'allume de 0 à 360 degrés)
            int arc_fin = (270 + (int)angle_degres) % 360;
            lv_arc_set_angles(arc_duree_chrono, 270, arc_fin);

            // Temps restant affiché en texte
            unsigned long restant_ms  = (temps_ecoule < duree_chrono_ms) ? (duree_chrono_ms - temps_ecoule) : 0;
            unsigned int  min_rest    = restant_ms / 60000UL;
            unsigned int  sec_rest    = (restant_ms % 60000UL) / 1000UL;
            lv_label_set_text_fmt(label_chrono, "%02d:%02d", min_rest, sec_rest);

            // FIN DU CHRONOMÈTRE
            if (temps_ecoule >= duree_chrono_ms) {
                chrono_en_cours = false;
                analogWrite(pinPWM, 0); // Moteur stoppé
                lv_label_set_text(lv_obj_get_child(btn_start_chrono, 0), "START");
                lv_obj_set_style_bg_color(btn_start_chrono, COL_GOLD, 0);
                lv_label_set_text(label_statut_chrono, "Termine ! 1 Tour effectue.");
                lv_obj_set_style_text_color(label_statut_chrono, COL_GOLD, 0);
                
                // Flash visuel doré
                lv_obj_set_style_bg_color(lv_screen_active(), COL_GOLD, 0);
                temps_flash    = millis();
                ecran_en_flash = true;
                Serial.println("CHRONO TERMINE ! Moteur stope.");
            }
        }
        else {
            // Sécurité : Moteur coupé si rien n'est lancé
            analogWrite(pinPWM, 0);
        }

        // Retour fond noir après le flash doré
        if (ecran_en_flash && (millis() - temps_flash >= 400)) {
            lv_obj_set_style_bg_color(lv_screen_active(), COL_BG, 0);
            ecran_en_flash = false;
        }
    }
}

#ifdef ARDUINO

void myTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
    }
}

#else
#include "app_hal.h"
int main(void) {
    lv_init();
    hal_setup();
    afficherMenu();
    hal_loop();
    return 0;
}
#endif