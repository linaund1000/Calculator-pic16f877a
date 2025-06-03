#include <xc.h>

// Configuration bits
#pragma config FOSC = HS, WDTE = OFF, PWRTE = ON, BOREN = ON
#pragma config LVP = OFF, CPD = OFF, WRT = OFF, CP = OFF
#define _XTAL_FREQ 20000000

// Hardware definitions
#define RS PORTCbits.RC0
#define EN PORTCbits.RC2
#define RW PORTCbits.RC1
#define LCD_DATA PORTD

// Calculator states
typedef enum {
    IDLE, INPUT_NUM, SHOW_OP, CALC_DONE, ERROR_STATE
} calc_state_t;

// Function prototypes
void system_init(void);
void lcd_init(void);
void lcd_cmd(unsigned char cmd);
void lcd_char(unsigned char data);
void lcd_string(const char* str);
void lcd_clear(void);
void lcd_goto(unsigned char row, unsigned char col);
void lcd_number(long num);
unsigned char scan_keypad(void);
void show_error(unsigned char err);
void show_welcome(void);
void display_help(void);
unsigned char check_overflow(long a, long b, char op);
long calculate(long a, long b, char op);
void process_key(unsigned char key);
void update_display(void);

// Global variables
long num1 = 0, num2 = 0, result = 0, memory = 0;
char operation = 0;
calc_state_t state = IDLE;
unsigned char error_code = 0;
unsigned char help_screen = 0;

// Error codes
#define ERR_OVERFLOW 1
#define ERR_DIV_ZERO 2
#define ERR_INVALID  3

void system_init(void) {
    TRISC = 0x00; TRISD = 0x00; TRISB = 0xF0;
    OPTION_REGbits.nRBPU = 0;
    PORTC = 0x00; PORTD = 0x00; PORTB = 0x0F;
}

void lcd_init(void) {
    __delay_ms(50);
    lcd_cmd(0x38); lcd_cmd(0x0C); lcd_cmd(0x06); lcd_cmd(0x01);
    __delay_ms(2);
}

void lcd_cmd(unsigned char cmd) {
    RS = 0; RW = 0; LCD_DATA = cmd;
    EN = 1; __delay_us(1); EN = 0; __delay_ms(2);
}

void lcd_char(unsigned char data) {
    RS = 1; RW = 0; LCD_DATA = data;
    EN = 1; __delay_us(1); EN = 0; __delay_ms(1);
}

void lcd_string(const char* str) {
    while(*str) lcd_char(*str++);
}

void lcd_clear(void) { 
    lcd_cmd(0x01); 
    __delay_ms(2); 
}

void lcd_goto(unsigned char row, unsigned char col) {
    lcd_cmd((row == 1) ? 0x80 + col : 0xC0 + col);
}

void lcd_number(long num) {
    char str[12]; 
    int i = 0, j;
    if(num == 0) { lcd_char('0'); return; }
    if(num < 0) { lcd_char('-'); num = -num; }
    while(num > 0) { 
        str[i++] = (char)((num % 10) + '0'); 
        num /= 10; 
    }
    for(j = i-1; j >= 0; j--) lcd_char(str[j]);
}

unsigned char scan_keypad(void) {
    unsigned char keys[4][4] = {
        {7, 8, 9, 15},   // 1,2,3,+ 
        {4, 5, 6, 12},   // 4,5,6,-
        {1, 2, 3, 11},   // 7,8,9,*
        {13, 0, 14, 10}  // C,0,=,/ (15 is division)
    };
    
    for(unsigned char row = 0; row < 4; row++) {
        PORTB = (~(1 << row)) & 0x0F; 
        __delay_us(10);
        for(unsigned char col = 0; col < 4; col++) {
            if(!(PORTB & (0x10 << col))) {
                PORTB = 0x0F;
                return keys[row][col];
            }
        }
    }
    PORTB = 0x0F; 
    return 255;
}

void show_error(unsigned char err) {
    lcd_clear();
    switch(err) {
        case ERR_OVERFLOW: lcd_string("ERROR: Overflow"); break;
        case ERR_DIV_ZERO: lcd_string("ERROR: Div by 0"); break;
        case ERR_INVALID:  lcd_string("ERROR: Invalid"); break;
        default: lcd_string("ERROR: Unknown"); break;
    }
    lcd_goto(2, 0); lcd_string("Press C to clear");
}

void show_welcome(void) {
    lcd_clear();
    lcd_string("Smart Calc v2.0");
    lcd_goto(2, 0); lcd_string("Press any key...");
}

void display_help(void) {
    lcd_clear();
    switch(help_screen) {
        case 0:
            lcd_string("Keys: 0-9 Numbers");
            lcd_goto(2, 0); lcd_string("+,-,*,/ Operators");
            break;
        case 1:
            lcd_string("= Calculate");
            lcd_goto(2, 0); lcd_string("C Clear, # Help");
            break;
        case 2:
            lcd_string("Chain operations");
            lcd_goto(2, 0); lcd_string("after results");
            break;
        default:
            help_screen = 0;
            break;
    }
    help_screen = (help_screen + 1) % 3;
}

