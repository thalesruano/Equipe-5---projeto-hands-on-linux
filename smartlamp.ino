// Defina os pinos de LED e LDR
// Defina uma variável com valor máximo do LDR (4000)
// Defina uma variável para guardar o valor atual do LED (10)

int ledPin = 18;
int ldrPin = 33;
int ledValue = 10;
int ldrMax = 4000;
void ledUpdate();
int ldrGetValue();
void processCommand(String command);


void setup() {
    Serial.begin(9600);
    
    pinMode(ledPin, OUTPUT);
    pinMode(ldrPin, INPUT);
    
    Serial.printf("SmartLamp Initialized.\n");
    processCommand("GET_LDR\n");

}

// Função loop será executada infinitamente pelo ESP32
void loop() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        
        if (command.length() > 0) {
            processCommand(command);
        }
    }
}
void processCommand(String command) {
    command.trim(); 
    
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
            Serial.printf("RES SET_LED 1\n");
        } else {
            Serial.printf("RES SET_LED -1\n");
        }
    }
    // Processa o comando GET_LED
    else if (command.startsWith("GET_LED")) {
        Serial.printf("RES GET_LED %d\n", ledValue);
    }
}

// Função para atualizar o valor do LED
void ledUpdate() {

   int pwmValue = map(ledValue, 0, 100, 0, 255);
    pwmValue = constrain(pwmValue, 0, 255);
    analogWrite(ledPin, pwmValue);
}

// Função para ler o valor do LDR
int ldrGetValue() {
   // Leia o sensor LDR e retorne o valor normalizado entre 0 e 100
    int rawValue = analogRead(ldrPin);
    // Normalização usando o valor ldrMax (4000)
    int normalizedValue = map(rawValue, 0, ldrMax, 0, 100);
    // Garante que o retorno fique estritamente no intervalo [0, 100]
    return constrain(normalizedValue, 0, 100);
}