
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <atomic>
#include <freertos/semphr.h>

#include "firebase_auth.h"
#include "firebase_ca.h"
#include "wifi_secrets.h"

// ========================================
// ANDESCHESS LIVE - FIREBASE AUTH
// ========================================

constexpr uint32_t FIREBASE_CHECK_MS = 5000;
constexpr uint32_t FIREBASE_RETRY_MS = 15000;
constexpr uint32_t FIREBASE_RELOGIN_MS = 3000000;

static std::atomic<bool> autenticado{false};
static TaskHandle_t firebaseTaskHandle = nullptr;

// El token solo se utiliza desde la tarea
// de comunicacion. No se imprime por Serial.
static String firebaseIdToken;
static SemaphoreHandle_t tokenMutex = nullptr;

static void guardarFirebaseToken(const String &token)
{
    if (tokenMutex == nullptr)
    {
        return;
    }

    if (xSemaphoreTake(tokenMutex, portMAX_DELAY) == pdTRUE)
    {
        firebaseIdToken = token;
        xSemaphoreGive(tokenMutex);
    }
}

bool obtenerFirebaseToken(String &token)
{
    token = "";

    if (tokenMutex == nullptr)
    {
        return false;
    }

    if (xSemaphoreTake(tokenMutex, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        return false;
    }

    bool disponible =
        autenticado.load() &&
        !firebaseIdToken.isEmpty();

    if (disponible)
    {
        token = firebaseIdToken;
    }

    xSemaphoreGive(tokenMutex);

    return disponible;
}

// ========================================
// SINCRONIZACION DE HORA
// ========================================

bool prepararHoraTLS()
{

    static bool ntpIniciado = false;

    if (!ntpIniciado)
    {
        configTime(
            0,
            0,
            "pool.ntp.org",
            "time.google.com");

        ntpIniciado = true;
    }

    time_t ahora = time(nullptr);
    Serial.print("[FIREBASE] Fecha UTC: ");
    Serial.print(ctime(&ahora));

    // TLS necesita una fecha valida para
    // comprobar los certificados.
    return ahora > 1700000000;
}

// ========================================
// INICIO DE SESION FIREBASE
// ========================================

bool autenticarFirebase()
{

    if (WiFi.status() != WL_CONNECTED)
    {
        return false;
    }

    if (!prepararHoraTLS())
    {
        Serial.println(
            "[FIREBASE] Esperando hora NTP");
        return false;
    }

    WiFiClientSecure cliente;
    cliente.setCACert(FIREBASE_CA_CERT);
    cliente.setTimeout(8000);

    HTTPClient http;
    http.setTimeout(8000);

    String url =
        "https://identitytoolkit.googleapis.com"
        "/v1/accounts:signInWithPassword?key=";

    url += FIREBASE_API_KEY;

    if (!http.begin(cliente, url))
    {
        Serial.println(
            "[FIREBASE] Error al iniciar HTTPS");
        return false;
    }

    http.addHeader(
        "Content-Type",
        "application/json");

    JsonDocument solicitud;
    solicitud["email"] = FIREBASE_EMAIL;
    solicitud["password"] = FIREBASE_PASSWORD;
    solicitud["returnSecureToken"] = true;

    String cuerpo;
    serializeJson(solicitud, cuerpo);

    Serial.println(
        "[FIREBASE] Intentando autenticacion...");

    int codigo = http.POST(cuerpo);

    String respuesta;

    if (codigo > 0)
    {
        respuesta = http.getString();
    }

    http.end();

    // No mostrar la respuesta completa:
    // puede contener tokens privados.

    if (codigo != 200)
    {
        Serial.printf(
            "[FIREBASE] Error HTTPS/HTTP: %d\n",
            codigo);

        // Extraer solamente el codigo de error
        // para no mostrar credenciales ni tokens.
        JsonDocument errorDoc;

        if (deserializeJson(errorDoc, respuesta) == DeserializationError::Ok)
        {
            const char *mensaje =
                errorDoc["error"]["message"] | "DESCONOCIDO";

            Serial.print("[FIREBASE] Motivo: ");
            Serial.println(mensaje);
        }
        else
        {
            Serial.println(
                "[FIREBASE] No se pudo interpretar el error");
        }

        return false;
    }

    JsonDocument documento;

    DeserializationError error =
        deserializeJson(documento, respuesta);

    if (error)
    {
        Serial.println(
            "[FIREBASE] Respuesta JSON invalida");
        return false;
    }

    String token =
        documento["idToken"].as<String>();

    String uid =
        documento["localId"].as<String>();

    if (token.isEmpty() || uid.isEmpty())
    {
        Serial.println(
            "[FIREBASE] No se recibio token valido");
        return false;
    }

    guardarFirebaseToken(token);

    Serial.println(
        "[FIREBASE] Autenticacion exitosa");

    Serial.println(
        "[FIREBASE] Token recibido correctamente");

    return true;
}

// ========================================
// TAREA INDEPENDIENTE DE FIREBASE
// ========================================

void tareaFirebase(void *parametro)
{

    Serial.println(
        "[FIREBASE] Tarea iniciada");

    uint32_t ultimoIntento = 0;
    bool primerIntento = true;

    for (;;)
    {

        uint32_t ahora = millis();

        if (WiFi.status() != WL_CONNECTED)
        {

            autenticado.store(false);
            guardarFirebaseToken("");

            primerIntento = true;
        }
        else
        {

            uint32_t intervalo =
                autenticado.load()
                    ? FIREBASE_RELOGIN_MS
                    : FIREBASE_RETRY_MS;

            if (
                primerIntento ||
                (uint32_t)(ahora - ultimoIntento) >= intervalo)
            {

                primerIntento = false;
                ultimoIntento = ahora;

                autenticado.store(false);

                bool resultado =
                    autenticarFirebase();

                autenticado.store(resultado);

                if (!resultado)
                {
                    guardarFirebaseToken("");

                    Serial.println(
                        "[FIREBASE] Autenticacion pendiente");
                }
            }
        }

        vTaskDelay(
            pdMS_TO_TICKS(FIREBASE_CHECK_MS));
    }
}

// ========================================
// INICIALIZACION
// ========================================

void iniciarFirebaseAuth()
{

    if (firebaseTaskHandle != nullptr)
    {
        return;
    }

    if (tokenMutex == nullptr)
    {
        tokenMutex = xSemaphoreCreateMutex();

        if (tokenMutex == nullptr)
        {
            Serial.println(
                "[FIREBASE] Error al crear mutex");
            return;
        }
    }

    BaseType_t resultado =
        xTaskCreatePinnedToCore(
            tareaFirebase,
            "FirebaseAuth",
            12288,
            nullptr,
            1,
            &firebaseTaskHandle,
            0);

    if (resultado == pdPASS)
    {

        Serial.println(
            "[FIREBASE] Modulo inicializado");
    }
    else
    {

        firebaseTaskHandle = nullptr;

        Serial.println(
            "[FIREBASE] Error al crear tarea");
    }
}

// ========================================
// ESTADO
// ========================================

bool firebaseAutenticado()
{
    return autenticado.load();
}
