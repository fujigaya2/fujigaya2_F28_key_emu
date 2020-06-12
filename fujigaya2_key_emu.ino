
//20200608 fujigaya2

#include "Keyboard.h"
#include "ErriezTM1637.h"

//tm1637
#define TM1637_1_CLK_PIN      15
#define TM1637_1_DIO_PIN      14
#define TM1637_2_CLK_PIN      16
#define TM1637_2_DIO_PIN      10

// -A-
// | |
// F B
// | |
// -G-
// | |
// E C
// | |
// -D- DP
// DP G F E D C B A 

#define SEG_0 0b00111111
#define SEG_1 0b00000110
#define SEG_2 0b01011011
#define SEG_3 0b01001111
#define SEG_4 0b01100110
#define SEG_5 0b01101101
#define SEG_6 0b01111101
#define SEG_7 0b00000111
#define SEG_8 0b01111111
#define SEG_9 0b01101111

#define SEG_L 0b00111000
#define SEG_O 0b01011100
#define SEG_C 0b01011000
#define SEG_E 0b01111001
#define SEG_F 0b01110001
#define SEG_R 0b01010000
#define SEG_None 0x00
#define SEG_S 0b01101100
#define SEG_I 0b00000100
#define SEG_T 0b01111000
#define SEG_H 0b01110100
#define SEG_V 0b00111110
#define SEG_W 0b01111110



#define BTN_F0    2
#define BTN_F1    4
#define BTN_F2    7
#define BTN_F3    8
#define BTN_F4    9
#define BTN_DIR   A3
#define SPEED_REF A2

#define WAIT_KEY  333
#define WAIT_KEY_SPD  200

TM1637 tm1637_1(TM1637_1_CLK_PIN, TM1637_1_DIO_PIN);
TM1637 tm1637_2(TM1637_2_CLK_PIN, TM1637_2_DIO_PIN);

static uint8_t keysLast_0 = 0;
static uint8_t keysLast_1 = 0;
static uint8_t keysLast_2 = 0;

static uint16_t function_state[2] = {1,1};//初期値で０にするため！

//Task Schedule
unsigned long gPreviousL1 = 0; // 250ms interval(Packet Task)
//speed buffer
int prev_speed = 0;
//temp_text
char aText[16 + 1];

void setup()
{
  Serial.begin(115200);
  // initialize control over the keyboard:
  Keyboard.begin();

  //main board key init
  main_key_init();
  
  // Initialize TM1637
  tm1637_1.begin();
  tm1637_2.begin();

  // Turn display off (All LED's off)
  tm1637_1.displayOff();
  tm1637_2.displayOff();

  // Clear display
  tm1637_1.clear();
  tm1637_2.clear();

  // Set brightness 0..7
  tm1637_1.setBrightness(1);
  tm1637_2.setBrightness(1);

  // Turn display on
  tm1637_1.displayOn();
  tm1637_2.displayOn();

  //write Loco
  seg_led_emit(SEG_L,SEG_O,SEG_C,SEG_O);
}

