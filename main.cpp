//******************************************/
// Universidad del Valle de Guatemala
// BE3029 - Electronica Digital 2
// Proyecto 2
// MCU: ESP32 dev kit 1.0
//******************************************/

//******************************************/
// Librerias
//******************************************/
#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include "driver/spi_slave.h"

//******************************************/
// Definiciones
//******************************************/

// Pines SPI
#define PIN_MOSI 23
#define PIN_MISO 19
#define PIN_SCK  18
#define PIN_CS   5

// Pines I2C
#define PIN_SDA 21
#define PIN_SCL 22
#define I2C_ADDR 0x55

// LEDs
#define LED_VERDE     25
#define LED_AMARILLO  26
#define LED_ROJO      27

// Potenciometro
#define PIN_POT 34

// Pantalla LCD
#define LCD_RS 4
#define LCD_EN 15
#define LCD_D4 33
#define LCD_D5 32
#define LCD_D6 14
#define LCD_D7 13


//******************************************/
// Prototipos de funciones
//******************************************/
void apagarLEDs(void);

void configurarSPI(void);
void revisarSPI(void);

void encenderLED(char led, uint16_t tiempo);
void revisarTiempoLED(void);

void enviarI2C(void);


//******************************************/
// Variables globales
//******************************************/

// Pantalla LCD
LiquidCrystal lcd(
  LCD_RS,
  LCD_EN,
  LCD_D4,
  LCD_D5,
  LCD_D6,
  LCD_D7
);


// Ultimo LED utilizado
char ultimoLED = '-';


// Valor del ADC
volatile uint16_t adcI2C = 0;


// Buffer para recibir los 3 bytes por SPI
uint8_t datos_rx[3];


// Transaccion SPI
spi_slave_transaction_t transaccion;
spi_slave_transaction_t *transaccion_recibida;


// Variables controlar el tiempo del LED
bool ledEncendido = false;

unsigned long tiempoInicioLED = 0;

uint16_t tiempoLED = 0;


// Variable para actualizar LCD
unsigned long tiempoLCD = 0;


//******************************************/
// ISRs Rutinas de Interrupcion
//******************************************/

// solicitar datos por I2C
void enviarI2C()
{
  uint8_t byteAlto;
  uint8_t byteBajo;

  byteAlto = adcI2C >> 8;
  byteBajo = adcI2C & 0xFF;

  Wire.write(byteAlto);
  Wire.write(byteBajo);
}


//******************************************/
// Configuracion
//******************************************/
void setup()
{
  // --------------------------
  // Monitor serial
  // --------------------------
  Serial.begin(115200);


  // --------------------------
  // Configurar LEDs
  // --------------------------
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);

  apagarLEDs();


  // --------------------------
  // Configurar ADC
  // --------------------------
  pinMode(PIN_POT, INPUT);

  analogReadResolution(12);


  // --------------------------
  // Configurar LCD
  // --------------------------
  lcd.begin(16, 2);

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("PROYECTO DIGITAL");

  lcd.setCursor(0, 1);
  lcd.print("INICIANDO...");

  delay(1000);

  lcd.clear();


  // --------------------------
  // Configurar I2C
  // --------------------------

  // ESP32 como esclavo I2C
  Wire.begin(
    I2C_ADDR,
    PIN_SDA,
    PIN_SCL,
    100000
  );


  // Nucleo solicita datos
  Wire.onRequest(enviarI2C);


  // --------------------------
  // Primera lectura ADC
  // --------------------------
  adcI2C = analogRead(PIN_POT);


  // --------------------------
  // Configurar SPI
  // --------------------------
  configurarSPI();


  Serial.println("ESP32 iniciado");
}


//******************************************/
// Loop Principal
//******************************************/
void loop()
{
  int adc;
  float voltaje;


  // --------------------------
  // Leer potenciometro
  // --------------------------
  adc = analogRead(PIN_POT);

  adcI2C = adc;


  // --------------------------
  // Revisar si llego un comando SPI
  // --------------------------
  revisarSPI();


  // --------------------------
  // Revisar tiempo del LED
  // --------------------------
  revisarTiempoLED();


  // --------------------------
  // Actualizar LCD
  // --------------------------

  // Se actualiza cada 100 ms
  if (millis() - tiempoLCD >= 100)
  {
    tiempoLCD = millis();


    // Calcular voltaje
    voltaje = adc * 3.3 / 4095.0;


    // Primera fila
    lcd.setCursor(0, 0);

    lcd.print("Pot1:");
    lcd.print(voltaje, 2);
    lcd.print("V      ");


    // Segunda fila
    lcd.setCursor(0, 1);

    lcd.print("ADC:");
    lcd.print(adc);

    lcd.print(" LED:");
    lcd.print(ultimoLED);

    lcd.print("   ");
  }
}


