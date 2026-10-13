/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    main.c
  * @brief   Proyecto 2 - Electronica Digital 2
  *          NUCLEO-F446RE
  *
  * UART: USART2
  * SPI:  SPI1 Master
  * I2C:  I2C1 Master
  * Timer: TIM2
  ******************************************************************************
  */
/* USER CODE END Header */


/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* USER CODE END Includes */


/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */


/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// Direccion del ESP32 para I2C
#define I2C_ESP32_ADDR 0x55

// Tamano del texto recibido por UART
#define UART_BUFFER_SIZE 32

// Pin CS del SPI
#define SPI_CS_PORT GPIOB
#define SPI_CS_PIN  GPIO_PIN_6

// Pin para la señal cuadrada
#define TIMER_PORT GPIOA
#define TIMER_PIN  GPIO_PIN_8

/* USER CODE END PD */


/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */


/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */


/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */

void enviarTexto(char *texto);

void mostrarMenu(void);

void leerLineaUART(char *buffer, uint8_t tamano);

void enviarSPI(
    uint8_t led,
    uint16_t tiempo
);

void leerI2C(void);

void configurarTimer(uint16_t tiempo);

/* USER CODE END PFP */


/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/*
 * Funcion para enviar texto por UART
 */
void enviarTexto(char *texto)
{
    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)texto,
        strlen(texto),
        HAL_MAX_DELAY
    );
}


/*
 * Menu principal
 */
void mostrarMenu(void)
{
    enviarTexto("\r\n");
    enviarTexto("============================\r\n");
    enviarTexto("     PROYECTO DIGITAL 2\r\n");
    enviarTexto("============================\r\n");
    enviarTexto("1. Controlar LED por SPI\r\n");
    enviarTexto("2. Leer sensor por I2C\r\n");
    enviarTexto("============================\r\n");
    enviarTexto("Seleccione una opcion: ");
}


/*
 * Leer una linea desde la terminal serial
 *
 * La funcion recibe caracteres hasta encontrar
 * Enter (\r o \n)
 */
void leerLineaUART(
    char *buffer,
    uint8_t tamano
)
{
    uint8_t dato;
    uint8_t posicion;

    posicion = 0;


    while (posicion < tamano - 1)
    {
        HAL_UART_Receive(
            &huart2,
            &dato,
            1,
            HAL_MAX_DELAY
        );


        // Si el usuario presiona Enter
        if (dato == '\r' || dato == '\n')
        {
            // Salir solamente si ya se escribio algo
            if (posicion > 0)
            {
                break;
            }
        }

        else
        {
            buffer[posicion] = dato;

            posicion++;
        }
    }


    // Final del texto
    buffer[posicion] = '\0';
}


/*
 * Enviar comando al ESP32 por SPI
 *
 * Se envian 3 bytes:
 *
 * Byte 1 = numero del LED
 * Byte 2 = parte alta del tiempo
 * Byte 3 = parte baja del tiempo
 */
void enviarSPI(
    uint8_t led,
    uint16_t tiempo
)
{
    uint8_t datos[3];


    // --------------------------
    // Preparar datos
    // --------------------------

    // Convertir numero de LED a caracter
    // Ejemplo:
    // 1 -> '1'
    datos[0] = led + '0';


    // Byte alto del tiempo
    datos[1] = tiempo >> 8;


    // Byte bajo del tiempo
    datos[2] = tiempo & 0xFF;


    // --------------------------
    // Activar ESP32
    // --------------------------

    HAL_GPIO_WritePin(
        SPI_CS_PORT,
        SPI_CS_PIN,
        GPIO_PIN_RESET
    );


    // --------------------------
    // Enviar 3 bytes
    // --------------------------

    HAL_SPI_Transmit(
        &hspi1,
        datos,
        3,
        HAL_MAX_DELAY
    );


    // --------------------------
    // Desactivar ESP32
    // --------------------------

    HAL_GPIO_WritePin(
        SPI_CS_PORT,
        SPI_CS_PIN,
        GPIO_PIN_SET
    );


    enviarTexto(
        "\r\nComando enviado al ESP32.\r\n"
    );
}


/*
 * Configurar el Timer
 *
 * El tiempo ingresado indica cuanto tiempo
 * permanece HIGH y cuanto tiempo permanece LOW.
 *
 * Ejemplo:
 *
 * tiempo = 500 ms
 *
 * HIGH = 500 ms
 * LOW  = 500 ms
 *
 * Periodo total = 1000 ms
 */
