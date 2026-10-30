
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ====================================
// ESP32 CHESS CLOCK - VERSION 0.2
// DOS OLED + DOS PULSADORES
// ====================================

// Pantallas
constexpr int ANCHO = 128;
constexpr int ALTO = 64;
constexpr uint8_t DIRECCION = 0x3C;

// Pulsadores
constexpr uint8_t BOTON_J1 = 25;
constexpr uint8_t BOTON_J2 = 26;

// Configuracion del reloj
constexpr uint32_t TIEMPO_INICIAL = 300000UL;
constexpr uint32_t REBOTE_MS = 50;

// Buses I2C independientes
TwoWire I2C_1 = TwoWire(0);
TwoWire I2C_2 = TwoWire(1);

Adafruit_SSD1306 oled1(ANCHO, ALTO, &I2C_1, -1);
Adafruit_SSD1306 oled2(ANCHO, ALTO, &I2C_2, -1);

// Estados
enum Estado {
    ESPERANDO,
    TURNO_J1,
    TURNO_J2,
    FINALIZADO
};

Estado estado = ESPERANDO;

// Temporizadores
uint32_t tiempoJ1 = TIEMPO_INICIAL;
uint32_t tiempoJ2 = TIEMPO_INICIAL;

uint32_t ultimoTick = 0;
uint32_t ultimaActualizacion = 0;
uint32_t ultimoSerial = 0;

// Estructura de pulsadores
struct Boton {
    uint8_t pin;
    bool lecturaAnterior;
    bool estadoEstable;
    uint32_t ultimoCambio;

    Boton(uint8_t numeroPin) {
        pin = numeroPin;
        lecturaAnterior = HIGH;
        estadoEstable = HIGH;
        ultimoCambio = 0;
    }
};

Boton boton1(BOTON_J1);
Boton boton2(BOTON_J2);

// ====================================
// DETECCION DE PULSACIONES
// ====================================

bool fuePresionado(Boton &boton) {
    bool lectura = digitalRead(boton.pin);
    uint32_t ahora = millis();

    if (lectura != boton.lecturaAnterior) {
        boton.ultimoCambio = ahora;
    }

    boton.lecturaAnterior = lectura;

    if (ahora - boton.ultimoCambio >= REBOTE_MS) {
        if (lectura != boton.estadoEstable) {
            boton.estadoEstable = lectura;
            return lectura == LOW;
        }
    }

    return false;
}

// ====================================
// ACTUALIZAR TIEMPO
// ====================================

void actualizarTiempo(uint32_t ahora) {
    uint32_t transcurrido = ahora - ultimoTick;
    ultimoTick = ahora;

    uint32_t *tiempoActivo = nullptr;

    if (estado == TURNO_J1) {
        tiempoActivo = &tiempoJ1;
    } else if (estado == TURNO_J2) {
        tiempoActivo = &tiempoJ2;
    }

    if (tiempoActivo == nullptr) return;

    if (transcurrido >= *tiempoActivo) {
        *tiempoActivo = 0;
        estado = FINALIZADO;
        Serial.println("TIEMPO AGOTADO");
    } else {
        *tiempoActivo -= transcurrido;
    }
}

// ====================================
// DIBUJAR RELOJ EN OLED
// ====================================

