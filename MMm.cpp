/* Definiciones externas para el sistema de colas simple */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.cpp"  /* Encabezado para el generador de numeros aleatorios */

#define LIMITE_COLA 1000000 /* Capacidad maxima de la cola */
#define OCUPADO      1  /* Indicador de Servidor Ocupado */
#define LIBRE      0  /* Indicador de Servidor Libre */
#define NUM_SERVERS   5 /* Numero de servidores */

int   sig_tipo_evento, num_clientes_espera, num_esperas_requerido, num_eventos,
      num_entra_cola, sig_servidor_salida, estado_servidores[NUM_SERVERS];
float area_num_entra_cola, area_estado_servidores[NUM_SERVERS], media_entre_llegadas, media_atencion,
      tiempo_simulacion, tiempo_llegada[LIMITE_COLA + 1], tiempo_salida[NUM_SERVERS],
      tiempo_ultimo_evento, tiempo_sig_evento[3], total_de_esperas, area_estado_servidores_suma,
      area_hay_cola,area_no_hay_servers;
bool  servidores_libres = true;
FILE  *parametros, *resultados;

void  inicializar(void);
void  controltiempo(void);
void  llegada(void);
void  salida(int servidor_actual);
void  reportes(void);
void  actualizar_estad_prom_tiempo(void);
float expon(float mean);


int main(void)  /*6 Funcion Principal */
{
    /* Abre los archivos de entrada y salida */

    parametros  = fopen("paramSDM.txt",  "r");
    resultados = fopen("resultSDM.txt", "w");

    /* Especifica el numero de eventos para la funcion controltiempo. */

    num_eventos = 2;

    /* Lee los parametros de enrtrada. */

    fscanf(parametros, "%f %f %d", &media_entre_llegadas, &media_atencion,
           &num_esperas_requerido);

    /* Escribe en el archivo de salida los encabezados del reporte y los parametros iniciales */

    fprintf(resultados, "Sistema de Colas Simple\n\n");
    fprintf(resultados, "Tiempo promedio de llegada%11.3f minutos\n\n",
            media_entre_llegadas);
    fprintf(resultados, "Tiempo promedio de atencion%16.3f minutos\n\n", media_atencion);
    fprintf(resultados, "Numero de clientes%14d\n\n", num_esperas_requerido);
    fprintf(resultados, "Numero de servidores%14i\n\n", NUM_SERVERS);

    /* iInicializa la simulacion. */

    inicializar();

    /* Corre la simulacion mientras no se llegue al numero de clientes especificaco en el archivo de entrada*/

    while (num_clientes_espera < num_esperas_requerido) {

        /* Determina el siguiente evento */

        controltiempo();

        

        /* Actualiza los acumuladores estadisticos de tiempo promedio */

        actualizar_estad_prom_tiempo();

        /* Invoca la funcion del evento adecuado. */
    
        switch (sig_tipo_evento) {
            
            case 1:
                llegada();
                break;
            case 2:
                salida(sig_servidor_salida);
                break;
        }
    }

    /* Invoca el generador de reportes y termina la simulacion. */

    reportes();

    fclose(parametros);
    fclose(resultados);

    return 0;
}


void inicializar(void)  /*1 Funcion de inicializacion. */
{
    /* Inicializa el reloj de la simulacion. */

    int   i;
    tiempo_simulacion = 0.0;

    /* Inicializa las variables de estado */

    num_entra_cola       = 0;
    tiempo_ultimo_evento = 0.0;
    sig_servidor_salida = 0;
    for(i = 0; i < NUM_SERVERS; i++) {
        estado_servidores[i] = LIBRE;
        tiempo_salida[i] = 1.0e+29;
    }

    /* Inicializa los contadores estadisticos. */

    num_clientes_espera = 0;
    total_de_esperas    = 0.0;
    area_num_entra_cola = 0.0;
    for(i = 0; i < NUM_SERVERS; i++) {
        area_estado_servidores[i] = 0.0;
    }

    /* Inicializa la lista de eventos. Ya que no hay clientes, el evento salida
       (terminacion del servicio) no se tiene en cuenta */
    
    tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
    tiempo_sig_evento[2] = 1.0e+30;
}


void controltiempo(void)  /*2. Funcion controltiempo */
{
    int   i;
    float min_tiempo_sig_evento = 1.0e+29;
    tiempo_sig_evento[2] = 1.0e+30; // Por ahora; revisar salida

    sig_tipo_evento = 0;

    /* Calcula la salida mas proxima a ocurrir */
    for (i = 0; i < NUM_SERVERS; i++){
        if (tiempo_salida[i] < tiempo_sig_evento[2]) {
            tiempo_sig_evento[2] = tiempo_salida[i];
            sig_servidor_salida = i;
        }
    }
    
    /*  Determina el tipo de evento del evento que debe ocurrir. */

    for (i = 1; i <= num_eventos; ++i) {
        if (tiempo_sig_evento[i] < min_tiempo_sig_evento) {
            min_tiempo_sig_evento = tiempo_sig_evento[i];
            sig_tipo_evento     = i;
        }
    }

    /* Revisa si la lista de eventos esta vacia. */

    if (sig_tipo_evento == 0) {

        /* La lista de eventos esta vacia, se detiene la simulacion. */

        fprintf(resultados, "\nLa lista de eventos esta vacia %f", tiempo_simulacion);
        exit(1);
    }

    /* La lista de eventos no esta vacia, adelanta el reloj de la simulacion. */

    tiempo_simulacion = min_tiempo_sig_evento;
}