void configurarTimer(uint16_t tiempo)
{
    uint32_t cuentas;


    // --------------------------
    // Detener Timer
    // --------------------------

    HAL_TIM_Base_Stop_IT(
        &htim2
    );


    // --------------------------
    // Calcular ARR
    // --------------------------

    /*
     * El Timer esta configurado para contar
     * 10 veces por cada milisegundo.
     *
     * Por eso:
     *
     * cuentas = tiempo * 10
     */

    cuentas = (uint32_t)tiempo * 10;


    // --------------------------
    // Cambiar periodo
    // --------------------------

    __HAL_TIM_SET_AUTORELOAD(
        &htim2,
        cuentas - 1
    );


    // --------------------------
    // Reiniciar contador
    // --------------------------

    __HAL_TIM_SET_COUNTER(
        &htim2,
        0
    );


    // --------------------------
    // Limpiar bandera
    // --------------------------

    __HAL_TIM_CLEAR_IT(
        &htim2,
        TIM_IT_UPDATE
    );


    // --------------------------
    // Empezar señal en LOW
    // --------------------------

    HAL_GPIO_WritePin(
        TIMER_PORT,
        TIMER_PIN,
        GPIO_PIN_RESET
    );


    // --------------------------
    // Iniciar Timer
    // --------------------------

    HAL_TIM_Base_Start_IT(
        &htim2
    );
}


/*
 * Leer ADC del ESP32 mediante I2C
 */
void leerI2C(void)
{
    uint8_t datos[2];

    uint16_t adc;

    char mensaje[40];


    // --------------------------
    // Solicitar 2 bytes al ESP32
    // --------------------------

    if (
        HAL_I2C_Master_Receive(
            &hi2c1,
            I2C_ESP32_ADDR << 1,
            datos,
            2,
            HAL_MAX_DELAY
        ) == HAL_OK
    )
    {
        // --------------------------
        // Reconstruir ADC
        // --------------------------

        /*
         * datos[0] = byte alto
         * datos[1] = byte bajo
         */

        adc =
            ((uint16_t)datos[0] << 8)
            |
            datos[1];


        // --------------------------
        // Mostrar resultado
        // --------------------------

        sprintf(
            mensaje,
            "\r\nADC recibido: %u\r\n",
            adc
        );


        enviarTexto(
            mensaje
        );
    }

    else
    {
        enviarTexto(
            "\r\nError de comunicacion I2C.\r\n"
        );
    }
}


