/*
    GyverOLED - лёгкая и быстрая библиотека для OLED дисплея
    Документация: 
    GitHub: https://github.com/GyverLibs/GyverOLED
    Возможности:
    - Поддержка OLED дисплеев на SSD1306/SSH1106 с разрешением 128х64 и 128х32 с подключением по I2C
    - Выбор буфера
        - Вообще без буфера (и без особой потери возможностей)
        - Буфер на стороне МК (тратит кучу оперативки, но удобнее в работе)
        - Обновление буфера в выбранном месте (для быстрой отрисовки)
        - Динамический буфер выбранного размера (вся геометрия, текст, байты)
        - TODO: Буфер на стороне дисплея (только для SSH1106!!!)
    - Вывод текста
        - Самый быстрый вывод текста среди OLED библиотек
        - Поддержка русского языка и буквы ё (!)
        - Более приятный шрифт (по сравнению с beta)
        - Координаты вне дисплея для возможности прокрутки
        - Вывод текста в любую точку (попиксельная адресация)
        - Полноэкранный вывод с удалением лишних пробелов
        - 4 размера букв (на базе одного шрифта, экономит кучу памяти!)
        - Возможность писать чёрным-по-белому и белым-по-чёрному
    - Управление дисплеем
        - Установка яркости
        - Быстрая инверсия всего дисплея
        - Включение/выключение дисплея из скетча
        - Изменение ориентации дисплея (зеркально по вертикали и горизонтали)
    - Графика (контур, заливка, очистка)
        - Точки
        - Линии
        - Прямоугольники
        - Прямоугольники со скруглёнными углами
        - Окружности
        - Кривые Безье
    - Изображения (битмап)
        - Вывод битмапа в любую точку дисплея
        - Вывод "за дисплей"
        - Программа для конвертации изображений есть в библиотеке
    - Поддержка библиотеки microWire для ATmega328 (очень лёгкий и быстрый вывод)
    
    AlexGyver, alex@alexgyver.ru
    https://alexgyver.ru/
    MIT License
    
    Версии (beta)
    v0.1 (27.02.2021) - исправил непечатающуюся нижнюю строку
    v0.2 (16.03.2021) - исправлены символы [|]~$
    v0.3 (26.03.2021) - добавил кривую Безье
    v0.4 (10.04.2021) - совместимость с есп
    v0.5 (09.05.2021) - добавлена поддержка SPI и SSH1106 (только буфер)! gnd-vcc-sck-data-rst-dc-cs
    
    v1.0 - релиз
    v1.1 - улучшен перенос строк (не убирает первый символ просто так)
    v1.2 - переделан FastIO
    v1.3 - прямоугольники можно рисовать из любого угла
    v1.3.1 - пофиксил линии (сломались в 1.3.0)
    v1.3.2 - убран FastIO
    v1.4 - пофикшены SPI дисплеи
*/

#ifndef GyverOLED_h
#define GyverOLED_h

// ===== константы =====
#define OLED_I2C 0
#define OLED_SPI 1

#define OLED_CLEAR 0
#define OLED_FILL 1
#define OLED_STROKE 2

#define BUF_ADD 0
#define BUF_SUBTRACT 1
#define BUF_REPLACE 2

#define BITMAP_NORMAL 0
#define BITMAP_INVERT 1

// ===========================================================================
#include <Wire.h>

#include <Arduino.h>
#include <Print.h>
#include "charMap.h"

// ============================ БЭКЭНД КОНСТАНТЫ ==============================
// внутренние константы
#define OLED_WIDTH      		128
#define OLED_HEIGHT_64      	0x12
#define OLED_64             	0x3F

#define OLED_DISPLAY_OFF    	0xAE
#define OLED_DISPLAY_ON     	0xAF

#define OLED_COMMAND_MODE       0x00
#define OLED_ONE_COMMAND_MODE   0x80
#define OLED_DATA_MODE          0x40
#define OLED_ONE_DATA_MODE      0xC0

