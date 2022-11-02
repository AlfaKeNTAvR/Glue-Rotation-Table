#include <Arduino.h>


class DebounceRead
{
  public:
    int debouce_counter;
    int debounce_time;
    unsigned long debounce_current_millis;
    unsigned long debounce_previous_millis;

  // Конструктор класса
  DebounceRead()
  {
    debouce_counter = 0;
    debounce_time = 20;  //20
    debounce_current_millis = 0;
    debounce_previous_millis = 0;
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


// Устранение дребезга контактов кнопки
class PinStateMachine
{
  public:
    String pin_state;
    bool pinChecked;

    bool button_off;
    bool button_on;

    DebounceRead buttonOn;
    DebounceRead buttonOff;

  // Конструктор класса
  PinStateMachine()
  {
    pin_state = "OFF";
    pinChecked = true;
    
    button_off = true;
    button_on = false;
  }

  // Конечный автомат
  void updatePinState(int pin_number)
  {
    // Нужно сделать два объекта класса debounceRead, чтобы разделить
    // переменные debounce_counter. Проблема в том, что одна и та же
    // переменная используется везде: для уровня HIGH и для уровня LOW.

    if(buttonOn.debouncedRead(pin_number, HIGH, 50) == true)
    {
      button_off = true;
      button_on = false;
    }

    else if(buttonOff.debouncedRead(pin_number, LOW, 50) == true)
    {
      button_off = false;
      button_on = true;
    }

    if (button_off == true && pin_state == "OFF")
    {
      pin_state = "OFF"; 
      //Serial.println("State 1");
    }

    else if(button_on == true && pin_state == "OFF")
    {
      pin_state = "ON";
      pinChecked = false;
      //Serial.println("State 2");
    }

    else if(button_on == true && pin_state == "ON") 
    {
      pin_state = "ON"; 
      //Serial.println("State 3"); 
    } 

    else if(button_off == true && pin_state == "ON") 
    {
      pin_state = "OFF"; 
      pinChecked = true;
      //Serial.println("State 4");
    }
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
  void setTimer(int delay)
  {
    // Установка референсного значения времени
    timer_previous_millis = millis();

    // Установка задержки
    timer_delay = delay * 1000;

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

const int start_button_pin = 8;
const int rotation_button_pin = 9;
const int lift_button_pin = 10;
const int glue_button_pin = 11;

const int rotation_table_pin = 2; //4
const int lift_pin = 3;           //6
const int glue_pin = 4;           //5
const int free_pin = 5;           //7

const int glue_delay_afterstart_pin = A0;
const int full_cycle_pin = A1;
const int glue_delay_b4end_pin = A2;

String mode = "WAIT";
int manual_counter = 0;

String command = "";

// Объект класса - Кнопки
PinStateMachine startButton;          // Кнопка "ПУСК"
PinStateMachine rotationButton;       // Кнопка "ВРАЩЕНИЕ СТОЛА"
PinStateMachine liftButton;           // Кнопка "УПРАВЛЕНИЕ ЦИЛИНДРОМ"
PinStateMachine glueButton;           // Кнопка "УПРАВЛЕНИЕ КЛЕЕМ"

// Объект класса - Таймеры
Timer rotationTableLiftTimer;     // Таймер вращающегося стола и пневмоподъёмника
Timer glueDelayTimer;             // Таймер задержки подачи клея от начала
Timer glueActionTimer;            // Таймер подачи клея

DebounceRead buttonOn;
DebounceRead buttonOff;

const int table_ON = LOW;
const int lift_ON = LOW;
const int glue_ON = LOW;

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

  digitalWrite(rotation_table_pin, !table_ON);
  digitalWrite(lift_pin, !lift_ON);  
  digitalWrite(glue_pin, !glue_ON);
  digitalWrite(free_pin, HIGH); 

  // Задержки по умолчанию
  rotationTableLiftTimer.timer_delay = 20 * 1000;
  glueDelayTimer.timer_delay = 1 * 1000;
  glueActionTimer.timer_delay = 7 * 1000;

  Serial.begin(9600);
}

void loop()
{
  // Проверка кнопок
  startButton.updatePinState(start_button_pin);
  rotationButton.updatePinState(rotation_button_pin);
  liftButton.updatePinState(lift_button_pin);
  glueButton.updatePinState(glue_button_pin);

  // Состояние ожидания нажатия кнопок
  if(mode == "WAIT")
  {
    // Проверка нажатия кнопки "ПУСК"
    if(startButton.pin_state == "ON" && startButton.pinChecked == false)
    {
      startButton.pinChecked = true;
      mode = "AUTO";

      // Считывание показаний потенциометра
      int glue_delay_afterstart = map(analogRead(glue_delay_afterstart_pin), 0, 1023, 5, 0);
      int full_cycle_delay = map(analogRead(full_cycle_pin), 0, 1023, 30, `);
      int glue_delay_b4end = map(analogRead(glue_delay_b4end_pin), 0, 1023, 5, 0);
      
      // Проверка правильности значений
      

      // Установка таймеров
      rotationTableLiftTimer.setTimer(full_cycle_delay);              // Установка таймера стола и пневмоподъёмника
      glueDelayTimer.setTimer(glue_delay_afterstart);                 // Установка таймера задержки до подачи клея
      glueActionTimer.setTimer(full_cycle_delay - glue_delay_b4end);  // Установка таймера отключения подачи клея

      // Включение стола
      digitalWrite(rotation_table_pin, table_ON);

      // Спуск цилиндра
      digitalWrite(lift_pin, lift_ON);
    }

    // Проверка нажатия кнопки "ВРАЩЕНИЕ СТОЛА"
    else if(rotationButton.pin_state == "ON" && rotationButton.pinChecked == false)
    {
      rotationButton.pinChecked = true;
      mode = "MANUAL";
      Serial.println("Table 1");
      
      // Включение стола
      digitalWrite(rotation_table_pin, table_ON);
      manual_counter = 1;
    }

    // Проверка нажатия кнопки "УПРАВЛЕНИЕ ЦИЛИНДРОМ"
    else if(liftButton.pin_state == "ON" && liftButton.pinChecked == false)
    {
      liftButton.pinChecked = true;
      mode = "MANUAL";
      Serial.println("Cylinder 1");

      // Cпуск цилиндра
      digitalWrite(lift_pin, lift_ON); 
      manual_counter = 1;
    }

    // Проверка нажатия кнопки "УПРАВЛЕНИЕ КЛЕЕМ"
    else if(glueButton.pin_state == "ON" && glueButton.pinChecked == false)
    {
      glueButton.pinChecked = true;
      mode = "MANUAL";
      Serial.println("Glue 1");

      // Подача клея
      digitalWrite(glue_pin, glue_ON); 
      manual_counter = 1;
    }
  }


  // Состояние автоматический режим
  else if(mode == "AUTO")
  {
    // Проверка таймера задержки до подачи клея
    if(glueDelayTimer.checkTimer() == true && glueActionTimer.checkTimer() != true)
    {
      // Включение клея
      digitalWrite(glue_pin, glue_ON);
    }

    // Проверка таймера подачи клея
    if(glueActionTimer.checkTimer() == true)
    {
      // Выключение клея
      digitalWrite(glue_pin, !glue_ON);
    }

    // Проверка основного таймера цикла
    if(rotationTableLiftTimer.checkTimer() == true)
    {
      mode = "WAIT";

      // Выключение стола
      digitalWrite(rotation_table_pin, !table_ON);

      // Подъём цилиндра
      digitalWrite(lift_pin, !lift_ON);

      // Выключение клея (резервное отключение)
      digitalWrite(glue_pin, !glue_ON);
    }

     // Проверка нажатия кнопки "ПУСК"
    if(startButton.pin_state == "ON" && startButton.pinChecked == false)
    {
      startButton.pinChecked = true;
      mode = "WAIT";

      // Выключение стола
      digitalWrite(rotation_table_pin, !table_ON);

      // Подъём цилиндра
      digitalWrite(lift_pin, !lift_ON);

      // Выключение клея (резервное отключение)
      digitalWrite(glue_pin, !glue_ON);
    }
  }

  // Состояние ручной режим
  else if(mode == "MANUAL")
  {
    // Проверка нажатия кнопки "ВРАЩЕНИЕ СТОЛА"
    if(rotationButton.pin_state == "ON" && rotationButton.pinChecked == false)
    {
      rotationButton.pinChecked = true;
      //Serial.println("Table 2");
      Serial.println(digitalRead(rotation_table_pin));

      if(digitalRead(rotation_table_pin) != table_ON)
      {
        digitalWrite(rotation_table_pin, table_ON);
        manual_counter += 1;
      }

      else
      {
        digitalWrite(rotation_table_pin, !table_ON);
        manual_counter -= 1;
      } 

      //Serial.println(manual_counter);
    }

    // Проверка нажатия кнопки "УПРАВЛЕНИЕ ЦИЛИНДРОМ"
    else if(liftButton.pin_state == "ON" && liftButton.pinChecked == false)
    {
      liftButton.pinChecked = true;
      //Serial.println("Cylinder 2");

      if(digitalRead(lift_pin) != lift_ON)
      {
        digitalWrite(lift_pin, lift_ON);
        manual_counter += 1;
      }

      else
      {
        digitalWrite(lift_pin, !lift_ON);
        manual_counter -= 1;
      } 

      //Serial.println(manual_counter);
    }

    // Проверка нажатия кнопки "УПРАВЛЕНИЕ КЛЕЕМ"
    else if(glueButton.pin_state == "ON" && glueButton.pinChecked == false)
    {
      glueButton.pinChecked = true;
      //Serial.println("Glue 2");

      if(digitalRead(glue_pin) != glue_ON)
      {
        digitalWrite(glue_pin, glue_ON);
        manual_counter += 1;
      }

      else
      {
        digitalWrite(glue_pin, !glue_ON);
        manual_counter -= 1;
      } 

      //Serial.println(manual_counter);
    }

    // Выход из режима, если нет активных кнопок
    if(manual_counter == 0)
    {
      mode = "WAIT";
    }
  }
}