/* USER CODE END 0 */


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

    /* USER CODE BEGIN 1 */

    uint8_t opcion;

    char comando[UART_BUFFER_SIZE];

    unsigned int led;
    unsigned int tiempo;

    /* USER CODE END 1 */


    /* MCU Configuration--------------------------------------------------------*/

    HAL_Init();


    /* USER CODE BEGIN Init */

    /* USER CODE END Init */


    SystemClock_Config();


    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */


    /* Initialize all configured peripherals */

    MX_GPIO_Init();

    MX_SPI1_Init();

    MX_TIM2_Init();

    MX_USART2_UART_Init();

    MX_I2C1_Init();


    /* USER CODE BEGIN 2 */


    // --------------------------
    // Estado inicial de CS
    // --------------------------

    HAL_GPIO_WritePin(
        SPI_CS_PORT,
        SPI_CS_PIN,
        GPIO_PIN_SET
    );


    // --------------------------
    // Estado inicial salida Timer
    // --------------------------

    HAL_GPIO_WritePin(
        TIMER_PORT,
        TIMER_PIN,
        GPIO_PIN_RESET
    );


    // --------------------------
    // Mensaje inicial
    // --------------------------

    enviarTexto(
        "\r\nNUCLEO F446RE INICIADA\r\n"
    );


    mostrarMenu();


    /* USER CODE END 2 */


    /* Infinite loop */

    /* USER CODE BEGIN WHILE */

    while (1)
    {

        /*
         * Esperar una opcion enviada desde
         * la computadora
         */

        HAL_UART_Receive(
            &huart2,
            &opcion,
            1,
            HAL_MAX_DELAY
        );


        //******************************************/
        // OPCION 1
        // CONTROL SPI + TIMER
        //******************************************/

        if (opcion == '1')
        {
            enviarTexto("\r\n");

            enviarTexto(
                "CONTROL DE LED POR SPI\r\n"
            );

            enviarTexto(
                "Ingrese LED y tiempo (ejemplo 1,500): "
            );


            // --------------------------
            // Leer comando completo
            // --------------------------

            leerLineaUART(
                comando,
                UART_BUFFER_SIZE
            );


            // --------------------------
            // Separar LED y tiempo
            // --------------------------

            /*
             * Ejemplo:
             *
             * comando = "1,500"
             *
             * led = 1
             * tiempo = 500
             */

            if (
                sscanf(
                    comando,
                    "%u,%u",
                    &led,
                    &tiempo
                ) == 2
            )
            {

                // --------------------------
                // Verificar datos
                // --------------------------

                if (
                    led >= 1 &&
                    led <= 3 &&
                    tiempo > 0 &&
                    tiempo <= 65535
                )
                {

                    // --------------------------
                    // Enviar comando por SPI
                    // --------------------------

                    enviarSPI(
                        (uint8_t)led,
                        (uint16_t)tiempo
                    );


                    // --------------------------
                    // Configurar señal cuadrada
                    // --------------------------

                    configurarTimer(
                        (uint16_t)tiempo
                    );
                }

                else
                {
                    enviarTexto(
                        "\r\nValores incorrectos.\r\n"
                    );
                }
            }

            else
            {
                enviarTexto(
                    "\r\nFormato incorrecto.\r\n"
                );

                enviarTexto(
                    "Use el formato LED,tiempo\r\n"
                );

                enviarTexto(
                    "Ejemplo: 1,500\r\n"
                );
            }


            mostrarMenu();
        }


        //******************************************/
        // OPCION 2
        // LEER I2C
        //******************************************/

        else if (opcion == '2')
        {
            enviarTexto("\r\n");

            leerI2C();

            mostrarMenu();
        }


        //******************************************/
        // IGNORAR ENTER
        //******************************************/

        else if (
            opcion == '\r' ||
            opcion == '\n'
        )
        {
            // No hacer nada
        }


        //******************************************/
        // OPCION INCORRECTA
        //******************************************/

        else
        {
            enviarTexto(
                "\r\nOpcion incorrecta.\r\n"
            );

            mostrarMenu();
        }
    }

    /* USER CODE END WHILE */


    /* USER CODE BEGIN 3 */

    /* USER CODE END 3 */
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};


    __HAL_RCC_PWR_CLK_ENABLE();

    __HAL_PWR_VOLTAGESCALING_CONFIG(
        PWR_REGULATOR_VOLTAGE_SCALE3
    );


    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSE;

    RCC_OscInitStruct.HSEState =
        RCC_HSE_BYPASS;

    RCC_OscInitStruct.PLL.PLLState =
        RCC_PLL_ON;

    RCC_OscInitStruct.PLL.PLLSource =
        RCC_PLLSOURCE_HSE;

    RCC_OscInitStruct.PLL.PLLM = 8;

    RCC_OscInitStruct.PLL.PLLN = 336;

    RCC_OscInitStruct.PLL.PLLP =
        RCC_PLLP_DIV4;

    RCC_OscInitStruct.PLL.PLLQ = 2;

    RCC_OscInitStruct.PLL.PLLR = 2;


    if (
        HAL_RCC_OscConfig(
            &RCC_OscInitStruct
        ) != HAL_OK
    )
    {
        Error_Handler();
    }


    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;


    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_PLLCLK;


    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;


    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV2;


    RCC_ClkInitStruct.APB2CLKDivider =
        RCC_HCLK_DIV1;


    if (
        HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_2
        ) != HAL_OK
    )
    {
        Error_Handler();
    }
}


/* USER CODE BEGIN 4 */


/*
 * Interrupcion del Timer
 *
 * Cada vez que el TIM2 termina su conteo
 * se cambia el estado del pin.
 *
 * HIGH -> LOW
 * LOW  -> HIGH
 */
void HAL_TIM_PeriodElapsedCallback(
    TIM_HandleTypeDef *htim
)
{
    if (htim->Instance == TIM2)
    {
        HAL_GPIO_TogglePin(
            TIMER_PORT,
            TIMER_PIN
        );
    }
}


/* USER CODE END 4 */


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */

    __disable_irq();

    while (1)
    {
    }

    /* USER CODE END Error_Handler_Debug */
}


#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file name
  *         and the source line number.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(
    uint8_t *file,
    uint32_t line
)
{
    /* USER CODE BEGIN 6 */

    (void)file;

    (void)line;

    /* USER CODE END 6 */
}

#endif
