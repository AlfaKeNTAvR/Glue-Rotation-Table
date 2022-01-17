#include <Arduino.h>

// Устранение дребезга контактов кнопки
class PinDebouncing
{
  public:
    int debouce_counter;
    
    unsigned long debounce_current_millis;
    unsigned long debounce_previous_millis;
    int debounce_time;

  PinDebouncing()
  {
    debouce_counter = 0;
    debounce_current_millis = 0;
    debounce_previous_millis = 0;
    debounce_time = 100;
  }

  // Функция устранения дребезга контактов
  bool debouncedRead(int pin_number, int pin_state, int debounce_time)
  {
    debounce_current_millis = millis();
      
    // Первый этап проверки пина
    if(debouce_counter == 0 && digitalRead(pin_number) == pin_state)
    {
      debounce_previous_millis = debounce_current_millis;
      debouce_counter = 1;
    }
  
    // Второй этап проверки пина
    else if(debouce_counter == 1 && debounce_current_millis - debounce_previous_millis >= debounce_time)
    {
      if(digitalRead(pin_number) == pin_state)
      {
        debounce_previous_millis = debounce_current_millis;
        debouce_counter = 2;
      }
  
      // Ложное срабатывание
      else 
      {
        // Cброс флага срабатывания пина
        debouce_counter = 0;
      }
    }
  
    // Третий этап этап проверки пина
    else if(debouce_counter == 2 && debounce_current_millis - debounce_previous_millis >= debounce_time)
    {
      if(digitalRead(pin_number) == pin_state)
      {
        // Cброс флага срабатывания пина
        debouce_counter = 0;
  
        // Выход из функции по завершению
        return true;            
      }
  
      // Ложное срабатывание
      else 
      {
        // Cброс флага срабатывания пина
        debouce_counter = 0;
      }
    }
  
     return false;
  }   
};

class Timer
{
  public:
    unsigned long timer_current_millis;
    unsigned long timer_previous_millis;
    int timer_delay;
    bool timer_flag;

  Timer()
  {
    timer_current_millis = 0;
    timer_previous_millis = 0;
    timer_delay = 0;
    timer_flag = false;
  }

  // Функция установки таймера
  void setTimer()
  {
    // Установка референсного значения времени
    timer_previous_millis = millis();

    // Установка флага таймера
    timer_flag = true;
  }

  // Функция проверки таймера
  bool checkTimer()
  {
    // Получить текущее время
    timer_current_millis = millis();

    // Если задержка выполнена
    if(timer_current_millis - timer_previous_millis >= timer_delay)
    {
        // Сброс флага таймера
        timer_flag = false;

        return true;
    }

    else
    {
      return false;
    }
  }
};

// Объект класса - Кнопки
PinDebouncing startButton;        // Кнопка "ПУСК"
PinDebouncing rotationButton;     // Кнопка "ВРАЩЕНИЕ СТОЛА"
PinDebouncing liftButton;         // Кнопка "УПРАВЛЕНИЕ ЦИЛИНДРОМ"
PinDebouncing glueButton;         // Кнопка "УПРАВЛЕНИЕ КЛЕЕМ"

// Объект класса - Таймеры
Timer rotationTableLiftTimer;     // Таймер вращающегося стола и пневмоподъёмника
Timer glueDelayTimer;             // Таймер задержки подачи клея от начала
Timer glueActionTimer;            // Таймер подачи клея

const int start_button_pin = 8;
const int rotation_button_pin = 9;
const int lift_button_pin = 10;
const int glue_button_pin = 11;

const int rotation_table_pin = 0;
const int lift_pin = 1;
const int glue_pin = 2;

String mode = "WAIT";
int manual_counter = 0;

void setup() 
{
  // Входные пины
  pinMode(start_button_pin, INPUT_PULLUP);
  pinMode(rotation_button_pin, INPUT_PULLUP);
  pinMode(lift_button_pin, INPUT_PULLUP);
  pinMode(glue_button_pin, INPUT_PULLUP);

  // Выходные пины
  pinMode(rotation_table_pin, OUTPUT);
  pinMode(lift_pin, OUTPUT);
  pinMode(glue_pin, OUTPUT);

  digitalWrite(rotation_table_pin, LOW);
  digitalWrite(lift_pin, LOW);
  digitalWrite(glue_pin, LOW);

  // Задержки по умолчанию
  rotationTableLiftTimer.timer_delay = 10 * 1000;
  glueDelayTimer.timer_delay = 1 * 1000;
  glueActionTimer.timer_delay = 7 * 1000;

  Serial.begin(9600);
}