void loop()
{
  uint8_t keys_0;
  uint8_t keys_1;
  uint8_t keys_2;

  // Read keys
  keys_0 = get_main_key();
  keys_1 = tm1637_1.getKeys();
  keys_2 = tm1637_2.getKeys();

  if( (millis() - gPreviousL1) >= 200)
  {
    //speed 値の確認！
    int joy_loco_speed = analogRead(SPEED_REF);
    //値が違う時だけ送信！ 電圧が振れる対策でabsを入れる
    if(abs(prev_speed - joy_loco_speed) > 5)
    {
      prev_speed = joy_loco_speed;
      seg_number_emit2(joy_loco_speed / 8);
      sprintf(aText, "P%04d",joy_loco_speed);
      //Serial.print("Speed:");
      Serial.println(aText);
      Keyboard.print(aText);
      gPreviousL1 = millis();
    }
  } 

  if( keysLast_0 != keys_0)
  {
    //Serial.print(F("Keys_0: 0x"));
    //Serial.println(keys_0, HEX);
    if (keys_0 == 0xFF)
    {
      Serial.print(F("Press: 0x"));
      Serial.println(keysLast_0, HEX);
      disp_seg(keysLast_0);
      keyboard_send_main(keysLast_0);
    }
    keysLast_0 = keys_0;
  }
    
  // Check key up
  if (keysLast_1 != keys_1)
  {
    Serial.print(F("Keys_1: 0x"));
    Serial.println(keys_1, HEX);
    if (keys_1 == 0xFF)
    {
      function_button_OnOff(keysLast_1);
      keyboard_send_func(keysLast_1);
    }
    keysLast_1 = keys_1;
  }
  // Check key up
  if (keysLast_2 != keys_2)
  {
    Serial.print(F("Keys_2: 0x"));
    Serial.println(keys_2, HEX);
    if (keys_2 == 0xFF)
    {
      function_button_OnOff(keysLast_2 + 0x10);
      keyboard_send_func(keysLast_2 + 0x10);
    }
    keysLast_2 = keys_2;
  }
}

void keyboard_send_main(uint8_t num)
{
  //Keyboard出力だが、zで方向転換のため、ちょっと拡張にしたい！
  switch(num)
  {
    case 0:  break;  //loco
    case 1:  break;  //switch
    case 2:  break;      //t
    case 3:  break;      //c
    case 4:  Keyboard.press(KEY_LEFT_SHIFT);Keyboard.press('z');Keyboard.releaseAll();      break;  //for
    case 5:  Keyboard.press(KEY_LEFT_ALT);Keyboard.press('z');Keyboard.releaseAll();      break;  //rev
    default:      break;                
  }
}

void disp_seg(uint8_t num)
{
  //キーボード送信
  switch(num)
  {
    case 0:   seg_led_emit(SEG_L,SEG_O,SEG_C,SEG_O);      break;     //loco
    case 1:      seg_led_emit(SEG_S,SEG_W,SEG_T,SEG_H);      break;      //switch
    case 2:      seg_led_emit(SEG_T,SEG_None,SEG_None,SEG_None);      break;      //t
    case 3:      seg_led_emit(SEG_C,SEG_None,SEG_None,SEG_None);      break;      //c
    case 4:      seg_led_emit(SEG_F,SEG_O,SEG_R,SEG_None);      break;  //for
    case 5:      seg_led_emit(SEG_R,SEG_E,SEG_V,SEG_None);      break;  //rev
    default:      seg_led_emit(SEG_None,SEG_None,SEG_None,SEG_None);      break;                
  }
}

void keyboard_send_func(uint8_t num)
{
  uint8_t sel_num = num / 10;
  uint8_t char_num = num % 10 + 0x30;
  //29-31は特殊
  switch(num)
  {
    case 29:
    case 30:
      return;
    case 31:
      Keyboard.press(32); Keyboard.releaseAll();return;
    default:
      break;
  }
  //F0-F28
  switch(sel_num)
  {
    case 0:      Keyboard.press(char_num); Keyboard.releaseAll();     break;
    case 1:      Keyboard.press(KEY_LEFT_SHIFT);Keyboard.press(char_num);Keyboard.releaseAll();      break;
    case 2:      Keyboard.press(KEY_LEFT_ALT) ;Keyboard.press(char_num);Keyboard.releaseAll();      break;
    default:      break;
  }
}


void seg_number_emit(int num)
{
  //4桁数値表示
  seg_led_emit(seg_trans_num(num / 1000 % 10),seg_trans_num(num / 100 % 10),seg_trans_num(num / 10 % 10),seg_trans_num(num % 10));
}

