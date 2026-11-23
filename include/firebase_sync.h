
#pragma once

#include <Arduino.h>

// ========================================
// ANDESCHESS LIVE
// Sincronizacion con Firestore
// ========================================

enum class EstadoPartidaCloud {
    WAITING,
    READY,
    PLAYING,
    PAUSED,
    FINISHED
};

// Datos que el reloj enviara a Firebase.
struct EstadoRelojCloud {
    uint32_t whiteMs;
    uint32_t blackMs;

    EstadoPartidaCloud status;

    // 0 = Ninguno
    // 1 = Blancas
    // 2 = Negras
    uint8_t activePlayer;
    uint64_t sampledAtMs;
};

// Inicializacion del modulo.
void iniciarFirebaseSync();

// Recibe una copia del estado del reloj.
// No debe bloquear los temporizadores.
void actualizarEstadoCloud(
    const EstadoRelojCloud& estado
);