#define OLED_ADDRESSING_MODE	0x20
#define OLED_HORIZONTAL			0x00
#define OLED_VERTICAL   		0x01

#define OLED_NORMAL_V			0xC8
#define OLED_FLIP_V				0xC0
#define OLED_NORMAL_H			0xA1
#define OLED_FLIP_H				0xA0

#define OLED_CONTRAST   		0x81
#define OLED_SETCOMPINS   		0xDA
#define OLED_SETVCOMDETECT		0xDB
#define OLED_CLOCKDIV 			0xD5
#define OLED_SETMULTIPLEX		0xA8
#define OLED_COLUMNADDR			0x21
#define OLED_PAGEADDR			0x22
#define OLED_CHARGEPUMP			0x8D

#define OLED_NORMALDISPLAY		0xA6
#define OLED_INVERTDISPLAY		0xA7

#define BUFSIZE_128x64 (128*64/8)
#define BUFSIZE_128x32 (128*32/8)

// индекс в буфере
#define _bufIndex(x, y) (((y) >> 3) + ((x) << (_TYPE ? 3 : 2)))		// _y / 8 + _x * (4 или 8)
#define _swap(x, y) x = x ^ y ^ (y = x)
#define _inRange(x, mi, ma) ((x) >= (mi) && (x) <= (ma))

// список инициализации
static const uint8_t _oled_init[] PROGMEM = {
    OLED_DISPLAY_OFF,
    OLED_CLOCKDIV,
    0x80,	// value
    OLED_CHARGEPUMP,
    0x14,	// value
    OLED_ADDRESSING_MODE,
    OLED_VERTICAL,
    OLED_NORMAL_H,
    OLED_NORMAL_V,
    OLED_CONTRAST,
    0x7F,	// value
    OLED_SETVCOMDETECT,
    0x40, 	// value
    OLED_NORMALDISPLAY,
    OLED_DISPLAY_ON,
};

// ========================== КЛАСС КЛАСС КЛАСС ============================= 
class GyverOLED : public Print {
public:
    // ========================== КОНСТРУКТОР ============================= 
    GyverOLED(uint8_t address = 0x3C) : _address(address) {}

    // ============================= СЕРВИС =============================== 
    // инициализация
    void init() {
        Wire.begin();
        
        beginCommand();
        for (uint8_t i = 0; i < 15; i++) sendByteRaw(pgm_read_byte(&_oled_init[i]));
        endTransm();
        beginCommand();
        sendByteRaw(OLED_SETCOMPINS);
        sendByteRaw(OLED_HEIGHT_64);
        sendByteRaw(OLED_SETMULTIPLEX);
        sendByteRaw(OLED_64);
        endTransm();
        
        setCursorXY(0, 0);
    }
    
    // очистить дисплей
    void clear() { fill(0); }
    
    // яркость 0-255
    void setContrast(uint8_t value) { sendCommand(OLED_CONTRAST, value); }
    
    // вкл/выкл
    void setPower(bool mode) 		{ sendCommand(mode ? OLED_DISPLAY_ON : OLED_DISPLAY_OFF); }
    
    // отразить по горизонтали
    void flipH(bool mode) 			{ sendCommand(mode ? OLED_FLIP_H : OLED_NORMAL_H); }
    
    // инвертировать дисплей
    void invertDisplay(bool mode) 	{ sendCommand(mode ? OLED_INVERTDISPLAY : OLED_NORMALDISPLAY); }
    
    // отразить по вертикали
    void flipV(bool mode) 			{ sendCommand(mode ? OLED_FLIP_V : OLED_NORMAL_V); }

