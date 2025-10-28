#include <stdio.h>
#include <stdbool.h>

struct Coordenada {
    int x;
    int y;
};

struct Heroe {
    int vida;
    int attack_dmg;
    int attack_range;
    struct Coordenada pos_inicial;
    struct Coordenada* ruta_meta;
    int num_pasos_ruta;
    int paso_actual;
};

struct Monstruo {
    int vida;
    int attack_dmg;
    int attack_range;
    struct Coordenada pos_inicial;
    bool activo;
    struct Coordenada* objetivo;
};