void llegada(void)  /*3. Funcion de llegada */
{
    int   i;
    float espera;
    servidores_libres = false;
    int servidor_libre;
    
    /* Programa la siguiente llegada. */
    tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
    // printf("\n llegada  cliente# %d - %f",num_cliente, tiempo_simulacion);
    // num_cliente++;

    /* Revisa si algun servidor esta LIBRE. */
    for(i = 0; i < NUM_SERVERS; i++) {
        
        if (estado_servidores[i] == LIBRE) {
            servidores_libres = true;
            servidor_libre = i;
            break;
        }
    }

    if(servidores_libres == false) {
        ++num_entra_cola;

        /* Verifica si hay condici�n de desbordamiento */

        if (num_entra_cola > LIMITE_COLA) {

            /* Se ha desbordado la cola, detiene la simulacion */

            fprintf(resultados, "\nDesbordamiento del arreglo tiempo_llegada a la hora");
            fprintf(resultados, "%f", tiempo_simulacion);
            exit(2);
        }

        /* Todavia hay espacio en la cola, se almacena el tiempo de llegada del
        	cliente en el ( nuevo ) fin de tiempo_llegada */

        tiempo_llegada[num_entra_cola] = tiempo_simulacion;
    }
    
    else {

        espera            = 0.0;
        total_de_esperas += espera;

        /* Incrementa el numero de clientes en espera, y pasa el servidor a ocupado */
        ++num_clientes_espera;
        estado_servidores[servidor_libre] = OCUPADO;

        /* Programa una salida ( servicio terminado ) */
        tiempo_salida[servidor_libre] = tiempo_simulacion + expon(media_atencion);
        
        //printf("\n salida cliente# %d - %f",num_cliente_atendido, tiempo_simulacion);
        //num_cliente_atendido++;
        
    }

}



void salida(int servidor_actual)  /*3 Funcion de Salida. */
{
    /* Programa la siguiente llegada. */
    int   i;
    float espera;

    
    /* Revisa si la cola esta vacia */

    if (num_entra_cola == 0) {

        /* La cola esta vacia, pasa el servidor a LIBRE y
        no considera el evento de salida */
        estado_servidores[servidor_actual] = LIBRE;
        servidores_libres = true;
        tiempo_salida[servidor_actual] = 1.0e+30;
        tiempo_sig_evento[2] = 1.0e+30;
    }

    else {

        /* La cola no esta vacia, disminuye el numero de clientes en cola. */
        --num_entra_cola;

        /* Calcula la espera del cliente que esta siendo atendido y
        actualiza el acumulador de espera. */

        espera            = tiempo_simulacion - tiempo_llegada[1];
        total_de_esperas += espera;

        /* Incrementa el numero de clientes en espera, y programa la salida. */   
        ++num_clientes_espera;
        
        tiempo_salida[servidor_actual] = tiempo_simulacion + expon(media_atencion);
        /* Mueve cada cliente en la cola ( si los hay ) una posicion hacia adelante */
        for (i = 1; i <= num_entra_cola; ++i)
            tiempo_llegada[i] = tiempo_llegada[i + 1];
    }
}


void reportes(void)  /*5 Funcion generadora de reportes. */
{
    int i;
    area_estado_servidores_suma = 0.0;

    /* Calcula y estima los estimados de las medidas deseadas de desempenio */
    for(i = 0; i < NUM_SERVERS; i++) {
        area_estado_servidores_suma += area_estado_servidores[i] / tiempo_simulacion;
    }

    fprintf(resultados, "\n\nEspera promedio en la cola%11.3f minutos\n\n",
            total_de_esperas / num_clientes_espera);
    fprintf(resultados, "Numero promedio en cola%10.3f\n\n",
            area_num_entra_cola / tiempo_simulacion);
    fprintf(resultados, "Uso de los servidores%15.3f\n\n",
            area_estado_servidores_suma / NUM_SERVERS);
    fprintf(resultados, "Tiempo de terminacion de la simulacion%12.3f minutos\n\n", tiempo_simulacion);
    // fprintf(resultados, "Probabilidad de que haya cola:%12.6f\n\n",
    //         area_hay_cola / tiempo_simulacion);
    fprintf(resultados, "Probabilidad de que los servidores estan ocupados:%12.6f\n\n",
            area_no_hay_servers / tiempo_simulacion);

    printf( "Server 1%16.3f minutos\n\n", area_estado_servidores[0]);
    printf( "Server 2%16.3f minutos\n\n", area_estado_servidores[1]);
}


void actualizar_estad_prom_tiempo(void)  /*2 Actualiza los acumuladores de area para las estadisticas de tiempo promedio. */
{
    float time_since_last_event;
    int i;

    /* Calcula el tiempo desde el ultimo evento, y actualiza el marcador
    	del ultimo evento */

    time_since_last_event = tiempo_simulacion - tiempo_ultimo_evento;
    tiempo_ultimo_evento = tiempo_simulacion;

    /* Actualiza el area bajo la funcion de numero_en_cola */
    area_num_entra_cola += num_entra_cola * time_since_last_event;
    // int hay_cola = num_entra_cola==0 ? 0 : 1;
    // area_hay_cola += hay_cola * time_since_last_event;
    int hay_servers_libres =  servidores_libres == true ? 0 : 1; 
    area_no_hay_servers += hay_servers_libres * time_since_last_event;

    /* Actualiza el area bajo la funcion indicadora de cada servidor ocupado*/
    for(i = 0; i < NUM_SERVERS; i++) {
        area_estado_servidores[i] += estado_servidores[i] * time_since_last_event;
        //printf( "Server %16.3i  : %16.3f \n\n", area_estado_servidores[0]);
    }
}


float expon(float media)  /*4 Funcion generadora de la exponencias */
{
    return -media * log(lcgrand(1));
}

