#include <Arduino.h>

// Устранение дребезга контактов кнопки
class PinDebouncing
{
  public:
    int debouce_counter;
    
    unsigned long debounce_current_millis;
    unsigned long debounce_previous_millis;
    int debounce_time;

    int off_state;
    String pin_state;

  PinDebouncing()
  {
    debouce_counter = 0;
    debounce_current_millis = 0;
    debounce_previous_millis = 0;
    debounce_time = 100;

    off_state = HIGH;
    pin_state = "OFF";
  }

  // Функция устранения дребезга контактов
  bool debouncedRead(int pin_number, int trigger_state, int debounce_time)
  {
    debounce_current_millis = millis();
      
    // Первый этап проверки пина
    if(debouce_counter == 0 && digitalRead(pin_number) == trigger_state)
    {
      debounce_previous_millis = debounce_current_millis;
      debouce_counter = 1;
    }
  
    // Второй этап проверки пина
    else if(debouce_counter == 1 && debounce_current_millis - debounce_previous_millis >= debounce_time)
    {
      if(digitalRead(pin_number) == trigger_state)
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
      if(digitalRead(pin_number) == trigger_state)
      {
        // Cброс флага срабатывания пина
        debouce_counter = 0;

        // Cменя состояния пина
        if(trigger_state == off_state) pin_state = "OFF";
        else pin_state = "TRIGGERED";
          
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

const int rotation_table_pin = 4;
const int lift_pin = 6;
const int glue_pin = 5;
const int free_pin = 7;

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
  pinMode(free_pin, OUTPUT);

  digitalWrite(rotation_table_pin, LOW);
  digitalWrite(lift_pin, LOW);  
  digitalWrite(glue_pin, LOW);
  digitalWrite(free_pin, LOW); 

  // Задержки по умолчанию
  rotationTableLiftTimer.timer_delay = 20 * 1000;
  glueDelayTimer.timer_delay = 1 * 1000;
  glueActionTimer.timer_delay = 7 * 1000;

  Serial.begin(9600);
}

void loop()
{
  // Сброс кнопок в состоянии "OFF", если не нажаты
  startButton.debouncedRead(start_button_pin, HIGH, 50)
  rotationButton.debouncedRead(rotation_button_pin, HIGH, 50)
  liftButton.debouncedRead(lift_button_pin, HIGH, 50)
  glueButton.debouncedRead(glue_button_pin, HIGH, 50)


  if(mode == "WAIT")
  {
    // Проверка нажатия кнопки "ПУСК"
    if(startButton.debouncedRead(start_button_pin, LOW, 50) == true && startButton.pin_state == "OFF")
    {
      mode = "AUTO";

      // Считывание показаний потенциометра


      // Расчёт задержек
      rotationTableLiftTimer.timer_delay = 40 * 1000;
      glueDelayTimer.timer_delay = 5 * 1000;
      glueActionTimer.timer_delay = rotationTableLiftTimer.timer_delay - 20 * 1000;

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
    else if(rotationButton.debouncedRead(rotation_button_pin, LOW, 50) == true && rotationButton.pin_state == "OFF")
    {
      mode = "MANUAL";

      // Включение стола
      digitalWrite(rotation_table_pin, HIGH);
      manual_counter += 1;
    }

    // Проверка нажатия кнопки "УПРАВЛЕНИЕ ЦИЛИНДРОМ"
    else if(liftButton.debouncedRead(lift_button_pin, LOW, 50) == true && liftButton.pin_state == "OFF")
    {
      mode = "MANUAL";

      // Cпуск цилиндра
      digitalWrite(lift_pin, HIGH); 
      manual_counter += 1;
    }

    // Проверка нажатия кнопки "УПРАВЛЕНИЕ КЛЕЕМ"
    else if(glueButton.debouncedRead(glue_button_pin, LOW, 50) == true && glueButton.pin_state == "OFF")
    {
      mode = "MANUAL";

      // Подача клея
      digitalWrite(glue_pin, HIGH); 
      manual_counter += 1;
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
    if(rotationButton.debouncedRead(rotation_button_pin, LOW, 50) == true && rotationButton.pin_state == "OFF")
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
    else if(liftButton.debouncedRead(lift_button_pin, LOW, 50) == true && liftButton.pin_state == "OFF")
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
    else if(glueButton.debouncedRead(glue_button_pin, LOW, 50) == true && glueButton.pin_state == "OFF")
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