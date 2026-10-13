# Proyecto 2 - Electrónica Digital 2

Universidad del Valle de Guatemala  
Curso: BE3029 - Electrónica Digital 2  
Microcontroladores: STM32 NUCLEO-F446RE y ESP32 DevKit V1



Christian Girón - 23233
Kristen Ackerman - 23852

## Descripción del proyecto

Este proyecto consiste en la integración de dos microcontroladores, una NUCLEO-F446RE y un ESP32, utilizando los protocolos de comunicación UART, SPI e I2C.

La Nucleo funciona como controlador principal y permite al usuario seleccionar diferentes opciones mediante una terminal serial. El ESP32 se encarga de controlar los LEDs, realizar lecturas del potenciómetro mediante el ADC y mostrar información en una pantalla LCD.

Componentes utilizados

- STM32 NUCLEO-F446RE
- ESP32 DevKit V1
- Pantalla LCD 16x2
- 3 LEDs (verde, amarillo y rojo)
- Potenciómetro
- Resistencias
- Protoboard y cables de conexión

Funcionamiento

### 1. Comunicación UART

Se utiliza USART2 de la Nucleo para comunicarse con la computadora mediante Hercules.

El usuario puede seleccionar dos opciones:

- **Opción 1:** Controlar los LEDs mediante SPI.
- **Opción 2:** Obtener la lectura ADC del potenciómetro mediante I2C.

### 2. Comunicación SPI

La Nucleo funciona como maestro SPI y el ESP32 como esclavo.

El usuario ingresa un comando como `1,1000`, donde el primer número indica el LED y el segundo representa el tiempo de encendido en milisegundos.

La Nucleo envía tres bytes al ESP32: uno para identificar el LED y dos para representar el tiempo.

El ESP32 recibe el comando, valida los datos y enciende el LED correspondiente durante el tiempo solicitado.

### 3. Comunicación I2C

La Nucleo funciona como maestro I2C y el ESP32 como esclavo con dirección `0x55`.

Cuando el usuario selecciona la opción 2, la Nucleo solicita la lectura ADC del potenciómetro.

El ESP32 envía el valor dividido en dos bytes y la Nucleo reconstruye la medición para mostrarla en Hercules.

### 4. Pantalla LCD y ADC

El ESP32 utiliza su ADC de 12 bits para leer el potenciómetro, obteniendo valores entre 0 y 4095.

La pantalla LCD 16x2 muestra:

- Voltaje aproximado del potenciómetro.
- Valor digital del ADC.
- Inicial del último LED activado.

### 5. Timer

Se utiliza TIM2 de la Nucleo para generar una señal cuadrada en el pin PA8 mediante interrupciones.

El tiempo de la señal se configura utilizando el valor ingresado por el usuario, sin emplear retardos bloqueantes.

