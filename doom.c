#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 256
#define MAX_PATH_STEPS 100

enum EstadoMonstruo {
    PASIVO,
    ACTIVO
};

struct Coordenada {
    int x;
    int y;
};

struct Heroe {
    int vida;
    int attack_dmg;
    int attack_range;
    struct Coordenada pos_inicial;
    struct Coordenada pos_actual;
    int paso_ruta_actual;
    struct Coordenada ruta_meta[MAX_PATH_STEPS];
    int num_pasos_ruta;
    pthread_mutex_t lock;
};

struct Monstruo {
    int id;
    int vida;
    int attack_dmg;
    int attack_range;
    int vision_range;
    struct Coordenada pos_inicial;
    struct Coordenada pos_actual;
    enum EstadoMonstruo estado;
    pthread_mutex_t lock;
};

struct Simulacion {
    int grid_x;
    int grid_y;
    struct Heroe heroe;
    struct Monstruo* monstruos;
    int num_monstruos;
    bool heroe_vivo;
    int monstruos_vivos;
    pthread_mutex_t lock_sim;
};

struct MonstruoArgs {
    struct Simulacion* sim;
    int monster_index;
};

int distancia_manhattan(struct Coordenada p1, struct Coordenada p2) {
    return abs(p1.x - p2.x) + abs(p1.y - p2.y);
}

void* funcion_heroe(void* arg) {
    struct Simulacion* sim = (struct Simulacion*) arg;
    struct Heroe* heroe = &sim->heroe;
    
    printf("[Doomguy] Pos inicial (%d, %d); vida %d.\n", 
        heroe->pos_actual.x, heroe->pos_actual.y, heroe->vida);

    while (sim->heroe_vivo && sim->monstruos_vivos > 0 && heroe->paso_ruta_actual < heroe->num_pasos_ruta) {
        
        bool en_combate = false;
        
        for (int i = 0; i < sim->num_monstruos; i++) {
            pthread_mutex_lock(&sim->monstruos[i].lock);
            bool monstruo_vivo = sim->monstruos[i].vida > 0;
            struct Coordenada pos_monstruo = sim->monstruos[i].pos_actual;
            pthread_mutex_unlock(&sim->monstruos[i].lock);

            if (!monstruo_vivo) continue;

            pthread_mutex_lock(&heroe->lock);
            struct Coordenada pos_heroe = heroe->pos_actual;
            pthread_mutex_unlock(&heroe->lock);

            int dist = distancia_manhattan(pos_heroe, pos_monstruo);
            
            if (dist <= heroe->attack_range) {
                en_combate = true;
                
                printf("[Doomguy] Monstruo %d en rango de ataque. Atacando.\n", sim->monstruos[i].id);
                
                pthread_mutex_lock(&sim->monstruos[i].lock);
                sim->monstruos[i].vida -= heroe->attack_dmg;
                int vida_restante = sim->monstruos[i].vida;
                
                if (vida_restante <= 0) {
                    printf("[Doomguy] Monstruo %d eliminado.\n", sim->monstruos[i].id);
                    
                    pthread_mutex_lock(&sim->lock_sim);
                    sim->monstruos_vivos--;
                    pthread_mutex_unlock(&sim->lock_sim);
                }
                pthread_mutex_unlock(&sim->monstruos[i].lock);
                
                break; 
            }
        }

        if (!en_combate) {
            pthread_mutex_lock(&heroe->lock);
            
            heroe->pos_actual = heroe->ruta_meta[heroe->paso_ruta_actual];
            heroe->paso_ruta_actual++;
            
            printf("[Doomguy] Avanzando a (%d, %d). (Vida: %d)\n", 
                heroe->pos_actual.x, heroe->pos_actual.y, heroe->vida);
                
            pthread_mutex_unlock(&heroe->lock);
        }

        sleep(1); 
    }

    if (!sim->heroe_vivo) {
        printf("[Doomguy] muelto\n");
    } else if (sim->monstruos_vivos <= 0) {
        printf("[Doomguy] Ha matado a todos los monstruos.\n");
    } else {
        printf("[Doomguy] Ha llegado a la meta.\n");
    }
    
    pthread_mutex_lock(&sim->lock_sim);
    if (heroe->vida <= 0 || heroe->paso_ruta_actual >= heroe->num_pasos_ruta) {
        sim->heroe_vivo = false;
    }
    pthread_mutex_unlock(&sim->lock_sim);

    return NULL;
}

