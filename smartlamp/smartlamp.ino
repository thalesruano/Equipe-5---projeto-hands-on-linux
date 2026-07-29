// Definição de pinos e variáveis
int ledPin = 18;
int ldrPin = 33;
int ledValue = 10; // Quanto maior, maior o brilho do led
int ldrMax = 4000;

// Configuração do PWM para o ESP32 (API v3.x)
const int freq = 5000;
const int resolution = 8;

// Declaração de funções
void ledUpdate();
int ldrGetValue();
void processCommand(String command);

void setup() {
Serial.begin(9600);
delay(1000); // Aguarda a estabilização do Serial
pinMode(ldrPin, INPUT);

// Configuração moderna do PWM no ESP32
if (!ledcAttach(ledPin, freq, resolution)) {
Serial.printf("Erro ao inicializar o PWM no pino %d\n", ledPin);
} else {
Serial.printf("PWM configurado com sucesso no pino %d\n", ledPin);
}

Serial.println("SmartLamp Initialized.");
processCommand("GET_LDR");
ledUpdate(); // Atualiza o LED com o valor inicial (100%)
}

void loop() {
if (Serial.available() > 0) {
String command = Serial.readStringUntil('\n');
command.trim(); // Remove espaços e \r extras
if (command.length() > 0) {
processCommand(command);
}
}
}

void processCommand(String command) {
// Processa o comando GET_LDR
if (command.startsWith("GET_LDR")) {
int ldrVal = ldrGetValue();
Serial.printf("RES GET_LDR %d\n", ldrVal);
}
// Processa o comando SET_LED X
else if (command.startsWith("SET_LED")) {
int spaceIdx = command.indexOf(' ');
if (spaceIdx != -1) {
int val = command.substring(spaceIdx + 1).toInt();
ledValue = constrain(val, 0, 100); // Garante escala entre 0 e 100
ledUpdate();
Serial.printf("RES SET_LED 1 (Valor setado: %d)\n", ledValue);
} else {
Serial.printf("RES SET_LED -1\n");
}
}
// Processa o GET_LED
else if (command.startsWith("GET_LED")) {
Serial.printf("RES GET_LED %d\n", ledValue);
}
}

// Função para ler o valor do LDR
int ldrGetValue() {
return analogRead(ldrPin);
}

// Função para atualizar o valor do LED (converte 0-100 para PWM 0-255)
void ledUpdate() {
int pwmValue = map(ledValue, 0, 100, 0, 255);
ledcWrite(ledPin, pwmValue);
}
