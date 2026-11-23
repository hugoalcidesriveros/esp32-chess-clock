
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <WiFi.h>
#include "wifi_secrets.h"

// =====================================
// ESP32 CHESS CLOCK - VERSION 0.4
// Menu + Fischer + pausa + reinicio
// =====================================

constexpr uint8_t PIN_J1 = 25;
constexpr uint8_t PIN_J2 = 26;
constexpr uint8_t PIN_CENTRAL = 27;

constexpr uint8_t OLED_ADDR = 0x3C;
constexpr uint32_t REBOTE_MS = 50;
constexpr uint32_t VOLVER_MS = 2000;
constexpr uint32_t RESET_MS = 3000;

// =====================================
// ANDESCHESS LIVE - WIFI
// =====================================

constexpr uint32_t WIFI_REINTENTO_MS = 15000;

uint32_t ultimoIntentoWiFi = 0;
bool wifiConectadoAnterior = false;

TwoWire BUS1(0);
TwoWire BUS2(1);

Adafruit_SSD1306 oled1(128, 64, &BUS1, -1);
Adafruit_SSD1306 oled2(128, 64, &BUS2, -1);

// =====================================
// RITMOS DISPONIBLES
// =====================================

struct Ritmo {
    uint16_t minutos;
    uint8_t incremento;
};

const Ritmo ritmos[] = {
    {3, 2},
    {5, 0},
    {10, 5},
    {15, 10},
    {30, 0}
};

constexpr uint8_t CANT_RITMOS =
    sizeof(ritmos) / sizeof(ritmos[0]);

uint8_t ritmoActual = 1; // 5+0 inicial

// =====================================
// ESTADOS DEL RELOJ
// =====================================

enum Estado {
    MENU,
    CONFIRMADO,
    LISTO,
    JUGANDO_J1,
    JUGANDO_J2,
    PAUSADO,
    FINALIZADO
};

Estado estado = MENU;
Estado estadoAntesPausa = LISTO;

uint32_t tiempoJ1 = 300000;
uint32_t tiempoJ2 = 300000;
uint32_t ultimoTick = 0;
uint32_t ultimaPantalla = 0;

// Ultima posicion del mecanismo
// 0 = desconocida
// 1 = izquierdo abajo
// 2 = derecho abajo
uint8_t ultimaPosicion = 0;

// =====================================
// BOTONES CON ANTIRREBOTE
// =====================================

struct Boton {
    uint8_t pin;
    bool anterior;
    bool estable;
    uint32_t cambio;

    Boton(uint8_t p) {
        pin = p;
        anterior = HIGH;
        estable = HIGH;
        cambio = 0;
    }
};

Boton boton1(PIN_J1);
Boton boton2(PIN_J2);
Boton central(PIN_CENTRAL);

void iniciarBoton(Boton &b) {
    pinMode(b.pin, INPUT_PULLUP);

    // Leer posicion fisica inicial
    b.anterior = digitalRead(b.pin);
    b.estable = b.anterior;
    b.cambio = millis();
}

bool actualizarBoton(Boton &b, uint32_t ahora) {
    bool lectura = digitalRead(b.pin);

    if (lectura != b.anterior) {
        b.cambio = ahora;
    }

    b.anterior = lectura;

    if (ahora - b.cambio >= REBOTE_MS &&
        lectura != b.estable) {
        b.estable = lectura;
        return true;
    }

    return false;
}

// =====================================
// CONFIGURACION DE TIEMPO
// =====================================

void cargarTiempoElegido() {
    uint32_t inicial =
        (uint32_t)ritmos[ritmoActual].minutos * 60000UL;

    tiempoJ1 = inicial;
    tiempoJ2 = inicial;
}

void reiniciarPartida() {
    cargarTiempoElegido();
    estado = LISTO;
    estadoAntesPausa = LISTO;
    ultimoTick = millis();

    Serial.println("PARTIDA REINICIADA");
}

void siguienteRitmo() {
    ritmoActual = (ritmoActual + 1) % CANT_RITMOS;

    Serial.print("Ritmo: ");
    Serial.print(ritmos[ritmoActual].minutos);
    Serial.print("+");
    Serial.println(ritmos[ritmoActual].incremento);
}

// =====================================
// TEMPORIZADORES
// =====================================

void actualizarTiempo(uint32_t ahora) {
    uint32_t delta = ahora - ultimoTick;
    ultimoTick = ahora;

    uint32_t *activo = nullptr;

    if (estado == JUGANDO_J1) {
        activo = &tiempoJ1;
    } else if (estado == JUGANDO_J2) {
        activo = &tiempoJ2;
    }

    if (!activo) return;

    if (delta >= *activo) {
        *activo = 0;
        estado = FINALIZADO;
        Serial.println("TIEMPO AGOTADO");
    } else {
        *activo -= delta;
    }
}

