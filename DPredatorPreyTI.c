//DESENCRIPTADOR FINAL EN LIMPIO Y ORDENADO
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <sys/time.h>
#include <stdint.h>

//CONSTANTES
#define FILAS 8
#define COLUMNAS 8
#define NUM_CAZADORES 2
#define NUM_PRESAS 8
#define BYTES_TO_CIPHER 8
//Menor a 4 presas activa la reproducción (es decir al llegar a 3 presas)
#define MIN_PRESAS 4
//Por si queremos encriptar de golpe
#define RONDAS 15
//FUNCIONES MACRO
//Mapa de Renyi con shift constante (version macro para mayor velocidad)
#define RENYI_MAP(x, B, shift) ((B) * (x) + ((x) >> (shift)))
//Distancia de Manhattan (macro para evitar llamada a funcion)
#define DISTANCIA_MANHATTAN(f1, c1, f2, c2) (abs((f1) - (f2)) + abs((c1) - (c2)))

//Estructura de Agente (solo las presas usan el mapa de Renyi)
typedef struct{
    int fila, columna;      //Posicion actual en el tablero (x,y)
    unsigned long long x0;  //Semilla inicial para el mapa caótico (x0 inicial)
    unsigned long long B;   //Parametro B del mapa de Renyi
    int shift;              //Shift utilizado en el mapa
} Agente;

//Estructura del tablero (permite verificar colisiones y ocupacion)
typedef struct{
    int hay_presa;         //1 si ocupado por una presa, 0 si libre
    Agente *presa_ptr;   //Puntero a la presa si la hay
    int hay_cazador;     //1 si hay un cazador en esta celda
} Control_Tablero;

//Estructura para representar todas las posibles POSICIONES
typedef struct {
    int indice;   //Indice lineal (0 a 63 en el caso 8x8)
    int fila;
    int columna;
} Posicion;

//Arreglo global del tablero
Control_Tablero tablero[FILAS][COLUMNAS];
//Arreglo global con todas las posiciones posibles(se barajea al inicio una vez solamente)
Posicion POSICIONES[FILAS * COLUMNAS];
//Arreglo de control para saber que posiciones estan ocupadas con más seguridad (segun indice en POSICIONES[])
bool posiciones_ocupadas[FILAS * COLUMNAS] = {false};

//RENYI GLOBAL
//Genera una posicion aleatoria en el tablero utilizando un mapa caotico de Renyi
//Estas variables de Renyi se usan tambien para la reproduccion
unsigned long long x0_general;  //Estado inicial del mapa
unsigned long long B_general;   //Parametro B del mapa de Renyi
int shift_general;              //Shift (desplazamiento) utilizado en Renyi

//ADMINISTRADO DE BITS (prestamos de bits a partir del RENYI GLOBAL)
unsigned long long bits_buffer = 0;  //Buffer donde se almacenan los bits generados
int bits_disponibles = 0;            //Numero de bits aun disponibles en el buffer

unsigned long long obtener_bits(int bits_necesitados){
    //Si no hay suficientes bits disponibles, generar nuevos bits desde el mapa de Renyi
    if (bits_disponibles < bits_necesitados) {
        x0_general = RENYI_MAP(x0_general, B_general, shift_general);
        bits_buffer = x0_general;
        bits_disponibles = sizeof(unsigned long long)*8;  //64 bits
    }
    //Construir mascara para extraer los bits requeridos
    unsigned long long mascara = (1ULL << bits_necesitados) - 1;
    unsigned long long bits = bits_buffer & mascara; //extrae los bits menos significativos
    //Actualizar el buffer y el numero de bits disponibles
    bits_buffer >>= bits_necesitados;
    bits_disponibles -= bits_necesitados;
    return bits;
}

//INICIALIZAR AGENTES
//Funcion para Barajear el arreglo POSICIONES[]
void barajar_posiciones(){
    int total = FILAS*COLUMNAS;
    for (int i = total - 1; i > 0; i--){
        //Si es muy necesario el modulo porque va reduciendose el espacio en el shuffle
        int j = obtener_bits(6)%(i + 1); //de 0 a 63 posiciones
        //tmp es tipo Posicion porque intercambiamos con toda la informacion, fila, columna e indice
        Posicion tmp = POSICIONES[i];
        POSICIONES[i] = POSICIONES[j];
        POSICIONES[j] = tmp;
    }
}