    // ============================= ПЕЧАТЬ ================================== 
    virtual size_t write(uint8_t data) {
        // переносы и пределы
        bool newPos = false;		
        if (data == '\r') { _x = 0; newPos = true; data = 0; }					    // получен возврат каретки
        if (data == '\n') { _y += _scaleY; newPos = true; data = 0; _getn = 1; }    // получен перевод строки
        if (_println && (_x + 6*_scaleX) >= _maxX) { _x = 0; _y += _scaleY; newPos = true; }	// строка переполненена, перевод и возврат
        if (newPos) setCursorXY(_x, _y);										    // переставляем курсор
        if (_y + _scaleY > _maxY + 1) data = 0;									    // дисплей переполнен
        if (_getn && _println && data == ' ' && _x == 0) { _getn = 0; data = 0; }   // убираем первый пробел в строке
        
        if (data == 0) return 1;	
        // если тут не вылетели - печатаем символ		
                // SSH1106
            int newX = _x + _scaleX * 6;
            if (newX < 0 || _x > _maxX) _x = newX;                              // пропускаем вывод "за экраном"
            else {
                for (uint8_t col = 0; col < 6; col++) {                         // 6 стобиков буквы
                    uint8_t bits = getFont(data, col);                          // получаем байт
                    if (_invState) bits = ~bits;                                // инверсия
                    if (_scaleX == 1) {                                         // если масштаб 1
                        if (_x >= 0 && _x <= _maxX) {                           // внутри дисплея
                            if (_shift == 0) {                                  // если вывод без сдвига на строку
                                beginData();
                                sendByteRaw(bits);                                 // выводим
                                endTransm();
                            } else {                                            // со сдвигом и с разделением на 2 строки
                                setWindowAddress(_x, _y);
                                beginData();
                                sendByteRaw(bits << _shift);                       // верхняя часть
                                endTransm();
                                setWindowAddress(_x, _y + 8);
                                beginData();
                                sendByteRaw(bits >> (8 - _shift));                 // нижняя часть
                                endTransm();
                            }
                        }
                    } else {                                                                // масштаб 2, 3 или 4 - растягиваем шрифт
                                                                                            // разбиваем и выводим по строкам
                        long newData = 0;                                                   // буфер
                        for (uint8_t i = 0, count = 0; i < 8; i++)
                            for (uint8_t j = 0; j < _scaleX; j++, count++)
                                bitWrite(newData, count, bitRead(bits, i));                 // пакуем растянутый шрифт
                                                                                            // выводим пачками по строкам (page)
                        for (byte y = 0; y < _scaleX; y++) {                                // проходим по каждой строке (y)
                            sendCommand(0xb0 + (_y >> 3) + y);                              // установить адрес строки (page address)
                            for (uint8_t i = 0; i < _scaleX; i++) {                         // выводим. По столбцам (i: x)
                                sendCommand((_x + 2 + i) & 0xf);                            // нижний адрес столбца
                                sendCommand(0x10 | ((_x + 2 + i) >> 4));                    // верхний адрес столбца
                                beginData();
                                if (_x + i >= 0 && _x + i <= _maxX) {                       // проверка на попадание внутри дисплея
                                    if (_shift == 0) {                                      // если вывод без сдвига на строку
                                        for (uint8_t j = 0; j < _scaleX; j++) {             // выводим. Разделяем по блокам (j: y)
                                            byte data;
                                            data = newData >> ((j + y) * 8);                // получаем кусок буфера
                                            sendByteRaw(data);                                 // выводим
                                        }
                                    } else {                                                // если вывод со сдвигом на строку
                                        for (uint8_t j = 0; j < _scaleX; j++) {             // выводим. Разделяем по блокам (j: y)
                                            byte data;
                                            data = (newData << _shift) >> ((j + y) * 8);    // получаем кусок буфера
                                            sendByteRaw(data);                                 // выводим
                                        }
                                    }
                                }
                                endTransm();
                            }
                        }
                        // Добавляется строка в конце при сдвиге на строку
                        if (_shift != 0) {
                            sendCommand(0xb0 + (_y >> 3) + _scaleX);                  // установить адрес строки (page address)
                            for (uint8_t i = 0; i < _scaleX; i++) {             // выводим. По столбцам (i: x)
                                sendCommand((_x + 2 + i) & 0xf);                // нижний адрес столбца
                                sendCommand(0x10 | ((_x + 2 + i) >> 4));        // верхний адрес столбца
                                beginData();
                                for (uint8_t j = 0; j < _scaleX; j++) {     // выводим. Разделяем по блокам (j: y)
                                    byte data;
                                    data = (newData << _shift) >> ((j + _scaleX) * 8);   // получаем кусок буфера
                                    // data = newData >> ((j + _scaleX) * 8 + 8 - _shift);   // получаем кусок буфера
                                    sendByteRaw(data);                         // выводим
                                }
                                endTransm();
                            }
                        }
                    }
                    _x += _scaleX;                                              // двигаемся на ширину пикселя (1-4)
                }
            }        
        return 1;
    }
    // поставить курсор для символа 0-127, 0-63(31)
    void setCursorXY(int x, int y) { 			
        _x = x;
        _y = y;	
        setWindowShift(x, y, _maxX, _scaleY);
        setWindowAddress(x, y);
    }
    
