
#pragma once

#include <Arduino.h>

// =====================================
// ANDESCHESS LIVE
// Firebase Authentication
// =====================================

// Inicializa el modulo.
void iniciarFirebaseAuth();

// Indica si existe una sesion autenticada.
bool firebaseAutenticado();

// Obtiene una copia del token de Firebase.
// La implementacion protegera el acceso
// entre las tareas de FreeRTOS.
bool obtenerFirebaseToken(String& token);