void loop()
{
  if(mode == "WAIT")
  {
    // Проверка нажатия кнопки "ПУСК"
    if(startButton.debouncedRead(start_button_pin, LOW, 50) == true)
    {
      mode = "AUTO";

      // Считывание показаний потенциометра


      // Расчёт задержек
      rotationTableLiftTimer.timer_delay = 10 * 1000;
      glueDelayTimer.timer_delay = 1 * 1000;
      glueActionTimer.timer_delay = rotationTableLiftTimer.timer_delay - glueDelayTimer.timer_delay;

      // Установка таймеров
      rotationTableLiftTimer.setTimer();    // Установка таймера стола и пневмоподъёмника
      glueDelayTimer.setTimer();            // Установка таймера задержки до подачи клея
      glueActionTimer.setTimer();           // Установка таймера подачи клея

      // Включение стола
      digitalWrite(rotation_table_pin, HIGH);

      // Спуск цилиндра
      digitalWrite(lift_pin, HIGH);
    }

    // Проверка нажатия кнопки "ВРАЩЕНИЕ СТОЛА"
    else if(rotationButton.debouncedRead(rotation_button_pin, LOW, 50) == true)
    {
      mode = "MANUAL";

      // Включение стола
      digitalWrite(rotation_table_pin, HIGH);
    }

    // Проверка нажатия кнопки "УПРАВЛЕНИЕ ЦИЛИНДРОМ"
    else if(liftButton.debouncedRead(lift_button_pin, LOW, 50) == true)
    {
      mode = "MANUAL";

      // Cпуск цилиндра
      digitalWrite(lift_pin, HIGH); 
    }

    // Проверка нажатия кнопки "УПРАВЛЕНИЕ КЛЕЕМ"
    else if(glueButton.debouncedRead(glue_button_pin, LOW, 50) == true)
    {
      mode = "MANUAL";

      // Подача клея
      digitalWrite(glue_pin, HIGH); 
    }
  }

  else if(mode == "AUTO")
  {
    // Проверка таймера задержки до подачи клея
    if(glueDelayTimer.checkTimer() == true)
    {
      // Включение клея
      digitalWrite(glue_pin, HIGH);
    }

    // Проверка таймера подачи клея
    if(glueActionTimer.checkTimer() == true)
    {
      // Выключение клея
      digitalWrite(glue_pin, LOW);
    }

    // Проверка основного таймера цикла
    if(rotationTableLiftTimer.checkTimer() == true)
    {
      mode = "WAIT";

      // Выключение стола
      digitalWrite(rotation_table_pin, LOW);

      // Подъём цилиндра
      digitalWrite(lift_pin, LOW);

      // Выключение клея (резервное отключение)
      digitalWrite(glue_pin, LOW);
    }
  }

  else if(mode == "MANUAL")
  {
    // Проверка нажатия кнопки "ВРАЩЕНИЕ СТОЛА"
    if(rotationButton.debouncedRead(rotation_button_pin, LOW, 50) == true)
    {
      if(digitalRead(rotation_table_pin) == LOW)
      {
        digitalWrite(rotation_table_pin, HIGH);
        manual_counter += 1;
      }

      else
      {
        digitalWrite(rotation_table_pin, LOW);
        manual_counter -= 1;
      } 
    }

    // Проверка нажатия кнопки "УПРАВЛЕНИЕ ЦИЛИНДРОМ"
    else if(liftButton.debouncedRead(lift_button_pin, LOW, 50) == true)
    {
      if(digitalRead(lift_pin) == LOW)
      {
        digitalWrite(lift_pin, HIGH);
        manual_counter += 1;
      }

      else
      {
        digitalWrite(lift_pin, LOW);
        manual_counter -= 1;
      } 
    }

    // Проверка нажатия кнопки "УПРАВЛЕНИЕ КЛЕЕМ"
    else if(glueButton.debouncedRead(glue_button_pin, LOW, 50) == true)
    {
      if(digitalRead(glue_pin) == LOW)
      {
        digitalWrite(glue_pin, HIGH);
        manual_counter += 1;
      }

      else
      {
        digitalWrite(glue_pin, LOW);
        manual_counter -= 1;
      } 
    }

    // Выход из режима, если нет активных кнопок
    if(manual_counter == 0)
    {
      mode = "WAIT";
    }
  }
}




















void loop() 
{
  if(wait_button_flag == true)
  {
    // Проверка нажатия кнопки
    if(startButton.debouncedRead(start_button_pin, LOW, 50) == true)
    {
      // Установка таймера стола и пневмоподъёмника
      rotationTableLiftTimer.setTimer();

      // Установка таймера задержки подачи клея
      glueDelayTimer.setTimer();

      // Включение стола
      digitalWrite(rotation_table_pin, HIGH);

      // Спуск пневмоподъёмника
      digitalWrite(lift_pin, HIGH);

      // Сброс флага ожидания нажатия кнопки
      wait_button_flag = false;

      // Установка флага задержки подачи клея
      glue_delay_flag = true;
    }
  }

  else if(wait_button_flag == false)
  {
    // Проверка основного таймера цикла
    if(rotationTableLiftTimer.checkTimer() == true)
    {
      // Выключение клея (чтобы гарантировать выключение клея при завершении цикла)
      digitalWrite(glue_pin, LOW);

      // Выключение стола
      digitalWrite(rotation_table_pin, LOW);

      // Подъём пневмоподъёмника
      digitalWrite(lift_pin, LOW);

      // Установка флага ожидания нажатия кнопки
      wait_button_flag = true;
    }

    else
    {
      if(glueDelayTimer.checkTimer() == true)


    }

  }

}