//******************************************/
// Otras funciones
//******************************************/


//******************************************/
// Apagar todos los LEDs
//******************************************/
void apagarLEDs(void)
{
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_ROJO, LOW);
}


//******************************************/
// Configurar SPI como esclavo
//******************************************/
void configurarSPI(void)
{
  // Configuracion de los pines del bus
  spi_bus_config_t bus = {};

  bus.mosi_io_num = PIN_MOSI;
  bus.miso_io_num = PIN_MISO;
  bus.sclk_io_num = PIN_SCK;

  bus.quadwp_io_num = -1;
  bus.quadhd_io_num = -1;


  // Configuracion del ESP32 como esclavo
  spi_slave_interface_config_t slave = {};

  slave.mode = 0;
  slave.spics_io_num = PIN_CS;
  slave.queue_size = 1;


  // Inicializar SPI
  spi_slave_initialize(
    VSPI_HOST,
    &bus,
    &slave,
    SPI_DMA_DISABLED
  );


  // Limpiar buffer
  memset(
    datos_rx,
    0,
    sizeof(datos_rx)
  );


  // Preparar transaccion
  memset(
    &transaccion,
    0,
    sizeof(transaccion)
  );

  // La Nucleo envia 3 bytes = 24 bits
  transaccion.length = 24;

  transaccion.rx_buffer = datos_rx;


  // Dejar preparada la recepcion
  spi_slave_queue_trans(
    VSPI_HOST,
    &transaccion,
    portMAX_DELAY
  );


  Serial.println("SPI esclavo listo");
}


//******************************************/
// Revisar si se recibieron datos por SPI
//******************************************/
void revisarSPI(void)
{
  // Revisar si la transaccion ya termino
  if (spi_slave_get_trans_result(
        VSPI_HOST,
        &transaccion_recibida,
        0
      ) == ESP_OK)
  {
    char led;
    uint16_t tiempo;


    // --------------------------
    // Primer byte = LED
    // --------------------------
    led = datos_rx[0];


    // --------------------------
    // Segundo y tercer byte = tiempo
    // --------------------------
    tiempo =
      ((uint16_t)datos_rx[1] << 8) |
      datos_rx[2];


    
    // --------------------------
    if ((led == '1' ||
         led == '2' ||
         led == '3') &&
        tiempo > 0)
    {
      encenderLED(
        led,
        tiempo
      );


      Serial.print("LED: ");
      Serial.print(led);

      Serial.print(" Tiempo: ");
      Serial.print(tiempo);

      Serial.println(" ms");
    }


    // --------------------------

    memset(
      datos_rx,
      0,
      sizeof(datos_rx)
    );

    transaccion.length = 24;
    transaccion.rx_buffer = datos_rx;


    spi_slave_queue_trans(
      VSPI_HOST,
      &transaccion,
      portMAX_DELAY
    );
  }
}


//******************************************/
// Encender LED 
//******************************************/
void encenderLED(char led, uint16_t tiempo)
{
  // apagar todos
  apagarLEDs();


  // LED verde
  if (led == '1')
  {
    digitalWrite(
      LED_VERDE,
      HIGH
    );

    ultimoLED = 'V';
  }


  // LED amarillo
  else if (led == '2')
  {
    digitalWrite(
      LED_AMARILLO,
      HIGH
    );

    ultimoLED = 'A';
  }


  // LED rojo
  else if (led == '3')
  {
    digitalWrite(
      LED_ROJO,
      HIGH
    );

    ultimoLED = 'R';
  }


  // Guardar el tiempo solicitado
  tiempoLED = tiempo;

  // Guardar cuando se encendio
  tiempoInicioLED = millis();

  // Indicar que hay un LED encendido
  ledEncendido = true;
}



void revisarTiempoLED(void)
{
  if (ledEncendido == true)
  {
    if (millis() - tiempoInicioLED >= tiempoLED)
    {
      apagarLEDs();

      ledEncendido = false;
    }
  }
}