//Inicializa el arreglo POSICIONES con todas las coordenadas posibles y las usa la funcion barajar_posiciones()
void inicializar_posiciones(){
    int k = 0;
    for(int f = 0; f < FILAS; f++){
        for (int c = 0; c < COLUMNAS; c++){
            POSICIONES[k].indice = k;
            POSICIONES[k].fila = f;
            POSICIONES[k].columna = c;
            k++;
        }
    }
    //Mezclar
    barajar_posiciones();
    //Reiniciar el arreglo de ocupacion de posiciones, todas desocupadas por el momento
    for (int i = 0; i < FILAS * COLUMNAS; i++){
        posiciones_ocupadas[i] = false;
    }
}

//Inicializa cazadores y presas usando las primeras posiciones del arreglo que se le hizo shuffle
void inicializar_agentes(Agente *cazadores, Agente *presas, unsigned long long *llave_acotada){
    int index_llave = 3; //Saltar los primeros 3 valores de la llave (usados por el RENYI GLOBAL)
    //Inicializar todas las celdas del tablero como vacias (este es el arreglo/struct del control del tablero)
    for(int f = 0; f < FILAS; f++){
        for (int c = 0; c < COLUMNAS; c++){
            tablero[f][c].hay_presa = 0;
            tablero[f][c].presa_ptr = NULL;
            tablero[f][c].hay_cazador = 0;
        }
    }
    //Inicializar los cazadores usando las primeras posiciones del arreglo POSICIONES (las dos primeras)
    for(int i = 0; i < NUM_CAZADORES; i++){
        cazadores[i].fila = POSICIONES[i].fila;
        cazadores[i].columna = POSICIONES[i].columna;
        tablero[cazadores[i].fila][cazadores[i].columna].hay_cazador = 1;
        posiciones_ocupadas[i] = true;
    }
    //Inicializar las presas usando las siguientes posiciones del arreglo (las siguiente ocho posiciones)
    for(int i = 0; i < NUM_PRESAS; i++){
        int idx = NUM_CAZADORES + i;
        presas[i].fila = POSICIONES[idx].fila;
        presas[i].columna = POSICIONES[idx].columna;
        presas[i].x0 = llave_acotada[index_llave++];
        presas[i].B  = llave_acotada[index_llave++];
        presas[i].shift = llave_acotada[index_llave++];
        tablero[presas[i].fila][presas[i].columna].hay_presa = 1;
        tablero[presas[i].fila][presas[i].columna].presa_ptr = &presas[i];
        posiciones_ocupadas[idx] = true;
    }
}

//ENCONTRAR AGENTE MAS CERCANO (UTIL PARA CAZADORES Y PRESAS)
//Encuentra el agente mas cercano a un agente dado usando distancia Manhattan
Agente* agente_mas_cercano(Agente *agente_origen, Agente *objetivos, int num_objetivos){
    int min_distancia = FILAS + COLUMNAS; //distancia maxima que puede haber en el tablero
    Agente *mas_cercano = NULL; //definimos el apuntador que vamos a devolver al agente mas cercano
    for(int i = 0; i < num_objetivos; i++){
        int distancia = DISTANCIA_MANHATTAN(agente_origen->fila, agente_origen->columna,
                                            objetivos[i].fila, objetivos[i].columna);
        if (distancia < min_distancia){
            min_distancia = distancia;
            mas_cercano = &objetivos[i];
        }
    }
    return mas_cercano;
}

//SELECCIONAR MAPAS PARA ENCRIPTAR (INDICE)
//Devuelve el indice de la presa mas cercana al cazador actual (alternando)
int seleccionar_indice_base(Agente *presas, int num_presas, Agente *cazadores){
    static int turno_cazador = 0;
    Agente *cazador_actual = &cazadores[turno_cazador];
    turno_cazador = obtener_bits(1);  //alterna
    //Usamos encontrar agente más cercano
    Agente *presa_cercana = agente_mas_cercano(cazador_actual, presas, num_presas);
    //Obtenemos el indice directamente
    return (int)(presa_cercana - presas);
}