void* funcion_monstruo(void* arg) {
    struct MonstruoArgs* args = (struct MonstruoArgs*) arg;
    struct Simulacion* sim = args->sim;
    struct Monstruo* monstruo = &sim->monstruos[args->monster_index];

    printf("[Monstruo %d] Posición (%d, %d). Inactivo.\n", 
        monstruo->id, monstruo->pos_actual.x, monstruo->pos_actual.y);

    while (sim->heroe_vivo && monstruo->vida > 0) {
        
        pthread_mutex_lock(&sim->heroe.lock);
        struct Coordenada pos_heroe = sim->heroe.pos_actual;
        pthread_mutex_unlock(&sim->heroe.lock);

        pthread_mutex_lock(&monstruo->lock);
        struct Coordenada pos_monstruo = monstruo->pos_actual;
        enum EstadoMonstruo estado_actual = monstruo->estado;
        pthread_mutex_unlock(&monstruo->lock);

        int dist = distancia_manhattan(pos_heroe, pos_monstruo);

        if (estado_actual == PASIVO) {
            if (dist <= monstruo->vision_range) {
                pthread_mutex_lock(&monstruo->lock);
                monstruo->estado = ACTIVO;
                pthread_mutex_unlock(&monstruo->lock);
                printf("[Monstruo %d] Doomguy en rango de visión. Persiguiendo.\n", monstruo->id);
            } else {
                // printf("MONSTRUO %d: ...zZz...\n", monstruo->id);
            }
        }
        
        else if (estado_actual == ACTIVO) {
            
            if (dist <= monstruo->attack_range) {
                printf("[Monstruo %d] Atacando a Doomguy\n", monstruo->id);
                
                pthread_mutex_lock(&sim->heroe.lock);
                sim->heroe.vida -= monstruo->attack_dmg;
                
                if (sim->heroe.vida <= 0) {
                    pthread_mutex_lock(&sim->lock_sim);
                    sim->heroe_vivo = false;
                    pthread_mutex_unlock(&sim->lock_sim);
                }
                printf("[Monstruo %d] Hit. Vida de Doomguy ahora es %d.\n", monstruo->id, sim->heroe.vida);
                pthread_mutex_unlock(&sim->heroe.lock);
            }
            else {
                int dx = pos_heroe.x - pos_monstruo.x;
                int dy = pos_heroe.y - pos_monstruo.y;

                pthread_mutex_lock(&monstruo->lock);
                if (abs(dx) > abs(dy)) {
                    monstruo->pos_actual.x += (dx > 0) ? 1 : -1;
                } else {
                    monstruo->pos_actual.y += (dy > 0) ? 1 : -1;
                }
                printf("[Monstruo %d] Yendo a (%d, %d). Persiguiendo.\n", 
                    monstruo->id, monstruo->pos_actual.x, monstruo->pos_actual.y);
                pthread_mutex_unlock(&monstruo->lock);
            }
        }

        sleep(1); 

        pthread_mutex_lock(&monstruo->lock);
        if (monstruo->vida <= 0) {
            pthread_mutex_unlock(&monstruo->lock);
            break; 
        }
        pthread_mutex_unlock(&monstruo->lock);
    }
    
    printf("[Monstruo %d] muelto\n", monstruo->id);
    return NULL;
}

