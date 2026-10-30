
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ESP32 CHESS CLOCK - VERSION 0.3
// Dos OLED + dos botones + pausa y reset

constexpr uint8_t BOTON_J1 = 25;
constexpr uint8_t BOTON_J2 = 26;
constexpr uint8_t BOTON_CONTROL = 27;

constexpr uint32_t TIEMPO_INICIAL = 300000UL;
constexpr uint32_t REBOTE_MS = 50;
constexpr uint32_t RESET_MS = 3000;

constexpr uint8_t OLED_ADDR = 0x3C;
constexpr int ANCHO = 128;
constexpr int ALTO = 64;

// Buses I2C independientes
TwoWire I2C_1(0);
TwoWire I2C_2(1);

Adafruit_SSD1306 oled1(ANCHO, ALTO, &I2C_1, -1);
Adafruit_SSD1306 oled2(ANCHO, ALTO, &I2C_2, -1);

enum Estado {
    ESPERANDO,
    TURNO_J1,
    TURNO_J2,
    PAUSADO,
    FINALIZADO
};

Estado estado = ESPERANDO;
Estado estadoAntesPausa = ESPERANDO;

uint32_t tiempoJ1 = TIEMPO_INICIAL;
uint32_t tiempoJ2 = TIEMPO_INICIAL;

uint32_t ultimoTick = 0;
uint32_t ultimaPantalla = 0;
uint32_t ultimoSerial = 0;

struct Boton {
    uint8_t pin;
    bool lecturaAnterior;
    bool estadoEstable;
    uint32_t ultimoCambio;

    // Constructor del boton
    Boton(uint8_t numeroPin) {
        pin = numeroPin;
        lecturaAnterior = HIGH;
        estadoEstable = HIGH;
        ultimoCambio = 0;
    }
};

// Inicializar los tres botones
Boton boton1(BOTON_J1);
Boton boton2(BOTON_J2);
Boton control(BOTON_CONTROL);


// Detecta cambios estables
// Retorna true cuando cambia de estado
bool actualizarBoton(Boton &b) {
    bool lectura = digitalRead(b.pin);
    uint32_t ahora = millis();

    if (lectura != b.lecturaAnterior) {
        b.ultimoCambio = ahora;
    }

    b.lecturaAnterior = lectura;

    if (ahora - b.ultimoCambio >= REBOTE_MS &&
        lectura != b.estadoEstable) {
        b.estadoEstable = lectura;
        return true;
    }

    return false;
}

// Control del tercer boton
uint32_t inicioControl = 0;
bool resetEjecutado = false;

void reiniciarReloj() {
    tiempoJ1 = TIEMPO_INICIAL;
    tiempoJ2 = TIEMPO_INICIAL;

    estado = ESPERANDO;
    estadoAntesPausa = ESPERANDO;

    ultimoTick = millis();

    Serial.println("RELOJ REINICIADO");
}

void alternarPausa() {
    if (estado == PAUSADO) {
        estado = estadoAntesPausa;
        ultimoTick = millis();
        Serial.println("RELOJ REANUDADO");
    }
    else if (estado == TURNO_J1 ||
             estado == TURNO_J2) {
        estadoAntesPausa = estado;
        estado = PAUSADO;
        Serial.println("RELOJ PAUSADO");
    }
}

void procesarControl(uint32_t ahora) {
    if (actualizarBoton(control)) {
        if (control.estadoEstable == LOW) {
            inicioControl = ahora;
            resetEjecutado = false;
        }
        else {
            if (!resetEjecutado) {
                alternarPausa();
            }
        }
    }

    // Mantener pulsado 3 segundos
    if (control.estadoEstable == LOW &&
        !resetEjecutado &&
        ahora - inicioControl >= RESET_MS) {

        reiniciarReloj();
        resetEjecutado = true;
    }
}

void actualizarTiempo(uint32_t ahora) {
    uint32_t transcurrido = ahora - ultimoTick;
    ultimoTick = ahora;

    uint32_t *activo = nullptr;

    if (estado == TURNO_J1) {
        activo = &tiempoJ1;
    }
    else if (estado == TURNO_J2) {
        activo = &tiempoJ2;
    }

    if (activo == nullptr) return;

    if (transcurrido >= *activo) {
        *activo = 0;
        estado = FINALIZADO;
        Serial.println("TIEMPO AGOTADO");
    }
    else {
        *activo -= transcurrido;
    }
}