//MOVIMIENTOS DE LOS AGENTES
//Mueve a los cazadores intentando minimizar la distancia a la presa mas cercana
//Evita colisiones y utiliza una preferencia aleatoria entre movimiento vertical u horizontal dependiendo la posicion de la presa
void mover_cazadores(Agente *cazadores, Agente *presas, int num_presas){
    //Evaluar movimiento para cada cazador
    for(int i = 0; i < NUM_CAZADORES; i++){
        //¿Quien es mi presa mas cercana?
        Agente *c = &cazadores[i];
        Agente *presa = agente_mas_cercano(c, presas, num_presas);
        //Si no hay presas, quedarse en su lugar
        if (!presa){
            continue;
        }
        //Determinar direccion de movimiento hacia la presa
        int vertical = 0, horizontal = 0;
        if (presa->fila < c->fila) vertical = -1;
        else if (presa->fila > c->fila) vertical = 1;
        if (presa->columna < c->columna) horizontal = -1;
        else if (presa->columna > c->columna) horizontal = 1;
        //Determinar preferencia aleatoria de direccion (0 = vertical, 1 = horizontal)
        int preferencia = obtener_bits(1);
        int nueva_fila = c->fila;
        int nueva_col = c->columna;
        //Intentar primera direccion segun preferencia
        if (preferencia == 0 && vertical != 0){
            int f = c->fila + vertical;
            if (f >= 0 && f < FILAS &&
                !tablero[f][c->columna].hay_cazador){
                nueva_fila = f;
            }
        }else if (preferencia == 1 && horizontal != 0){
            int c2 = c->columna + horizontal;
            if (c2 >= 0 && c2 < COLUMNAS &&
                !tablero[c->fila][c2].hay_cazador){
                nueva_col = c2;
            }
        }
        //Si la celda esta ocupada por otro cazador, intentar la otra direccion
        if(tablero[nueva_fila][nueva_col].hay_cazador){
            nueva_fila = c->fila;
            nueva_col = c->columna;
            if (preferencia == 0 && horizontal != 0){
                int c2 = c->columna + horizontal;
                if (c2 >= 0 && c2 < COLUMNAS &&
                    !tablero[c->fila][c2].hay_cazador){
                    nueva_col = c2;
                }
            }else if (preferencia == 1 && vertical != 0){
                int f = c->fila + vertical;
                if (f >= 0 && f < FILAS &&
                    !tablero[f][c->columna].hay_cazador){
                    nueva_fila = f;
                }
            }
            //Si ambas opciones fallan, no se mueve
            if(tablero[nueva_fila][nueva_col].hay_cazador ||
                (nueva_fila == c->fila && nueva_col == c->columna)){
                continue;
            }
        }
        //Actualizar en el tablero: limpiar celda anterior y marcar nueva celda
        tablero[c->fila][c->columna].hay_cazador = 0;
        tablero[nueva_fila][nueva_col].hay_cazador = 1;
        //Guardar el nuevo movimiento
        c->fila = nueva_fila;
        c->columna = nueva_col;
    }
}