    // масштаб шрифта (1-4)
    void setScale(uint8_t scale) {
        scale = constrain(scale, 1, 4);	// защита от нечитающих доку
        _scaleX = scale;
        _scaleY = scale * 8;
        setCursorXY(_x, _y);
    }
    
    // инвертировать текст (0-1)
    void invertText(bool inv) 	{ _invState = inv; }
    
    void textMode(byte mode) 	{ _mode = mode; }
    
    // возвращает true, если дисплей "кончился" - при побуквенном выводе
    bool isEnd() 				{ return (_y > _maxRow); }

    // ================================== ГРАФИКА ================================== 
    // точка (заливка 1/0)
    void dot(int x, int y, byte fill = 1) {		
        if (x < 0 || x > _maxX || y < 0 || y > _maxY) return;
        _x = 0;
        _y = 0;
            if (!_buf_flag) {
                setWindow(x, y>>3, _maxX, _maxRow);
                setCursorXY(x, y);
                beginData();			
                sendByteRaw(1 << (y & 0b111));	// задвигаем 1 на высоту y
                endTransm();
            } else {				
                x -= _bufX;
                y -= _bufY;
                if (x < 0 || x > _bufsizeX || y < 0 || y > (_bufsizeY << 3)) return;				
                bitWrite(_buf_ptr[ (y>>3) + x * _bufsizeY ], y & 0b111, fill);
            }
    }
    
    // линия
    void line(int x0, int y0, int x1, int y1, byte fill = 1) {
        _x = 0;
        _y = 0;
        if (x0 == x1) fastLineV(x0, y0, y1, fill);
        else if (y0 == y1) fastLineH(y0, x0, x1, fill);
        else {
            int sx, sy, e2, err;
            int dx = abs(x1 - x0);
            int dy = abs(y1 - y0);
            sx = (x0 < x1) ? 1 : -1;
            sy = (y0 < y1) ? 1 : -1;
            err = dx - dy;
            for (;;) {
                dot(x0, y0, fill);
                if (x0 == x1 && y0 == y1) return;
                e2 = err<<1;
                if (e2 > -dy) { 
                    err -= dy; 
                    x0 += sx; 
                }
                if (e2 < dx) { 
                    err += dx; 
                    y0 += sy; 
                }
            }
        }
    }
    
    // горизонтальная линия
    void fastLineH(int y, int x0, int x1, byte fill = 1) {
        _x = 0;
        _y = 0;
        if (x0 > x1) _swap(x0, x1);
        if (y < 0 || y > _maxY) return;
        if (x0 == x1) {
            dot(x0, y, fill);
            return;		
        }
        x1++;
        x0 = constrain(x0, 0, _maxX);
        x1 = constrain(x1, 0, _maxX);
            if (_buf_flag) {
                for (int x = x0; x < x1; x++) dot(x, y, fill);
                return;
            }
            byte data = 0b1 << (y & 0b111);
                setCursorXY(x0, y);
                y >>= 3;
                beginData();
                for (int x = x0; x < x1; x++) writeData(data, y, x);
                endTransm();		
    }
    