void seg_number_emit2(int num)
{
  //4桁数値表示 0は表示なし
  uint8_t seg0 = 0b00000000;
  uint8_t seg1 = 0b00000000;
  uint8_t seg2 = 0b00000000;
  uint8_t seg3 = 0b00000000;
  if(num / 1000 != 0)
  {
    seg0 = seg_trans_num(num / 1000 % 10);
  }
  if(num / 100 != 0)
  {
    seg1 = seg_trans_num(num / 100 % 10);
  }
  if(num / 10 != 0)
  {
    seg2 = seg_trans_num(num / 10 % 10);
  }
  seg3 = seg_trans_num(num % 10);
  seg_led_emit(seg0,seg1,seg2,seg3);
}


int8_t seg_trans_num(char num)
{
  //数値→SEG変換
  switch(num)
  {
    case 0: return SEG_0;
    case 1: return SEG_1;
    case 2: return SEG_2;
    case 3: return SEG_3;
    case 4: return SEG_4;
    case 5: return SEG_5;
    case 6: return SEG_6;
    case 7: return SEG_7;
    case 8: return SEG_8;
    case 9: return SEG_9;
    default: return SEG_0;
  }
}

void seg_led_emit(uint8_t char0,uint8_t char1,uint8_t char2,uint8_t char3)
{
  //7seg 表示
  tm1637_1.writeData(0x0, char0);
  tm1637_1.writeData(0x1, char1);
  tm1637_1.writeData(0x2, char2);
  tm1637_1.writeData(0x3, char3);
}

void function_button_OnOff(uint8_t num)
{
  //function の状態確認用
  //トグルなので呼ばれると裏返しにするだけ
  //16bitマイコンのため16bit以上のシフトはバグる！のでFunctionは16ビットごとに分けておいた。
  Serial.print("func:");
  Serial.println(num);
  if(num < 16)
  {
    uint16_t temp = 0x0001 << num;
    Serial.print("temp:");
    Serial.println(temp,BIN);
    function_state[0] ^= 0x0001 << num;
    tm1637_1.writeData(5,(function_state[0] >>  8)& 0xff);
    tm1637_1.writeData(4,(function_state[0]      )& 0xff);
  }  
  else
  {
    uint16_t temp = 0x0001 << (num - 16);
    Serial.print("temp:");
    Serial.println(temp,BIN);
    function_state[1] ^= 0x0001 << (num - 16);
    tm1637_2.writeData(5,(function_state[1] >>  8)& 0xff);
    tm1637_2.writeData(4,(function_state[1]      )& 0xff);
  }
}

void main_key_init()
{
  //mainkey部分のイニシャライズ
  //pinMode(BTN_STOP, INPUT);
  pinMode(BTN_DIR, INPUT);
  pinMode(BTN_F0, INPUT);
  pinMode(BTN_F1, INPUT);
  pinMode(BTN_F2, INPUT);
  pinMode(BTN_F3, INPUT);
  pinMode(BTN_F4, INPUT);
  
  //Internal PULL UP
  //digitalWrite(BTN_STOP, HIGH);
  digitalWrite(BTN_DIR, HIGH);
  digitalWrite(BTN_F0, HIGH);
  digitalWrite(BTN_F1, HIGH);
  digitalWrite(BTN_F2, HIGH);
  digitalWrite(BTN_F3, HIGH);
  digitalWrite(BTN_F4, HIGH);  
}

uint8_t get_main_key()
{
  //main基板側のキー確認用
/*  
  #define BTN_F0    D2
  #define BTN_F1    D4
  #define BTN_F2    D7
  #define BTN_F3    D8
  #define BTN_F4    D9
  #define BTN_DIR   A3
*/
  if(digitalRead(BTN_F0) == 0)
  {
    return 0;
  }
  else if(digitalRead(BTN_F1) == 0)
  {
    return 1;
  }
  else if(digitalRead(BTN_F2) == 0)
  {
    return 2;
  }
  else if(digitalRead(BTN_F3) == 0)
  {
    return 3;
  }
  else if(digitalRead(BTN_F4) == 0)
  {
    return 4;
  }
  else if(digitalRead(BTN_DIR) == 0)
  {
    return 5;
  }
  else
  {
    return 0xff;
  }
}