//MOVIMIENTO DE LAS PRESAS
//Las presas maximizan la distancia Manhattan con respecto al cazador más cercano
//Evitan colisiones con cazadores y otras presas, y actualizan su estado y mapa
void mover_presas(Agente *presas, Agente *cazadores, int *num_presas){
    for(int i = 0; i < *num_presas; i++){
        Agente *p = &presas[i];
        int fila_ant = p->fila;
        int col_ant  = p->columna;
        int nueva_fila = fila_ant;
        int nueva_col  = col_ant;

        //Buscar cazador más cercano
        Agente *cazador = agente_mas_cercano(p, cazadores, NUM_CAZADORES);
        if (!cazador) continue;

        //Determinar dirección de huida (inversa a la dirección hacia el cazador)
        int vertical = 0, horizontal = 0;
        if (cazador->fila < p->fila) vertical = 1;
        else if (cazador->fila > p->fila) vertical = -1;
        if (cazador->columna < p->columna) horizontal = 1;
        else if (cazador->columna > p->columna) horizontal = -1;

        //Preferencia aleatoria de dirección (0 = vertical, 1 = horizontal)
        int preferencia = obtener_bits(1);

        //Intentar primera dirección según preferencia
        if (preferencia == 0 && vertical != 0){
            int f = fila_ant + vertical;
            if (f >= 0 && f < FILAS &&
                !tablero[f][col_ant].hay_presa &&
                !tablero[f][col_ant].hay_cazador){
                nueva_fila = f;
            }
        } else if (preferencia == 1 && horizontal != 0){
            int c2 = col_ant + horizontal;
            if (c2 >= 0 && c2 < COLUMNAS &&
                !tablero[fila_ant][c2].hay_presa &&
                !tablero[fila_ant][c2].hay_cazador){
                nueva_col = c2;
            }
        }

        //Si la celda está ocupada, intentar otra dirección
        if (tablero[nueva_fila][nueva_col].hay_presa ||
            tablero[nueva_fila][nueva_col].hay_cazador){
            nueva_fila = fila_ant;
            nueva_col  = col_ant;

            if (preferencia == 0 && horizontal != 0){
                int c2 = col_ant + horizontal;
                if (c2 >= 0 && c2 < COLUMNAS &&
                    !tablero[fila_ant][c2].hay_presa &&
                    !tablero[fila_ant][c2].hay_cazador){
                    nueva_col = c2;
                }
            } else if (preferencia == 1 && vertical != 0){
                int f = fila_ant + vertical;
                if (f >= 0 && f < FILAS &&
                    !tablero[f][col_ant].hay_presa &&
                    !tablero[f][col_ant].hay_cazador){
                    nueva_fila = f;
                }
            }

            //Si ambas opciones fallan, no se mueve
            if ((nueva_fila == fila_ant && nueva_col == col_ant) ||
                tablero[nueva_fila][nueva_col].hay_presa ||
                tablero[nueva_fila][nueva_col].hay_cazador){
                continue;
            }
        }

        //Actualizar tablero (celda anterior y nueva)
        tablero[fila_ant][col_ant].hay_presa = 0;
        tablero[fila_ant][col_ant].presa_ptr = NULL;
        tablero[nueva_fila][nueva_col].hay_presa = 1;
        tablero[nueva_fila][nueva_col].presa_ptr = p;

        //Actualizar posición de la presa
        p->fila = nueva_fila;
        p->columna = nueva_col;

        //Actualizar el mapa caótico de la presa
        //p->x0 = RENYI_MAP(p->x0, p->B, p->shift);

        //Actualizar estado en posiciones_ocupadas
        for (int p_idx = 0; p_idx < FILAS * COLUMNAS; p_idx++) {
            if (POSICIONES[p_idx].fila == fila_ant && POSICIONES[p_idx].columna == col_ant)
                posiciones_ocupadas[p_idx] = false;
            if (POSICIONES[p_idx].fila == nueva_fila && POSICIONES[p_idx].columna == nueva_col)
                posiciones_ocupadas[p_idx] = true;
        }
    }
}


//VERIFICAR Y ELIMINAR PRESAS
//Verifica si un cazador ha atrapado una presa y la elimina del arreglo de presas
void verificar_y_eliminar_presas(Agente *cazadores, Agente *presas, int *num_presas){
    for (int i = 0; i < NUM_CAZADORES; i++){
        //Si hay una presa en esta celda, la eliminamos
        if(tablero[cazadores[i].fila][cazadores[i].columna].hay_presa &&
           tablero[cazadores[i].fila][cazadores[i].columna].presa_ptr != NULL){

            Agente *presa_comida = tablero[cazadores[i].fila][cazadores[i].columna].presa_ptr;

            for (int j = 0; j < *num_presas; j++){
                //Comparacion por posicion
                if (presas[j].fila == presa_comida->fila &&
                    presas[j].columna == presa_comida->columna){
                    //Marcar la posicion como libre en el arreglo POSICIONES
                    for(int p = 0; p < FILAS * COLUMNAS; p++){
                        if (POSICIONES[p].fila == presa_comida->fila &&
                            POSICIONES[p].columna == presa_comida->columna){
                            posiciones_ocupadas[p] = false;
                            break;
                        }
                    }

                    //Reorganizar el arreglo de presas (rellenar el hueco con la ultima presa)
                    if (j != (*num_presas - 1)){
                        presas[j] = presas[*num_presas - 1];
                        //ACTUALIZAR puntero en el tablero a la nueva posición
                        int f = presas[j].fila;
                        int c = presas[j].columna;
                        if (tablero[f][c].hay_presa) {
                            tablero[f][c].presa_ptr = &presas[j];
                        }
                    }
                    (*num_presas)--;
                    //Limpiar la celda del tablero
                    tablero[cazadores[i].fila][cazadores[i].columna].hay_presa = 0;
                    tablero[cazadores[i].fila][cazadores[i].columna].presa_ptr = NULL;
                    //Salir del ciclo interno una vez eliminada
                    break;
                }
            }
        }
    }
}