    // вертикальная линия
    void fastLineV(int x, int y0, int y1, byte fill = 1) {
        _x = 0;
        _y = 0;
        if (y0 > y1) _swap(y0, y1);
        if (x < 0 || x > _maxX) return;
        if (y0 == y1) {
            dot(x, y0, fill);
            return;			
        }
        y1++;
            if (_buf_flag) {
                for (int y = y0; y < y1; y++) dot(x, y, fill);
                return;
            }
            if (fill) fill = 255;
            byte shift = y0 & 0b111;
            byte shift2 = 8 - (y1 & 0b111);
            if (shift2 == 8) shift2 = 0;
            int height = y1 - y0;
            y0 >>= 3;
            y1 = (y1 - 1) >> 3;
            byte numBytes = y1 - y0;
                if (numBytes == 0) {
                    if (_inRange(y0, 0, _maxRow)) {
                        setCursorXY(x, y0 * 8);
                        beginData();
                        sendByteRaw((fill >> (8-height)) << shift);
                        endTransm();
                        }
                } else {
                    if (_inRange(y0, 0, _maxRow)) {
                        setCursorXY(x, y0 * 8);
                        beginData();
                        sendByteRaw(fill << shift);                                      // начальный кусок
                        endTransm();
                    }
                    y0++;
                    for (byte i = 0; i < numBytes - 1; i++, y0++) {
                        if (_inRange(y0, 0, _maxRow)) {
                            // столбик
                            setCursorXY(x, y0 * 8);
                            beginData();
                            sendByteRaw(fill);
                            endTransm();
                        }
                    }
                    if (_inRange(y0, 0, _maxRow)) {
                        setCursorXY(x, y0 * 8);
                        beginData();
                        sendByteRaw(fill >> shift2);                                     // нижний кусок
                        endTransm();
                    }
                }
    }
    
    // вывести битмап
    void drawBitmap(int x, int y, const uint8_t *frame, int width, int height, uint8_t invert = 0, byte mode = 0) {
        _x = 0;
        _y = 0;
        if (invert) invert = 255;
        byte left = height & 0b111;
        if (left != 0) height += (8 - left);	// округляем до ближайшего кратного степени 2
        int shiftY = (y>>3)+(height>>3);		// строка (row) крайнего байта битмапа
        setWindowShift(x, y, width, height);	// выделяем окно
        bool bottom = (_shift != 0 && shiftY >= 0 && shiftY <= _maxRow);	// рисовать ли нижний сдвинутый байт
        
        beginData();
        for (int X = x, countX = 0; X < x+width; X++, countX++) {						// в пикселях
            byte prevData = 0;
            if (_inRange(X, 0, _maxX)) {												// мы внутри дисплея по X
                for (int Y = y>>3, countY = 0; Y < shiftY; Y++, countY++) {				// в строках (пикс/8) 					
                    byte data = pgm_read_word(&(frame[countY*width+countX])) ^ invert;	// достаём байт
                    if (_shift == 0) {													// без сдвига по Y
                        if (_inRange(Y, 0, _maxRow)) writeData(data, Y, X, mode);		// мы внутри дисплея по Y
                    } else {															// со сдвигом по Y
                        if (_inRange(Y, 0, _maxRow)) writeData((prevData >> (8 - _shift)) | (data << _shift), Y, X, mode);	// задвигаем
                        prevData = data;						
                    }					
                }
                if (bottom) writeData(prevData >> (8 - _shift), shiftY, X, mode);		// нижний байт
            }
        }
        endTransm();
    }
    
