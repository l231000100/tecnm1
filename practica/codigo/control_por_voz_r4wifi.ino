#include <WiFiS3.h>
#include "Arduino_LED_Matrix.h"

// ============================================================
// WIFI
// ============================================================

char ssid[] = "iPhone de Ivan";
char password[] = "ivan1910";

WiFiServer server(80);

// ============================================================
// MATRIZ LED - ARDUINO UNO R4 WIFI
// ============================================================

ArduinoLEDMatrix matrix;

uint8_t matrizCompleta[8][12] = {
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1}
};

// ============================================================
// LED INTEGRADO
// ============================================================

const int LED_PIN = LED_BUILTIN;

// ============================================================
// MOTOR + L298N
// ============================================================

const int MOTOR_IN1 = 4;
const int MOTOR_IN2 = 5;
const int MOTOR_ENA = 6;

// ============================================================
// VELOCIDADES
// ============================================================

const int VEL_BAJA = 88;
const int VEL_MEDIA = 155;
const int VEL_FUERTE = 255;

int velocidadActual = VEL_MEDIA;

// ============================================================
// ESTADO DEL MOTOR
// ============================================================

bool motorAvanzando = false;
bool motorRetrocediendo = false;

// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(9600);
  delay(2000);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" CONTROL POR VOZ - ARDUINO UNO R4 WIFI");
  Serial.println("========================================");

  // LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // MATRIZ
  matrix.begin();
  matrix.clear();

  // MOTOR
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_ENA, OUTPUT);

  detenerMotor();

  // WIFI
  Serial.print("Conectando a la red: ");
  Serial.println(ssid);

  int estado = WL_IDLE_STATUS;

  while (estado != WL_CONNECTED) {

    estado = WiFi.begin(ssid, password);

    if (estado != WL_CONNECTED) {
      Serial.println("Intentando conectar...");
      delay(5000);
    }
  }

  Serial.println();
  Serial.println("WIFI CONECTADO CORRECTAMENTE");

  Serial.print("IP DEL ARDUINO: ");
  Serial.println(WiFi.localIP());

  // SERVIDOR
  server.begin();

  Serial.println("SERVIDOR LISTO");
  Serial.println("ESPERANDO COMANDOS...");
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  WiFiClient client = server.available();

  if (client) {

    String request = "";

    unsigned long tiempoInicio = millis();

    // --------------------------------------------------------
    // LEER PETICION
    // --------------------------------------------------------

    while (
      client.connected() &&
      millis() - tiempoInicio < 3000
    ) {

      if (client.available()) {

        char c = client.read();

        request += c;

        if (c == '\n') {
          break;
        }
      }
    }

    // --------------------------------------------------------
    // MOSTRAR PETICION
    // --------------------------------------------------------

    Serial.println();
    Serial.println("-------------");

    Serial.print("PETICION: ");
    Serial.println(request);

    // --------------------------------------------------------
    // EXTRAER COMANDO
    // --------------------------------------------------------

    String comando = extraerComando(request);

    comando = limpiarComando(comando);

    Serial.print("COMANDO RECIBIDO: [");
    Serial.print(comando);
    Serial.println("]");

    // --------------------------------------------------------
    // EJECUTAR
    // --------------------------------------------------------

    String respuesta = ejecutarComando(comando);

    // --------------------------------------------------------
    // RESPUESTA HTTP
    // --------------------------------------------------------

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.println();
    client.println(respuesta);

    delay(20);

    client.stop();

    Serial.print("RESPUESTA: ");
    Serial.println(respuesta);
  }
}

// ============================================================
// EXTRAER COMANDO
// ============================================================

String extraerComando(String request) {

  int inicio = request.indexOf("GET /");

  if (inicio == -1) {
    return "";
  }

  inicio += 5;

  int fin = request.indexOf(" HTTP", inicio);

  if (fin == -1) {
    return "";
  }

  return request.substring(inicio, fin);
}

// ============================================================
// LIMPIAR COMANDO
// ============================================================

String limpiarComando(String cmd) {

  cmd.replace("%20", " ");
  cmd.replace("+", " ");

  cmd.replace("%0A", "");
  cmd.replace("%0D", "");

  cmd.trim();

  cmd.toLowerCase();

  return cmd;
}

// ============================================================
// BUSCAR PALABRA
// ============================================================

bool contiene(String texto, String palabra) {

  return texto.indexOf(palabra) != -1;
}

// ============================================================
// EJECUTAR COMANDO
// ============================================================