//REPRODUCCION DE PRESAS
//Reproduce nuevas presas cuando su numero cae por debajo del minimo permitido
void reproducir_presas(Agente *presas, int *num_presas){
    //En hexa es una notacion mas limpia para mascaras muy grandes
    unsigned long long MASK_LSB = 0xFFFFFFFFUL;           //32 bits bajos
    unsigned long long MASK_MSB = 0xFFFFFFFF00000000UL;   //32 bits altos
    //Para shift las mascaras son diferentes, es un valor muy pequeñito (1 a 8)
    unsigned int SHIFT_LSB = 0x0F; //4 bits bajos
    unsigned int SHIFT_MSB = 0xF0; //4 bits altos

    while(*num_presas < NUM_PRESAS){
        int idx = *num_presas;
        //1.Construir lista de posiciones realmente libres
        int libres_idx[FILAS * COLUMNAS];
        int total_libres = 0;
        for(int i = 0; i < FILAS * COLUMNAS; i++){
            int f = POSICIONES[i].fila;
            int c = POSICIONES[i].columna;
            if (!tablero[f][c].hay_presa && !tablero[f][c].hay_cazador){
                libres_idx[total_libres++] = i;
            }
        }
        //Solo por robustez
        if (total_libres == 0) break;

        //2.Seleccionar una posicion aleatoria libre
        int nueva_pos_idx = libres_idx[obtener_bits(6) % total_libres];
        if (POSICIONES[nueva_pos_idx].fila < 0 || POSICIONES[nueva_pos_idx].fila >= FILAS ||
            POSICIONES[nueva_pos_idx].columna < 0 || POSICIONES[nueva_pos_idx].columna >= COLUMNAS){
            continue;
        }
        int fila = POSICIONES[nueva_pos_idx].fila;
        int columna = POSICIONES[nueva_pos_idx].columna;

        //3.Seleccionar padres aleatorios del arreglo de presas existente
        int parent1 = obtener_bits(3) % (*num_presas);
        int parent2 = obtener_bits(3) % (*num_presas);

        //4. Cruza genetica por particion de bits para generar nueva presa
        presas[idx].fila = fila;
        presas[idx].columna = columna;

        unsigned long long x0_lsb = presas[parent1].x0 & MASK_LSB;
        unsigned long long x0_msb = presas[parent2].x0 & MASK_MSB;
        presas[idx].x0 = (x0_lsb ^ x0_msb) + 1; // Evitar 0

        unsigned long long B_lsb = presas[parent1].B & MASK_LSB;
        unsigned long long B_msb = presas[parent2].B & MASK_MSB;
        presas[idx].B = (B_lsb ^ B_msb) + 1;    // Evitar 0

        unsigned int shift_lsb = presas[parent1].shift & SHIFT_LSB;
        unsigned int shift_msb = presas[parent2].shift & SHIFT_MSB;
        presas[idx].shift = ((shift_lsb ^ shift_msb) % 8) + 1; //Evitar 0 y que permanezca no tan grande

        //5. Actualizar tablero
        tablero[fila][columna].hay_presa = 1;
        tablero[fila][columna].presa_ptr = &presas[idx];
        (*num_presas)++;
    }
}

