
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "firebase_sync.h"
#include "firebase_auth.h"
#include "firebase_ca.h"

// ========================================
// ANDESCHESS LIVE - FIRESTORE SYNC
// ========================================

static QueueHandle_t colaEstados = nullptr;
static TaskHandle_t tareaSyncHandle = nullptr;

// Documento de nuestra partida de prueba.

static const char *FIRESTORE_URL =
    "https://firestore.googleapis.com/v1/"
    "projects/andeschess-live/"
    "databases/(default)/documents/games/game_001"
    "?updateMask.fieldPaths=whiteMs"
    "&updateMask.fieldPaths=blackMs"
    "&updateMask.fieldPaths=status"
    "&updateMask.fieldPaths=activePlayer"
    "&updateMask.fieldPaths=sampledAtMs";

// ========================================
// ENVIAR TIEMPOS A FIRESTORE
// ========================================

static const char *estadoComoTexto(
    EstadoPartidaCloud estado)
{
    switch (estado)
    {
    case EstadoPartidaCloud::READY:
        return "ready";

    case EstadoPartidaCloud::PLAYING:
        return "playing";

    case EstadoPartidaCloud::PAUSED:
        return "paused";

    case EstadoPartidaCloud::FINISHED:
        return "finished";

    default:
        return "waiting";
    }
}

static bool enviarEstadoFirestore(
    const EstadoRelojCloud &estado)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[SYNC] Wi-Fi no disponible");
        return false;
    }

    String token;

    if (!obtenerFirebaseToken(token))
    {
        Serial.println("[SYNC] Firebase no autenticado");
        return false;
    }

    WiFiClientSecure cliente;
    cliente.setCACert(FIREBASE_CA_CERT);
    cliente.setTimeout(8000);

    HTTPClient http;
    http.setTimeout(8000);

    if (!http.begin(cliente, FIRESTORE_URL))
    {
        Serial.println("[SYNC] Error al iniciar HTTPS");
        return false;
    }

    http.addHeader(
        "Content-Type",
        "application/json");

    http.addHeader(
        "Authorization",
        "Bearer " + token);

    // Formato JSON requerido por Firestore REST.
    JsonDocument documento;

    documento["fields"]["whiteMs"]["integerValue"] =
        String(estado.whiteMs);

    documento["fields"]["blackMs"]["integerValue"] =
        String(estado.blackMs);

    documento["fields"]["status"]["stringValue"] =
        estadoComoTexto(estado.status);

    documento["fields"]["activePlayer"]["integerValue"] =
        String(estado.activePlayer);

    // Convertir el timestamp de 64 bits a texto.
    // Firestore acepta integerValue como cadena.
    char timestampTexto[24];

    snprintf(
        timestampTexto,
        sizeof(timestampTexto),
        "%llu",
        (unsigned long long)estado.sampledAtMs);

    documento["fields"]["sampledAtMs"]["integerValue"] =
        timestampTexto;

    String cuerpo;
    serializeJson(documento, cuerpo);

    // PATCH actualiza solamente los campos
    // indicados en updateMask.
    int codigo = http.sendRequest("PATCH", cuerpo);

    http.end();

    if (codigo == 200)
    {
        Serial.println("[SYNC] Tiempos enviados correctamente");
        return true;
    }

    Serial.printf(
        "[SYNC] Error Firestore HTTP: %d\n",
        codigo);

    // Nunca imprimimos el token.
    return false;
}

// ========================================
// TAREA INDEPENDIENTE
// ========================================

static void tareaFirebaseSync(void *parametro)
{
    EstadoRelojCloud estado;

    Serial.println("[SYNC] Tarea iniciada");

    for (;;)
    {
        if (xQueueReceive(
                colaEstados,
                &estado,
                portMAX_DELAY) == pdTRUE)
        {
            enviarEstadoFirestore(estado);
        }
    }
}

// ========================================
// INICIALIZACION
// ========================================

void iniciarFirebaseSync()
{
    if (colaEstados != nullptr)
    {
        return;
    }

    colaEstados = xQueueCreate(
        1,
        sizeof(EstadoRelojCloud));

    if (colaEstados == nullptr)
    {
        Serial.println("[SYNC] Error al crear cola");
        return;
    }

    BaseType_t resultado =
        xTaskCreatePinnedToCore(
            tareaFirebaseSync,
            "FirebaseSync",
            12288,
            nullptr,
            1,
            &tareaSyncHandle,
            0);

    if (resultado != pdPASS)
    {
        Serial.println("[SYNC] Error al crear tarea");

        vQueueDelete(colaEstados);
        colaEstados = nullptr;
        tareaSyncHandle = nullptr;
        return;
    }

    Serial.println("[SYNC] Modulo inicializado");
}

// ========================================
// RECIBIR ESTADO DEL RELOJ
// ========================================

void actualizarEstadoCloud(
    const EstadoRelojCloud &estado)
{
    if (colaEstados == nullptr)
    {
        return;
    }

    // Conserva solamente el ultimo estado pendiente.
    xQueueOverwrite(colaEstados, &estado);
}