void dibujarReloj(
    Adafruit_SSD1306 &pantalla,
    uint8_t jugador,
    uint32_t tiempo
) {
    pantalla.clearDisplay();
    pantalla.setTextColor(SSD1306_WHITE);

    // Nombre del jugador
    pantalla.setTextSize(1);
    pantalla.setCursor(35, 3);
    pantalla.print("JUGADOR ");
    pantalla.print(jugador);

    // Separador
    pantalla.drawLine(
        0, 16, 127, 16, SSD1306_WHITE
    );

    // Convertir milisegundos a MM:SS
    uint32_t totalSegundos = (tiempo + 999) / 1000;
    uint32_t minutos = totalSegundos / 60;
    uint32_t segundos = totalSegundos % 60;

    char texto[12];
    snprintf(
        texto,
        sizeof(texto),
        "%02lu:%02lu",
        (unsigned long)minutos,
        (unsigned long)segundos
    );

    // Tiempo centrado
    pantalla.setTextSize(3);
    pantalla.setCursor(19, 23);
    pantalla.print(texto);

    // Estado del jugador
    pantalla.setTextSize(1);
    pantalla.setCursor(32, 55);

    bool activo =
        (jugador == 1 && estado == TURNO_J1) ||
        (jugador == 2 && estado == TURNO_J2);

    if (estado == FINALIZADO && tiempo == 0) {
        pantalla.print("SIN TIEMPO");
    } else if (activo) {
        pantalla.print("CORRIENDO");
    } else {
        pantalla.print("EN ESPERA");
    }

    pantalla.display();
}

void actualizarPantallas() {
    dibujarReloj(oled1, 1, tiempoJ1);
    dibujarReloj(oled2, 2, tiempoJ2);
}

// ====================================
// CAMBIAR TURNO
// ====================================

void procesarBotones(uint32_t ahora) {
    bool pulsadoJ1 = fuePresionado(boton1);
    bool pulsadoJ2 = fuePresionado(boton2);

    if (estado == FINALIZADO) return;

    // Ignorar dos pulsaciones simultaneas
    if (pulsadoJ1 && pulsadoJ2) return;

    // J1 termina su turno
    if (pulsadoJ1 &&
        (estado == ESPERANDO || estado == TURNO_J1)) {
        estado = TURNO_J2;
        ultimoTick = ahora;
        Serial.println("Turno del jugador 2");
    }

    // J2 termina su turno
    else if (pulsadoJ2 &&
        (estado == ESPERANDO || estado == TURNO_J2)) {
        estado = TURNO_J1;
        ultimoTick = ahora;
        Serial.println("Turno del jugador 1");
    }
}

// ====================================
// SETUP
// ====================================

void setup() {
    Serial.begin(115200);

    pinMode(BOTON_J1, INPUT_PULLUP);
    pinMode(BOTON_J2, INPUT_PULLUP);

    // Iniciar los dos buses
    I2C_1.begin(21, 22, 100000);
    I2C_2.begin(18, 19, 100000);

    // Iniciar OLED 1
    if (!oled1.begin(
        SSD1306_SWITCHCAPVCC,
        DIRECCION, true, false
    )) {
        Serial.println("ERROR OLED 1");
        while (true) delay(1000);
    }

    // Iniciar OLED 2
    if (!oled2.begin(
        SSD1306_SWITCHCAPVCC,
        DIRECCION, true, false
    )) {
        Serial.println("ERROR OLED 2");
        while (true) delay(1000);
    }

    ultimoTick = millis();
    ultimaActualizacion = ultimoTick;
    ultimoSerial = ultimoTick;

    actualizarPantallas();

    Serial.println("ESP32 CHESS CLOCK V0.2");
    Serial.println("DOS OLED INICIADAS");
    Serial.println("Esperando primera pulsacion");
}

// ====================================
// LOOP PRINCIPAL
// ====================================

void loop() {
    uint32_t ahora = millis();

    actualizarTiempo(ahora);
    procesarBotones(ahora);

    // Refrescar pantallas cada 100 ms
    if (ahora - ultimaActualizacion >= 100) {
        ultimaActualizacion = ahora;
        actualizarPantallas();
    }

    // Diagnostico por Monitor Serie
    if (ahora - ultimoSerial >= 1000) {
        ultimoSerial = ahora;

        Serial.print("J1: ");
        Serial.print(tiempoJ1 / 1000);

        Serial.print(" s | J2: ");
        Serial.print(tiempoJ2 / 1000);

        Serial.println(" s");
    }

    delay(1);
}