    // залить весь дисплей указанным байтом
    void fill(uint8_t data) {
        		// для SSH1106
                sendCommand(0x00); // Set Lower Column Start Address for Page Addressing Mode
                sendCommand(0x10); // Set Higher Column Start Address for Page Addressing Mode
                sendCommand(0x40); // Set display RAM display start line register from 0 - 63
                // For each page
                for (uint8_t i = 0; i < (64 >> 3); i++) {
                    sendCommand(0xB0 + i + 0);  // Set page address
                    sendCommand(2 & 0xf);       // Set lower column address
                    sendCommand(0x10);          // Set higher column address
                    // Divide for blocks
                    beginData();
                    for (uint8_t a = 0; a < 8; a++) {
                        // For each block
                        for (uint8_t b = 0; b < (OLED_WIDTH >> 3); b++) {
                            sendByteRaw(data);
                        }
                    }
                    endTransm();
                }
                // Return cursor home
                setCursorXY(_x, _y);	
    }
    
    // шлёт байт в "столбик" setCursor() и setCursorXY()
    void drawByte(uint8_t data) {
        if (++_x > _maxX) return;
            // для SSH1106 со сдвигом по вертикали
                setWindowAddress(_x, _y);
                beginData();
            writeData(data << _shift);                       // верхняя часть
                endTransm();
                setWindowAddress(_x, _y + 8);
                beginData();
            writeData(data >> (8 - _shift));                 // нижняя часть
            endTransm();
            /*			
            int h = y & 0x07;			
            if (_BUFF) {
                for (int p = 0; p < 2; p++) {
                    Wire.beginTransmission(_address);
                    continueCmd(0xB0 + (y >> 3) + p);        // Page
                    continueCmd(0x00 + ((x + 2) & 0x0F));    // Column low nibble
                    continueCmd(0x10 + ((x + 2) >> 4));      // Column high nibble
                    continueCmd(0xE0);            // Read modify write
                    Wire.write(OLED_ONE_DATA_MODE);
                    Wire.endTransmission();
                    Wire.requestFrom((int)_address, 2);
                    Wire.read();                       		 // Dummy read
                    int j = Wire.read();
                    Wire.beginTransmission(_address);
                    Wire.write(OLED_ONE_DATA_MODE);
                    Wire.write((data << h) >> (p << 3) | j);
                    continueCmd(0xEE);                       // Cancel read modify write
                    Wire.endTransmission();
                }
            } else {
                for (int p = 0; p < 2; p++) {
                    Wire.beginTransmission(_address);
                    continueCmd(0xB0 + (y >> 3) + p);        // Page
                    continueCmd(0x00 + ((x + 2) & 0x0F));    // Column low nibble
                    continueCmd(0x10 + ((x + 2) >> 4));      // Column high nibble
                    Wire.write(OLED_ONE_DATA_MODE);
                    Wire.write((data << h) >> (p << 3));
                    Wire.endTransmission();
                }
            }*/	
    }
    
    // вывести одномерный байтовый массив (линейный битмап высотой 8)
    void drawBytes(uint8_t* data, byte size) {
        beginData();
        for (byte i = 0; i < size; i++) {
            if (++_x > _maxX) return;			
            if (_shift == 0) {							// если вывод без сдвига на строку
                writeData(data[i]);						// выводим
            } else {									// со сдвигом
                writeData(data[i] << _shift);			// верхняя часть
                writeData(data[i] >> (8 - _shift), 1);	// нижняя часть со сдвигом на 1
            }			
        }
        endTransm();
    }

    // ================================== СИСТЕМНОЕ ===================================    
    
    // отправить байт по i2с или в буфер
    void writeData(byte data, byte offsetY = 0, byte offsetX = 0, int mode = 0) {
            if (_buf_flag) {
                int x = _x - _bufX;
                int y = _y - _bufY;
                if (x < 0 || x > _bufsizeX || y < 0 || y > (_bufsizeY << 3)) return;
                switch (mode) {
                case 0: _buf_ptr[ (y>>3) + x * _bufsizeY ] |= data;		// добавить
                    break;
                case 1: _buf_ptr[ (y>>3) + x * _bufsizeY ] &= ~data;	// вычесть
                    break;
                case 2: _buf_ptr[ (y>>3) + x * _bufsizeY ] = data;		// заменить
                    break;
                }				
            } else {
                sendByteRaw(data);
            }
    }	
    