String ejecutarComando(String cmd) {

  // ==========================================================
  // MATRIZ - PRENDER
  // ==========================================================

  if (
    contiene(cmd, "matriz") &&
    (
      contiene(cmd, "prender") ||
      contiene(cmd, "prende") ||
      contiene(cmd, "encender") ||
      contiene(cmd, "enciende")
    )
  ) {

    prenderMatriz();

    return "MATRIZ PRENDIDA";
  }

  // ==========================================================
  // MATRIZ - APAGAR
  // ==========================================================

  else if (
    contiene(cmd, "matriz") &&
    (
      contiene(cmd, "apagar") ||
      contiene(cmd, "apaga")
    )
  ) {

    apagarMatriz();

    return "MATRIZ APAGADA";
  }

  // ==========================================================
  // LED - PRENDER
  // ==========================================================

  else if (
    contiene(cmd, "led") &&
    (
      contiene(cmd, "prender") ||
      contiene(cmd, "prende") ||
      contiene(cmd, "encender") ||
      contiene(cmd, "enciende")
    )
  ) {

    digitalWrite(LED_PIN, HIGH);

    return "LED PRENDIDO";
  }

  // ==========================================================
  // LED - APAGAR
  // ==========================================================

  else if (
    contiene(cmd, "led") &&
    (
      contiene(cmd, "apagar") ||
      contiene(cmd, "apaga")
    )
  ) {

    digitalWrite(LED_PIN, LOW);

    return "LED APAGADO";
  }

  // ==========================================================
  // VELOCIDAD BAJA
  // ==========================================================

  else if (
    contiene(cmd, "baja") ||
    contiene(cmd, "lenta") ||
    contiene(cmd, "lento")
  ) {

    velocidadActual = VEL_BAJA;

    aplicarVelocidad();

    Serial.print("PWM BAJA: ");
    Serial.println(velocidadActual);

    return "VELOCIDAD BAJA";
  }

  // ==========================================================
  // VELOCIDAD MEDIA
  // ==========================================================

  else if (
    contiene(cmd, "media") ||
    contiene(cmd, "normal")
  ) {

    velocidadActual = VEL_MEDIA;

    aplicarVelocidad();

    Serial.print("PWM MEDIA: ");
    Serial.println(velocidadActual);

    return "VELOCIDAD MEDIA";
  }

  // ==========================================================
  // VELOCIDAD FUERTE
  // ==========================================================

  else if (
    contiene(cmd, "Fuerte") ||
    contiene(cmd, "fuerte") ||
    contiene(cmd, "maxima") ||
    contiene(cmd, "máxima") ||
    contiene(cmd, "maximo") ||
    contiene(cmd, "máximo")
  ) {

    velocidadActual = VEL_FUERTE;

    aplicarVelocidad();

    Serial.print("PWM FUERTE: ");
    Serial.println(velocidadActual);

    return "VELOCIDAD FUERTE";
  }

  // ==========================================================
  // AVANZAR
  // ==========================================================

  else if (
    contiene(cmd, "avanzar") ||
    contiene(cmd, "avanza") ||
    contiene(cmd, "adelante")
  ) {

    avanzar();

    return "AVANZANDO";
  }

  // ==========================================================
  // RETROCEDER
  // ==========================================================

  else if (
    contiene(cmd, "retroceder") ||
    contiene(cmd, "retrocede") ||
    contiene(cmd, "atras") ||
    contiene(cmd, "atrás")
  ) {

    retroceder();  

    return "RETROCEDIENDO";
  }

  // ==========================================================
  // ALTO
  // ==========================================================

  else if (
    contiene(cmd, "alto") ||
    contiene(cmd, "detente") ||
    contiene(cmd, "para")
  ) {

    detenerMotor();

    return "DETENIDO";
  }

  // ==========================================================
  // NO RECONOCIDO
  // ==========================================================

  else {

    return "COMANDO NO RECONOCIDO: " + cmd;
  }
}

// ============================================================
// PRENDER MATRIZ
// ============================================================

void prenderMatriz() {

  matrix.renderBitmap(
    matrizCompleta,
    8,
    12
  );
}

// ============================================================
// APAGAR MATRIZ
// ============================================================

void apagarMatriz() {

  matrix.clear();
}

// ============================================================
// AVANZAR
// ============================================================

void avanzar() {

  motorAvanzando = true;
  motorRetrocediendo = false;

  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);

  analogWrite(
    MOTOR_ENA,
    velocidadActual
  );

  Serial.print("AVANZANDO - PWM: ");
  Serial.println(velocidadActual);
}

// ============================================================
// RETROCEDER
// ============================================================

void retroceder() {

  motorAvanzando = false;
  motorRetrocediendo = true;

  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, HIGH);

  analogWrite(
    MOTOR_ENA,
    velocidadActual
  );

  Serial.print("RETROCEDIENDO - PWM: ");
  Serial.println(velocidadActual);
}

// ============================================================
// APLICAR VELOCIDAD
// ============================================================

void aplicarVelocidad() {

  if (motorAvanzando) {

    analogWrite(
      MOTOR_ENA,
      velocidadActual
    );

    Serial.print("NUEVA VELOCIDAD AVANCE: ");
    Serial.println(velocidadActual);
  }

  else if (motorRetrocediendo) {

    analogWrite(
      MOTOR_ENA,
      velocidadActual
    );

    Serial.print("NUEVA VELOCIDAD RETROCESO: ");
    Serial.println(velocidadActual);
  }
}

// ============================================================
// DETENER
// ============================================================

void detenerMotor() {

  motorAvanzando = false;
  motorRetrocediendo = false;

  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);

  analogWrite(
    MOTOR_ENA,
    0
  );

  Serial.println("MOTOR DETENIDO");
}