void cambiarTurno(uint8_t lado) {
    if (estado == LISTO) {
        estado = (lado == 1)
            ? JUGANDO_J2
            : JUGANDO_J1;

        Serial.println("PARTIDA INICIADA");
        return;
    }

    uint32_t incremento =
        (uint32_t)ritmos[ritmoActual].incremento * 1000UL;

    if (lado == 1 && estado == JUGANDO_J1) {
        // J1 termino su jugada
        tiempoJ1 += incremento;
        estado = JUGANDO_J2;
    }
    else if (lado == 2 && estado == JUGANDO_J2) {
        // J2 termino su jugada
        tiempoJ2 += incremento;
        estado = JUGANDO_J1;
    }
}

// =====================================
// CONTROL DE BOTONES GRANDES
// =====================================

void procesarLaterales(uint32_t ahora) {
    bool cambio1 = actualizarBoton(boton1, ahora);
    bool cambio2 = actualizarBoton(boton2, ahora);

    // Solo nos interesan nuevas activaciones
    bool presionado1 = cambio1 && boton1.estable == LOW;
    bool presionado2 = cambio2 && boton2.estable == LOW;

    if (central.estable == LOW) return;

    // Ignorar pulsaciones simultaneas
    if (presionado1 && presionado2) return;

    uint8_t posicion = 0;

    if (presionado1) posicion = 1;
    if (presionado2) posicion = 2;

    if (posicion == 0) return;

    // El mecanismo debe alternar de lado
    if (posicion == ultimaPosicion) return;

    // En modo menu: cambiar ritmo
    if (estado == MENU) {
        ultimaPosicion = posicion;
        siguienteRitmo();
        return;
    }

    // En modo partida: cambiar turno
    if (estado == LISTO ||
        estado == JUGANDO_J1 ||
        estado == JUGANDO_J2) {

        // No aceptar una posicion repetida
        if (estado == JUGANDO_J1 && posicion != 1)
            return;

        if (estado == JUGANDO_J2 && posicion != 2)
            return;

        ultimaPosicion = posicion;
        cambiarTurno(posicion);
    }
}

// =====================================
// CONTROL CENTRAL
// =====================================

uint32_t inicioCentral = 0;
bool accionLarga = false;

void accionCortaCentral() {
    switch (estado) {
        case MENU:
            estado = CONFIRMADO;
            Serial.println("RITMO CONFIRMADO");
            break;

        case CONFIRMADO:
            cargarTiempoElegido();
            estado = LISTO;
            ultimoTick = millis();
            Serial.println("RELOJ LISTO");
            break;

        case JUGANDO_J1:
        case JUGANDO_J2:
            estadoAntesPausa = estado;
            estado = PAUSADO;
            Serial.println("PAUSA");
            break;

        case PAUSADO:
            estado = estadoAntesPausa;
            ultimoTick = millis();
            Serial.println("REANUDADO");
            break;

        default:
            break;
    }
}

void procesarCentral(uint32_t ahora) {
    bool cambio = actualizarBoton(central, ahora);

    if (cambio) {
        if (central.estable == LOW) {
            inicioCentral = ahora;
            accionLarga = false;
        } else if (!accionLarga) {
            accionCortaCentral();
        }
    }

    if (central.estable != LOW || accionLarga)
        return;

    uint32_t duracion = ahora - inicioCentral;

    // Volver al menu desde confirmacion
    if (estado == CONFIRMADO &&
        duracion >= VOLVER_MS) {

        estado = MENU;
        accionLarga = true;
        Serial.println("VOLVER AL MENU");
    }

    // Reiniciar durante la partida o al finalizar
    else if (
        (estado == LISTO ||
         estado == JUGANDO_J1 ||
         estado == JUGANDO_J2 ||
         estado == PAUSADO ||
         estado == FINALIZADO) &&
        duracion >= RESET_MS) {

        reiniciarPartida();
        accionLarga = true;
    }
}

// =====================================
// DIBUJAR PANTALLAS
// =====================================

void dibujarTiempo(
    Adafruit_SSD1306 &oled,
    uint8_t jugador,
    uint32_t tiempo
) {
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(1);

    oled.setCursor(35, 3);
    oled.print("JUGADOR ");
    oled.print(jugador);

    oled.drawLine(0, 16, 127, 16, SSD1306_WHITE);

    uint32_t total = (tiempo + 999) / 1000;
    uint32_t minutos = total / 60;
    uint32_t segundos = total % 60;

    char reloj[12];
    snprintf(reloj, sizeof(reloj), "%02lu:%02lu",
        (unsigned long)minutos,
        (unsigned long)segundos);

    oled.setTextSize(3);
    oled.setCursor(19, 23);
    oled.print(reloj);

    oled.setTextSize(1);

    const char *mensaje = "EN ESPERA";

    if (estado == PAUSADO) {
        mensaje = "PAUSADO";
    }
    else if (estado == FINALIZADO && tiempo == 0) {
        mensaje = "SIN TIEMPO";
    }
    else if ((jugador == 1 && estado == JUGANDO_J1) ||
             (jugador == 2 && estado == JUGANDO_J2)) {
        mensaje = "CORRIENDO";
    }

    oled.setCursor((128 - strlen(mensaje) * 6) / 2, 55);
    oled.print(mensaje);
    oled.display();
}