//Funcion para imprimir el tablero (quedo igual)
void imprimir_tablero(Agente *cazadores, Agente *presas, int num_presas){
    char tablero[FILAS][COLUMNAS];
    memset(tablero,'.', sizeof(tablero));
    for (int i=0; i < NUM_CAZADORES; i++)
        tablero[cazadores[i].fila][cazadores[i].columna] = 'C';
    for (int i=0; i < num_presas; i++)
        tablero[presas[i].fila][presas[i].columna] = 'P';
    printf("\nTablero Actual:\n");
    for (int i=0; i < FILAS; i++) {
        for (int j=0; j < COLUMNAS; j++)
            printf("%c ", tablero[i][j]);
        printf("\n");
    }
}

//CREAR LLAVE
//Genera una llave aleatoria SIN acotaciones (%)
//Por el momento es de 1224 bits porque son 8 mapas y uno general
void crear_llave(unsigned long long *llave){
    for (int i = 0; i < 3 * (NUM_PRESAS + 1); i++){
        llave[i] = rand(); //Sin aplicar % todavia
    }
    printf("Llave se ha generado correctamente.\n");
}

//Guardar la llave en un archivo
void guardar_llave(unsigned long long *llave){
    FILE *archivo = fopen("llave_simulacion.txt", "w");
    if (!archivo){
        printf("Error al abrir el archivo de llave.\n");
        return;
    }
    for (int i = 0; i < 3 * (NUM_PRESAS + 1); i++){
        fprintf(archivo, "%llu ", llave[i]); //Espacio para separar
    }
    fprintf(archivo, "\n");
    fclose(archivo);
    printf("Llave guardada en 'llave_simulacion.txt'.\n");
}

//Aplica los modulos a cada valor de la llave antes de usarlos
void acotar_valores_llave(unsigned long long *llave, unsigned long long *llave_acotada){
    int src = 0; //leer
    int dst = 0; //indice para escribir y no confundir
    for(int i = 0; i < (NUM_PRESAS + 1); i++){
        llave_acotada[dst++] = (llave[src++]) + 1;         //x0
        llave_acotada[dst++] = (llave[src++]) + 1;         //B
        llave_acotada[dst++] = (llave[src++] % 8) + 1;     // un shift muy grande no es conveniente
    }
    printf("Valores de la llave acotados correctamente.\n");
}

//Inicializar solo el Renyi general usando la llave acotada
//(Los parametros de las presas ya se asignan dentro de inicializar_agentes)
void inicializar_renyi_general(unsigned long long *llave_acotada){
    //Los primeros 3 son del renyi general y a partir del indice 3 se asocian los de las presas
    x0_general    = llave_acotada[0];
    B_general     = llave_acotada[1];
    shift_general = llave_acotada[2];
    printf("Parametros del Renyi general inicializados correctamente.\n");
}

//CONSTRUCCION DE LA TABLA DE INDICES PARA CIFRADO SIN MODULO
//Cada fila corresponde a un valor posible de num_presas (de 4 a 8), es decir filas 0 a 4
unsigned char Tabla_indices[5][255];
void construir_tabla_indices() {
    for (int fila = 0; fila < 5; fila++) {
        int valor_np = fila + 4;
        for (int i = 0; i < 255; i++) {
            Tabla_indices[fila][i] = i % valor_np;
        }
    }
}