unsigned char check_overflow(long a, long b, char op) {
    switch(op) {
        case '+': return (a > 0 && b > 32767 - a) || (a < 0 && b < -32768 - a);
        case '-': return (a > 0 && -b > 32767 - a) || (a < 0 && -b < -32768 - a);
        case '*': return (a != 0 && (b > 32767/a || b < -32768/a));
        case '/': return (b == 0);
        default: return 0;
    }
}

long calculate(long a, long b, char op) {
    switch(op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return (b != 0) ? a / b : 0;
        default: return 0;
    }
}

void process_key(unsigned char key) {
    switch(state) {
        case IDLE:
            if(key >= 0 && key <= 9) {
                num1 = key; state = INPUT_NUM;
            } else if(key == 13) { // Clear
                num1 = num2 = result = 0; operation = 0;
            } else if(key == 15) { // Help (#)
                display_help(); __delay_ms(2000);
            }
            break;
            
        case INPUT_NUM:
            if(key >= 0 && key <= 9) {
                if(operation == 0) { // First number
                    if(num1 < 3276) { // Prevent overflow
                        num1 = (num1 * 10) + key;
                    } else {
                        error_code = ERR_OVERFLOW; state = ERROR_STATE;
                    }
                } else { // Second number
                    if(num2 < 3276) { // Prevent overflow
                        num2 = (num2 * 10) + key;
                    } else {
                        error_code = ERR_OVERFLOW; state = ERROR_STATE;
                    }
                }
            } else if((key >= 10 && key <= 12) || key == 15) { // Operations
                if(operation == 0) { // First operation
                    operation = (key == 10) ? '+' : (key == 11) ? '-' : 
                               (key == 12) ? '*' : '/';
                    state = SHOW_OP;
                } else { // Chain operation - calculate first, then set new operation
                    if(check_overflow(num1, num2, operation)) {
                        error_code = (operation == '/' && num2 == 0) ? ERR_DIV_ZERO : ERR_OVERFLOW;
                        state = ERROR_STATE;
                    } else {
                        result = calculate(num1, num2, operation);
                        num1 = result;
                        num2 = 0;
                        operation = (key == 10) ? '+' : (key == 11) ? '-' : 
                                   (key == 12) ? '*' : '/';
                        state = SHOW_OP;
                    }
                }
            } else if(key == 14 && operation != 0) { // Equals (only if we have an operation)
                if(check_overflow(num1, num2, operation)) {
                    error_code = (operation == '/' && num2 == 0) ? ERR_DIV_ZERO : ERR_OVERFLOW;
                    state = ERROR_STATE;
                } else {
                    result = calculate(num1, num2, operation);
                    state = CALC_DONE;
                }
            } else if(key == 13) { // Clear
                num1 = num2 = result = 0; operation = 0; state = IDLE;
            }
            break;
            
        case SHOW_OP:
            if(key >= 0 && key <= 9) {
                num2 = key; 
                state = INPUT_NUM; // Stay in INPUT_NUM to continue entering second number
            } else if(key == 13) { // Clear
                num1 = num2 = 0; operation = 0; state = IDLE;
            }
            break;
            
        case CALC_DONE:
            if((key >= 10 && key <= 12) || key == 15) { // Chain operations
                num1 = result; operation = (key == 10) ? '+' : (key == 11) ? '-' : 
                                         (key == 12) ? '*' : '/';
                num2 = 0; state = SHOW_OP;
            } else if(key == 13) { // Clear
                num1 = num2 = result = 0; operation = 0; state = IDLE;
            } else if(key >= 0 && key <= 9) { // New calculation
                num1 = key; num2 = result = 0; operation = 0; state = INPUT_NUM;
            }
            break;
            
        case ERROR_STATE:
            if(key == 13) { // Clear only
                error_code = 0; num1 = num2 = result = 0; 
                operation = 0; state = IDLE;
            }
            break;
    }
}

void update_display(void) {
    if(state == ERROR_STATE) {
        show_error(error_code);
        return;
    }
    
    lcd_clear();
    
    // Status line
    if(memory != 0) {
        lcd_char('M'); lcd_char(' ');
    }
    
    switch(state) {
        case IDLE:
            lcd_string("Ready...");
            lcd_goto(2, 0); lcd_string("# for help");
            break;
            
        case INPUT_NUM:
            if(operation == 0) {
                lcd_number(num1);
                lcd_goto(2, 0); lcd_string("Enter operation");
            } else {
                lcd_number(num1); lcd_char(' '); lcd_char(operation); lcd_char(' ');
                lcd_number(num2);
                lcd_goto(2, 0); lcd_string("Press = or continue");
            }
            break;
            
        case SHOW_OP:
            lcd_number(num1); lcd_char(' '); lcd_char(operation); lcd_char(' '); lcd_char('?');
            lcd_goto(2, 0); lcd_string("Enter second number");
            break;
            
        case CALC_DONE:
            lcd_number(num1); lcd_char(operation); lcd_number(num2); lcd_char('=');
            lcd_goto(2, 0); lcd_number(result);
            break;
            
        case ERROR_STATE:
            // Already handled above
            break;
    }
}

void main(void) {
    system_init();
    lcd_init();
    show_welcome();
    __delay_ms(2000);
    
    state = IDLE;
    
    while(1) {
        unsigned char key = scan_keypad();
        
        if(key != 255) {
            process_key(key);
            update_display();
            __delay_ms(250); // Debounce
        }
        __delay_ms(10);
    }
}