int main() {
    struct Simulacion sim;
    sim.num_monstruos = 0;
    sim.monstruos = NULL;

    FILE* fp = fopen("CONFIG.txt", "r");
    if (fp == NULL) {
        perror("Error: No pude abrir el CONFIG.txt");
        return 1;
    }

    char linea[MAX_LINE_LENGTH];
    bool parsing_hero_path = false;

    while (fgets(linea, sizeof(linea), fp) != NULL) {
        
        char linea_copia[MAX_LINE_LENGTH];
        strcpy(linea_copia, linea);
        
        char* first_token = strtok(linea_copia, " \n\t\r");
        if (first_token == NULL || first_token[0] == '#') continue; 

        if (parsing_hero_path && strncmp(first_token, "(", 1) == 0) {
            char* token = strtok(linea, " \n\t\r");
            while (token != NULL) {
                if (sim.heroe.num_pasos_ruta >= MAX_PATH_STEPS) {
                    printf("Advertencia: La ruta del héroe es muy larga (más de %d pasos). Voy a cortar.\n", MAX_PATH_STEPS);
                    break;
                }
                
                int x, y;
                if (sscanf(token, "(%d,%d)", &x, &y) == 2) {
                    int idx = sim.heroe.num_pasos_ruta;
                    sim.heroe.ruta_meta[idx].x = x;
                    sim.heroe.ruta_meta[idx].y = y;
                    sim.heroe.num_pasos_ruta++;
                }
                token = strtok(NULL, " \n\t\r");
            }
            continue; 
        }

        parsing_hero_path = false;
        
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
            sscanf(linea, "HERO_START %d %d", &sim.heroe.pos_inicial.x, &sim.heroe.pos_inicial.y);
        }
        else if (strncmp(linea, "HERO_PATH", 9) == 0) {
            parsing_hero_path = true;
            sim.heroe.num_pasos_ruta = 0;
            
            char* token = strtok(linea, " \n\t\r");
            token = strtok(NULL, " \n\t\r");
            while (token != NULL) {
                if (sim.heroe.num_pasos_ruta >= MAX_PATH_STEPS) break;
                int x, y;
                if (sscanf(token, "(%d,%d)", &x, &y) == 2) {
                    int idx = sim.heroe.num_pasos_ruta;
                    sim.heroe.ruta_meta[idx].x = x;
                    sim.heroe.ruta_meta[idx].y = y;
                    sim.heroe.num_pasos_ruta++;
                }
                token = strtok(NULL, " \n\t\r");
            }
        }
        else if (strncmp(linea, "MONSTER_COUNT", 13) == 0) {
            sscanf(linea, "MONSTER_COUNT %d", &sim.num_monstruos);
            sim.monstruos = malloc(sim.num_monstruos * sizeof(struct Monstruo));
            if (sim.monstruos == NULL) {
                perror("Error: No pude reservar memoria para los monstruos");
                fclose(fp); return 1;
            }
        }
        else if (strncmp(linea, "MONSTER_", 8) == 0) {
            int monster_id;
            if (sscanf(linea, "MONSTER_%d_", &monster_id) != 1) continue;
            if (monster_id < 1 || monster_id > sim.num_monstruos) {
                 printf("Error: El ID %d del monstruo está mal, no existe.\n", monster_id);
                continue;
            }
            int monster_idx = monster_id - 1;
            sim.monstruos[monster_idx].id = monster_id;

            if (strstr(linea, "_HP") != NULL) {
                int count = sscanf(linea, "MONSTER_%*d_HP %d", &sim.monstruos[monster_idx].vida);
                if (count == 0 && fgets(linea, sizeof(linea), fp) != NULL) {
                    sscanf(linea, "%d", &sim.monstruos[monster_idx].vida);
                }
            } else if (strstr(linea, "_ATTACK_DAMAGE") != NULL) {
                int count = sscanf(linea, "MONSTER_%*d_ATTACK_DAMAGE %d", &sim.monstruos[monster_idx].attack_dmg);
                if (count == 0 && fgets(linea, sizeof(linea), fp) != NULL) {
                    sscanf(linea, "%d", &sim.monstruos[monster_idx].attack_dmg);
                }
            } else if (strstr(linea, "_VISION_RANGE") != NULL) {
                int count = sscanf(linea, "MONSTER_%*d_VISION_RANGE %d", &sim.monstruos[monster_idx].vision_range);
                if (count == 0 && fgets(linea, sizeof(linea), fp) != NULL) {
                    sscanf(linea, "%d", &sim.monstruos[monster_idx].vision_range);
                }
            } else if (strstr(linea, "_ATTACK_RANGE") != NULL) {
                int count = sscanf(linea, "MONSTER_%*d_ATTACK_RANGE %d", &sim.monstruos[monster_idx].attack_range);
                if (count == 0 && fgets(linea, sizeof(linea), fp) != NULL) {
                    sscanf(linea, "%d", &sim.monstruos[monster_idx].attack_range);
                }
            } else if (strstr(linea, "_COORDS") != NULL) {
                int count = sscanf(linea, "MONSTER_%*d_COORDS %d %d", 
                    &sim.monstruos[monster_idx].pos_inicial.x,
                    &sim.monstruos[monster_idx].pos_inicial.y);
                if (count == 1 && fgets(linea, sizeof(linea), fp) != NULL) {
                    sscanf(linea, "%d", &sim.monstruos[monster_idx].pos_inicial.y);
                }
            }
        }
    }
    fclose(fp);

    printf("Configuración cargada. Empezando la simulación...\n");
    
    sim.heroe_vivo = true;
    sim.monstruos_vivos = sim.num_monstruos;
    pthread_mutex_init(&sim.lock_sim, NULL);

    sim.heroe.pos_actual = sim.heroe.pos_inicial;
    sim.heroe.paso_ruta_actual = 0;
    pthread_mutex_init(&sim.heroe.lock, NULL);

    for (int i = 0; i < sim.num_monstruos; i++) {
        sim.monstruos[i].pos_actual = sim.monstruos[i].pos_inicial;
        sim.monstruos[i].estado = PASIVO;
        pthread_mutex_init(&sim.monstruos[i].lock, NULL);
    }

    pthread_t id_thread_heroe;
    pthread_t* id_threads_monstruos = malloc(sim.num_monstruos * sizeof(pthread_t));
    struct MonstruoArgs* args_monstruos = malloc(sim.num_monstruos * sizeof(struct MonstruoArgs));

    if (id_threads_monstruos == NULL || args_monstruos == NULL) {
        perror("Error: No pude crear memoria para los hilos"); return 1;
    }

    pthread_create(&id_thread_heroe, NULL, funcion_heroe, (void*) &sim);

    for (int i = 0; i < sim.num_monstruos; i++) {
        args_monstruos[i].sim = &sim;
        args_monstruos[i].monster_index = i;
        pthread_create(&id_threads_monstruos[i], NULL, funcion_monstruo, (void*) &args_monstruos[i]);
    }

    pthread_join(id_thread_heroe, NULL);
    printf("MAIN: El hilo del héroe terminó.\n");

    for (int i = 0; i < sim.num_monstruos; i++) {
        pthread_join(id_threads_monstruos[i], NULL);
    }
    printf("MAIN: Todos los hilos de monstruos terminaron.\n");

    printf("MAIN: Se acabó. Limpiando la memoria.\n");

    pthread_mutex_destroy(&sim.heroe.lock);
    pthread_mutex_destroy(&sim.lock_sim);
    for (int i = 0; i < sim.num_monstruos; i++) {
        pthread_mutex_destroy(&sim.monstruos[i].lock);
    }

    free(sim.monstruos);
    free(id_threads_monstruos);
    free(args_monstruos); 

    return 0;
}