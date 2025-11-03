#include <stdio.h>
#include <stdbool.h> // Incluido por si lo usas más adelante
#include <string.h>  // Para strncmp, strstr
#include <stdlib.h>  // Para malloc, free, exit

#define MAX_LINE_LENGTH 256
#define MAX_PATH_STEPS 100

// Molde para una coordenada 2D
struct Coordenada {
    int x;
    int y;
};

// Molde para el Héroe
struct Heroe {
    int vida;
    int attack_dmg;
    int attack_range;
    struct Coordenada pos_inicial;
    struct Coordenada ruta_meta[MAX_PATH_STEPS]; // Aún no se parsea
    int num_pasos_ruta;
};

// Molde para un Monstruo
struct Monstruo {
    int vida;
    int attack_dmg;
    int attack_range;
    int vision_range;
    struct Coordenada pos_inicial;
};

// Estructura para guardar TODA la simulación
struct Simulacion {
    int grid_x;
    int grid_y;
    struct Heroe heroe;
    struct Monstruo* monstruos;
    int num_monstruos;
};

int main() {
    struct Simulacion sim;
    sim.num_monstruos = 0;
    sim.monstruos = NULL; // FIX: Inicializar puntero a NULL

    FILE* fp = fopen("CONFIG.txt", "r");
    if (fp == NULL) {
        perror("Error al abrir CONFIG.txt");
        return 1;
    }

    char linea[MAX_LINE_LENGTH];

    while (fgets(linea, sizeof(linea), fp) != NULL) {
        
        if (strncmp(linea, "GRID_SIZE", 9) == 0) {
            sscanf(linea, "GRID_SIZE %d %d", &sim.grid_x, &sim.grid_y);
        } 
        else if (strncmp(linea, "HERO_HP", 7) == 0) {
            sscanf(linea, "HERO_HP %d", &sim.heroe.vida);
        }
        else if (strncmp(linea, "HERO_ATTACK_DAMAGE", 18) == 0) {
            sscanf(linea, "HERO_ATTACK_DAMAGE %d", &sim.heroe.attack_dmg);
        }
        else if (strncmp(linea, "HERO_ATTACK_RANGE", 17) == 0) {
            sscanf(linea, "HERO_ATTACK_RANGE %d", &sim.heroe.attack_range);
        }
        else if (strncmp(linea, "HERO_START", 10) == 0) {
            // FIX: Añadir formato de paréntesis y coma
            sscanf(linea, "HERO_START (%d,%d)", &sim.heroe.pos_inicial.x, &sim.heroe.pos_inicial.y);
        }
        else if (strncmp(linea, "MONSTER_COUNT", 13) == 0) {
            sscanf(linea, "MONSTER_COUNT %d", &sim.num_monstruos);
            
            sim.monstruos = malloc(sim.num_monstruos * sizeof(struct Monstruo));

            // FIX: Comprobar si malloc falló
            if (sim.monstruos == NULL) {
                perror("Error: Fallo al reservar memoria para monstruos");
                fclose(fp);
                return 1;
            }
        }
        // ===================================================================
        // FIX: Lógica de parseo de monstruos corregida para evitar Segfault
        // ===================================================================
        else if (strncmp(linea, "MONSTER_", 8) == 0) {
            
            // 1. Comprobar si la memoria ya fue asignada
            if (sim.monstruos == NULL) {
                printf("Error: Se encontró una estadística de monstruo antes de MONSTER_COUNT.\n");
                continue;
            }

            // 2. Leer el ID de forma segura primero
            int monster_id;
            if (sscanf(linea, "MONSTER_%d_", &monster_id) != 1) {
                continue; // La línea no tiene un ID de monstruo válido
            }

            // 3. Validar el ID
            if (monster_id < 1 || monster_id > sim.num_monstruos) {
                printf("Error: Monstruo con ID %d fuera de rango.\n", monster_id);
                continue; // ID no válido
            }

            // 4. Crear índice base-0
            int monster_idx = monster_id - 1;

            // 5. Buscar el stat y asignar el valor
            // (%*d ignora el ID que ya leímos)

            if (strstr(linea, "_HP") != NULL) {
                sscanf(linea, "MONSTER_%*d_HP %d", &sim.monstruos[monster_idx].vida);
            }
            else if (strstr(linea, "_ATTACK_DAMAGE") != NULL) {
                sscanf(linea, "MONSTER_%*d_ATTACK_DAMAGE %d", &sim.monstruos[monster_idx].attack_dmg);
            }
            else if (strstr(linea, "_VISION_RANGE") != NULL) {
                sscanf(linea, "MONSTER_%*d_VISION_RANGE %d", &sim.monstruos[monster_idx].vision_range);
            }
            else if (strstr(linea, "_ATTACK_RANGE") != NULL) {
                sscanf(linea, "MONSTER_%*d_ATTACK_RANGE %d", &sim.monstruos[monster_idx].attack_range);
            }
            else if (strstr(linea, "_COORDS") != NULL) {
                // FIX: Añadir formato de paréntesis y coma
                sscanf(linea, "MONSTER_%*d_COORDS (%d,%d)", &sim.monstruos[monster_idx].pos_inicial.x, &sim.monstruos[monster_idx].pos_inicial.y);
            }
        }
        // ===================================================================
        // Fin del bloque de parseo de monstruos
        // ===================================================================
    }

    fclose(fp); // Cerrar el archivo

    // --- Imprimir resultados para depuración ---
    printf("¡Configuración cargada!\n");
    printf("Grid: %d x %d\n", sim.grid_x, sim.grid_y);
    printf("Vida del Héroe: %d\n", sim.heroe.vida);
    printf("Posición del Héroe: (%d, %d)\n", sim.heroe.pos_inicial.x, sim.heroe.pos_inicial.y);
    printf("Número de Monstruos: %d\n", sim.num_monstruos);

    // FIX: Imprimir stats ANTES de liberar la memoria
    if (sim.num_monstruos > 0) {
        printf("\n--- Stats Monstruo 1 ---\n");
        printf("Vida: %d\n", sim.monstruos[0].vida);
        printf("Daño: %d\n", sim.monstruos[0].attack_dmg);
        printf("Visión: %d\n", sim.monstruos[0].vision_range);
        printf("Ataque: %d\n", sim.monstruos[0].attack_range);
        printf("Coords: (%d, %d)\n", sim.monstruos[0].pos_inicial.x, sim.monstruos[0].pos_inicial.y);
    }

    if (sim.num_monstruos > 0) {
        printf("\n--- Stats Monstruo 2 ---\n");
        printf("Vida: %d\n", sim.monstruos[1].vida);
        printf("Daño: %d\n", sim.monstruos[1].attack_dmg);
        printf("Visión: %d\n", sim.monstruos[1].vision_range);
        printf("Ataque: %d\n", sim.monstruos[1].attack_range);
        printf("Coords: (%d, %d)\n", sim.monstruos[1].pos_inicial.x, sim.monstruos[1].pos_inicial.y);
    }
    
    if (sim.num_monstruos > 0) {
        printf("\n--- Stats Monstruo 3 ---\n");
        printf("Vida: %d\n", sim.monstruos[2].vida);
        printf("Daño: %d\n", sim.monstruos[2].attack_dmg);
        printf("Visión: %d\n", sim.monstruos[2].vision_range);
        printf("Ataque: %d\n", sim.monstruos[2].attack_range);
        printf("Coords: (%d, %d)\n", sim.monstruos[2].pos_inicial.x, sim.monstruos[2].pos_inicial.y);
    }
    // (Puedes añadir un bucle 'for' aquí para imprimir todos los monstruos)

    // FIX: Liberar memoria al final de todo
    if (sim.monstruos != NULL) {
        free(sim.monstruos);
    }

    return 0;
}