void dibujarMenu() {
    oled1.clearDisplay();
    oled2.clearDisplay();

    oled1.setTextColor(SSD1306_WHITE);
    oled2.setTextColor(SSD1306_WHITE);

    oled1.setTextSize(1);
    oled1.setCursor(15, 3);
    oled1.print("ELEGIR RITMO");

    oled1.setTextSize(3);
    oled1.setCursor(16, 23);
    oled1.print(ritmos[ritmoActual].minutos);
    oled1.print("+");
    oled1.print(ritmos[ritmoActual].incremento);

    oled1.setTextSize(1);
    oled1.setCursor(12, 55);
    oled1.print("BOTONES GRANDES");

    oled2.setTextSize(1);
    oled2.setCursor(28, 5);
    oled2.print("ESP32 CLOCK");

    oled2.setTextSize(2);
    oled2.setCursor(22, 25);

    if (estado == MENU) {
        oled2.print("ELEGIR");
    } else {
        oled2.print("EMPEZAR");
    }

    oled2.setTextSize(1);
    oled2.setCursor(15, 55);
    oled2.print("BOTON CENTRAL");

    oled1.display();
    oled2.display();
}

void actualizarPantallas() {
    if (estado == MENU || estado == CONFIRMADO) {
        dibujarMenu();
    } else {
        dibujarTiempo(oled1, 1, tiempoJ1);
        dibujarTiempo(oled2, 2, tiempoJ2);
    }
}



void iniciarWiFi() {
    Serial.println();
    Serial.println("ANDESCHESS LIVE - WIFI");
    Serial.println("Iniciando conexion...");

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    ultimoIntentoWiFi = millis();
}

void actualizarWiFi(uint32_t ahora) {
    bool conectado = WiFi.status() == WL_CONNECTED;

    // Detectar nueva conexion
    if (conectado && !wifiConectadoAnterior) {
        Serial.println();
        Serial.println("WIFI CONECTADO");

        Serial.print("Direccion IP: ");
        Serial.println(WiFi.localIP());

        Serial.print("Potencia de senal: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
    }

    // Detectar perdida de conexion
    if (!conectado && wifiConectadoAnterior) {
        Serial.println("WIFI DESCONECTADO");
        Serial.println("El reloj sigue funcionando normalmente");
    }

    wifiConectadoAnterior = conectado;

    // Reintento periodico sin esperar en un bucle
    if (!conectado &&
        ahora - ultimoIntentoWiFi >= WIFI_REINTENTO_MS) {

        ultimoIntentoWiFi = ahora;
        WiFi.reconnect();
    }
}


// =====================================
// INICIALIZACION
// =====================================

void setup() {
    Serial.begin(115200);

    iniciarBoton(boton1);
    iniciarBoton(boton2);
    iniciarBoton(central);

    // Reconocer una posicion inicial
    // sin interpretarla como pulsacion
    if (boton1.estable == LOW && boton2.estable == HIGH)
        ultimaPosicion = 1;
    else if (boton2.estable == LOW && boton1.estable == HIGH)
        ultimaPosicion = 2;

    BUS1.begin(21, 22, 100000);
    BUS2.begin(18, 19, 100000);

    if (!oled1.begin(SSD1306_SWITCHCAPVCC,
                     OLED_ADDR, true, false)) {
        Serial.println("ERROR OLED 1");
        while (true) delay(1000);
    }

    if (!oled2.begin(SSD1306_SWITCHCAPVCC,
                     OLED_ADDR, true, false)) {
        Serial.println("ERROR OLED 2");
        while (true) delay(1000);
    }

    cargarTiempoElegido();

    ultimoTick = millis();
    ultimaPantalla = ultimoTick;

    Serial.println("ESP32 CHESS CLOCK V0.4");
    Serial.println("SELECCIONAR RITMO");
    actualizarPantallas();
    iniciarWiFi();
}

// =====================================
// BUCLE PRINCIPAL
// =====================================

void loop() {
    uint32_t ahora = millis();

    actualizarTiempo(ahora);
    procesarCentral(ahora);
    procesarLaterales(ahora);
    actualizarWiFi(ahora);

    if (ahora - ultimaPantalla >= 100) {
        ultimaPantalla = ahora;
        actualizarPantallas();
    }

    delay(1);
}