    // окно со сдвигом. x 0-127, y 0-63 (31), ширина в пикселях, высота в пикселях
    void setWindowShift(int x0, int y0, int sizeX, int sizeY) {
        _shift = y0 & 0b111;
        setWindow(x0, (y0 >> 3), x0 + sizeX, (y0 + sizeY - 1) >> 3);
    }
    
    // ========= ЛОУ-ЛЕВЕЛ ОТПРАВКА =========
    
    void sendByteRaw(uint8_t data) {
        Wire.write(data);
    }

    
    // отправить команду
    void sendCommand(uint8_t cmd1) {       		
        beginOneCommand();
        sendByteRaw(cmd1);		
        endTransm();
    }
    
    // отправить код команды и команду
    void sendCommand(uint8_t cmd1, uint8_t cmd2) {       		
        beginCommand();
        sendByteRaw(cmd1);
        sendByteRaw(cmd2);
        endTransm();
    }	

    // выбрать "окно" дисплея
    void setWindow(int x0, int y0, int x1, int y1) {		
        beginCommand();
        sendByteRaw(OLED_COLUMNADDR);
        sendByteRaw(constrain(x0, 0, _maxX));
        sendByteRaw(constrain(x1, 0, _maxX));
        sendByteRaw(OLED_PAGEADDR);
        sendByteRaw(constrain(y0, 0, _maxRow));
        sendByteRaw(constrain(y1, 0, _maxRow));
        endTransm();
    }

    void setWindowAddress(int x, int y) {
        // for SSH1106 without buffer
            sendCommand(0xb0 + (y >> 3)); //set page address
            sendCommand((x + 2) & 0xf); //set lower column address
            sendCommand(0x10 | ((x + 2) >> 4)); //set higher column address
    }
    
    void beginData() {
        startTransm();
        sendByteRaw(OLED_DATA_MODE);	
    }
    
    void beginCommand() {
        startTransm();
        sendByteRaw(OLED_COMMAND_MODE);		
    }
    
    void beginOneCommand() {
        startTransm();
        sendByteRaw(OLED_ONE_COMMAND_MODE);		
    }
    
    void endTransm() {		
            Wire.endTransmission();
            _writes = 0;
    }

    void startTransm() {
        Wire.beginTransmission(_address);
    }


    // получить "столбик-байт" буквы
    uint8_t getFont(uint8_t font, uint8_t row) {
        if (row > 4) return 0;		
        font = font - '0' + 16;   // перевод код символа из таблицы ASCII	
        if (font <= 95) {
            return pgm_read_byte(&(charMap[font][row])); 		// для английских букв и символов
        } else if (font >= 96 && font <= 111) {					// и пизд*ц для русских
            return pgm_read_byte(&(charMap[font + 47][row]));
        } else if (font <= 159) {              			
            return pgm_read_byte(&(charMap[font - 17][row]));
        } else {
            return pgm_read_byte(&(charMap[font - 1][row]));	// для кастомных (ё)
        }
    }	
    
    // ==================== ПЕРЕМЕННЫЕ И КОНСТАНТЫ ====================
    const uint8_t _address = 0x3C;
    const uint8_t _maxRow = 7;
    const uint8_t _maxY = 63;
    const uint8_t _maxX = OLED_WIDTH - 1;	// на случай добавления мелких дисплеев	
    
private:
    // всякое

    bool _invState = 0;
    bool _println = false;
    bool _getn = false;
    uint8_t _scaleX = 1, _scaleY = 8;
    int _x = 0, _y = 0;
    uint8_t _shift = 0;
    uint8_t _lastChar;
    uint8_t _writes = 0;
    uint8_t _mode = 2;
    
    // дин. буфер
    int _bufsizeX, _bufsizeY;
    int _bufX, _bufY;
    uint8_t *_buf_ptr;
    bool _buf_flag = false;	
};
#endif