//FUNCION DE DESENCRIPTADO
//Recibe el archivo cifrado y simula la dinamica haciendo el proceso inverso
unsigned char* desencriptar_texto(unsigned char *entrada, size_t tamano, unsigned long long *llave_acotada, size_t *tamano_salida){
    if (!entrada) return NULL;

    //Inicializar agentes
    Agente cazadores[NUM_CAZADORES], presas[NUM_PRESAS];
    int num_presas = NUM_PRESAS;
    unsigned long long C_prev = 0;

    //Inicializar estado del sistema
    inicializar_posiciones();
    inicializar_agentes(cazadores, presas, llave_acotada);

    //Calcular bloques completos y bytes restantes
    size_t bloques = tamano / BYTES_TO_CIPHER;
    size_t sobrante = tamano % BYTES_TO_CIPHER;

    unsigned char *ptr = entrada;

    //INICIA CICLO DE DESENCRIPTADO por grupos de RONDAS
    size_t i = 0;
    while (i < bloques){
        //1. Reproducir presas solo si es necesario
        if (num_presas < MIN_PRESAS){
            reproducir_presas(presas, &num_presas);
        }

        //2. Seleccionar índice base desde la presa más cercana
        int k = seleccionar_indice_base(presas, num_presas, cazadores);

        //3. Obtener puntero a la tabla de índices según num_presas y desplazado por k
        unsigned char *TInd = Tabla_indices[num_presas - 4] + k;

        //4. Desencriptar hasta RONDAS bloques
        for (int j = 0; j < RONDAS && i < bloques; j++, i++){
            int a = *(TInd);
            int b = *(TInd + 1);
            int c = *(TInd + 2);

            unsigned long long C;
            memcpy(&C, ptr, sizeof(unsigned long long));
            unsigned long long P = ((C - C_prev) ^ presas[b].x0 ^ presas[c].x0) - presas[a].x0;
            memcpy(ptr, &P, sizeof(unsigned long long));
            C_prev = C;

            //Actualizar trayectorias caóticas
            presas[a].x0 = RENYI_MAP(presas[a].x0, presas[a].B, presas[a].shift);
            presas[b].x0 = RENYI_MAP(presas[b].x0, presas[b].B, presas[b].shift);
            presas[c].x0 = RENYI_MAP(presas[c].x0, presas[c].B, presas[c].shift);

            ptr += sizeof(unsigned long long);
            TInd += 3; //Avanzar al siguiente trío de índices
        }

        //5. Mover entorno (cazadores y presas)
        mover_cazadores(cazadores, presas, num_presas);
        verificar_y_eliminar_presas(cazadores, presas, &num_presas);
        mover_presas(presas, cazadores, &num_presas);
    }

    //CASO ESPECIAL: Procesar los últimos bytes si no alcanzan 8
    if (sobrante > 0){
        unsigned long long bloque_cifrado = 0;
        memcpy(&bloque_cifrado, ptr, sobrante);

        if (num_presas < MIN_PRESAS){
            reproducir_presas(presas, &num_presas);
        }

        int k = seleccionar_indice_base(presas, num_presas, cazadores);
        unsigned char *TInd = Tabla_indices[num_presas - 4] + k;

        int a = *(TInd);
        int b = *(TInd + 1);
        int c = *(TInd + 2);

        unsigned long long bloque_mensaje = ((bloque_cifrado - C_prev) ^ presas[b].x0 ^ presas[c].x0) - presas[a].x0;
        memcpy(ptr, &bloque_mensaje, sobrante);

        //Actualizar trayectorias caóticas
        presas[a].x0 = RENYI_MAP(presas[a].x0, presas[a].B, presas[a].shift);
        presas[b].x0 = RENYI_MAP(presas[b].x0, presas[b].B, presas[b].shift);
        presas[c].x0 = RENYI_MAP(presas[c].x0, presas[c].B, presas[c].shift);

        //Mover entorno
        mover_cazadores(cazadores, presas, num_presas);
        verificar_y_eliminar_presas(cazadores, presas, &num_presas);
        mover_presas(presas, cazadores, &num_presas);
    }

    *tamano_salida = tamano;
    return entrada;
}