void procesarJugadores() {
    bool evento1 = actualizarBoton(boton1);
    bool evento2 = actualizarBoton(boton2);

    bool j1 = evento1 && boton1.estadoEstable == LOW;
    bool j2 = evento2 && boton2.estadoEstable == LOW;

    if (estado == PAUSADO || estado == FINALIZADO) {
        return;
    }

    if (control.estadoEstable == LOW) return;
    if (j1 && j2) return;

    if (j1 && (estado == ESPERANDO ||
               estado == TURNO_J1)) {
        estado = TURNO_J2;
        Serial.println("TURNO JUGADOR 2");
    }
    else if (j2 && (estado == ESPERANDO ||
                    estado == TURNO_J2)) {
        estado = TURNO_J1;
        Serial.println("TURNO JUGADOR 1");
    }
}

// Dibujar tiempo y estado
void dibujarReloj(
    Adafruit_SSD1306 &oled,
    uint8_t jugador,
    uint32_t tiempo
) {
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);

    oled.setTextSize(1);
    oled.setCursor(34, 3);
    oled.print("JUGADOR ");
    oled.print(jugador);

    oled.drawLine(0, 16, 127, 16, SSD1306_WHITE);

    uint32_t total = (tiempo + 999) / 1000;
    uint32_t minutos = total / 60;
    uint32_t segundos = total % 60;

    char texto[12];
    snprintf(texto, sizeof(texto), "%02lu:%02lu",
             (unsigned long)minutos,
             (unsigned long)segundos);

    oled.setTextSize(3);
    oled.setCursor(19, 23);
    oled.print(texto);

    oled.setTextSize(1);

    bool activo = (jugador == 1 && estado == TURNO_J1) ||
                  (jugador == 2 && estado == TURNO_J2);

    const char *mensaje = "EN ESPERA";

    if (estado == PAUSADO) {
        mensaje = "PAUSADO";
    }
    else if (estado == FINALIZADO && tiempo == 0) {
        mensaje = "SIN TIEMPO";
    }
    else if (activo) {
        mensaje = "CORRIENDO";
    }

    int anchoTexto = strlen(mensaje) * 6;
    oled.setCursor((128 - anchoTexto) / 2, 55);
    oled.print(mensaje);

    oled.display();
}

void actualizarPantallas() {
    dibujarReloj(oled1, 1, tiempoJ1);
    dibujarReloj(oled2, 2, tiempoJ2);
}

void setup() {
    Serial.begin(115200);

    pinMode(BOTON_J1, INPUT_PULLUP);
    pinMode(BOTON_J2, INPUT_PULLUP);
    pinMode(BOTON_CONTROL, INPUT_PULLUP);

    I2C_1.begin(21, 22, 100000);
    I2C_2.begin(18, 19, 100000);

    if (!oled1.begin(
        SSD1306_SWITCHCAPVCC, OLED_ADDR, true, false)) {
        Serial.println("ERROR OLED 1");
        while (true) delay(1000);
    }

    if (!oled2.begin(
        SSD1306_SWITCHCAPVCC, OLED_ADDR, true, false)) {
        Serial.println("ERROR OLED 2");
        while (true) delay(1000);
    }

    ultimoTick = millis();
    ultimaPantalla = ultimoTick;
    ultimoSerial = ultimoTick;

    actualizarPantallas();

    Serial.println("ESP32 CHESS CLOCK V0.3");
    Serial.println("Dos OLED y tres botones");
    Serial.println("Control corto: pausa/reanudar");
    Serial.println("Control 3 segundos: reset");
}

void loop() {
    uint32_t ahora = millis();

    actualizarTiempo(ahora);
    procesarControl(ahora);
    procesarJugadores();

    if (ahora - ultimaPantalla >= 100) {
        ultimaPantalla = ahora;
        actualizarPantallas();
    }

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