//PROCESAMIENTO DE ARCHIVOS
//Funcion para detectar si es un archivo .pgm
int es_pgm(const char *nombre_archivo) {
    FILE *archivo = fopen(nombre_archivo, "rb");
    if (!archivo) return 0;
    char magic[3] = {0};
    fread(magic, 1, 2, archivo); // Leer los dos primeros bytes
    fclose(archivo);
    return (magic[0] == 'P' && (magic[1] == '5' || magic[1] == '2'));
}
//Funcion para leer archivo (PGM o generico)
unsigned char* leer_archivo(const char *nombre_archivo, char *cabecera, size_t *tamano_datos, size_t *cabecera_len, int es_pgm) {
    FILE *archivo = fopen(nombre_archivo, "rb");
    if (!archivo) {
        printf("Error: No se pudo abrir el archivo '%s'.\n", nombre_archivo);
        return NULL;
    }

    *cabecera_len = 0;
    if (es_pgm) {
        int c, contador_saltos = 0;
        while (contador_saltos < 3 && (c = fgetc(archivo)) != EOF) {
            cabecera[(*cabecera_len)++] = c;
            if (c == '\n') contador_saltos++;
        }
        cabecera[*cabecera_len] = '\0'; // Terminar cabecera como cadena
    }
    fseek(archivo, 0, SEEK_END);
    long tamano_total = ftell(archivo);
    *tamano_datos = tamano_total - *cabecera_len;
    rewind(archivo);
    fseek(archivo, *cabecera_len, SEEK_SET);
    unsigned char *datos = (unsigned char *)malloc(*tamano_datos);
    if (!datos) {
        printf("Error: No se pudo asignar memoria.\n");
        fclose(archivo);
        return NULL;
    }
    fread(datos, 1, *tamano_datos, archivo);
    fclose(archivo);
    return datos;
}
//Funcion para guardar archivo resultado
int guardar_archivo(const char *nombre_salida, const char *cabecera, size_t cabecera_len, const unsigned char *datos, size_t tamano) {
    FILE *archivo = fopen(nombre_salida, "wb");
    if (!archivo) {
        printf("Error: No se pudo abrir archivo de salida '%s'.\n", nombre_salida);
        return 1;
    }
    if (cabecera_len > 0) {
        fwrite(cabecera, 1, cabecera_len, archivo);
    }
    fwrite(datos, 1, tamano, archivo);
    fclose(archivo);
    return 0;
}
//Funcion auxiliar para obtener la extension de un archivo
const char* obtener_extension(const char *nombre_archivo){
    const char *punto = strrchr(nombre_archivo, '.');
    return (punto != NULL) ? (punto + 1) : "";
}

//MAIN DEL DESENCRIPTADOR
int main(int argc, char *argv[]){
    //Semilla aleatoria para Renyi
    srand(time(NULL));
    //El usuario debe ingresar la extension esperada del archivo recuperado
    if (argc != 2) {
        printf("Uso: %s <extension_salida>\n", argv[0]);
        return 1;
    }
    const char *extension_salida = argv[1];
    //1. Leer llave previamente generada desde archivo
    unsigned long long llave[(NUM_PRESAS + 1) * 3];
    FILE *archivo_llave = fopen("llave_simulacion.txt", "r");
    if (!archivo_llave) {
        printf("Error: No se pudo abrir 'llave_simulacion.txt'.\n");
        return 1;
    }
    for (int i = 0; i < ((NUM_PRESAS + 1) * 3); i++){
        if (fscanf(archivo_llave, "%llu", &llave[i]) != 1){
            fclose(archivo_llave);
            printf("Error: Lectura incompleta de la llave.\n");
            return 1;
        }
    }
    fclose(archivo_llave);
    //2. Acotar e inicializar parámetros caóticos
    unsigned long long llave_acotada[(NUM_PRESAS + 1) * 3];
    acotar_valores_llave(llave, llave_acotada);
    inicializar_renyi_general(llave_acotada);
    //Construir la tabla de indices
    construir_tabla_indices();
    //3. Leer archivo cifrado (binario)
    char cabecera[512];
    size_t tamano_datos = 0, cabecera_len = 0;
    int esPGM = es_pgm("archivo_encriptado.bin");
    unsigned char *cifrado = leer_archivo("archivo_encriptado.bin", cabecera, &tamano_datos, &cabecera_len, esPGM);
    if (!cifrado) return 1;
    //4.Desencriptar mensaje con dinámica inversa
    printf("\n----- Iniciando proceso de desencriptado -----\n");
    size_t tamano_salida = 0;
    unsigned char *mensaje = desencriptar_texto(cifrado, tamano_datos, llave_acotada, &tamano_salida);
    if (!mensaje){
        printf("Error: desencriptar_texto() falló.\n");
        free(cifrado);
        return 1;
    }
    //5. Guardar el archivo desencriptado
    char nombre_salida[512];
    snprintf(nombre_salida, sizeof(nombre_salida), "archivo_recuperado.%s", extension_salida);
    if (guardar_archivo(nombre_salida, esPGM ? cabecera : NULL, esPGM ? cabecera_len : 0, mensaje, tamano_salida) != 0) {
        printf("Error: No se pudo guardar '%s'\n", nombre_salida);
        free(cifrado);
        free(mensaje);
        return 1;
    }

    printf("\n Archivo desencriptado guardado como '%s'\n", nombre_salida);
    //6. Liberar memoria
    free(cifrado);
    free(mensaje);
    return 0;
}

