
#include "IOutput.h"
#include "IInput.h"
#include "Toggler.h"
#include "Timer.h"

enum IoPins
{
    BUZZER_PIN      = D10,
    WARNING_LED_PIN = D4,
    BUILTIN_LED_PIN = D17,
    USE_VL_11_5_PIN = D14,
    VOLTAGE_ANA_PIN = A0,
};

#define VERSION 1

enum
{
    NOMINAL_VOLTAGE     = 12, NV = NOMINAL_VOLTAGE,
};

enum VoltageLevel
{
    VL_11_5 = 115,
    VL_11_0 = 110,
    VL_10_5 = 105,
    VL_10_0 = 100,
    VL_9_5  = 95,
    VL_9_0  = 90,
};

//-----------------------------------------------------
//	INPUT/OUTPUT elements
//-----------------------------------------------------

static DigitalOutputPin			buzzer(BUZZER_PIN);
static DigitalOutputPin			warning_led(WARNING_LED_PIN);
static RevertedDigitalOutputPin	builtin_led(BUILTIN_LED_PIN);
static AnalogInputPin           voltage_sensor(VOLTAGE_ANA_PIN);
static DigitalPullupInputPin    use_VL_11_5(USE_VL_11_5_PIN);

//-----------------------------------------------------
//	Other...
//-----------------------------------------------------

static Toggler                builtin_led_toggler,
                              buzzer_toggler,
                              light_toggler;
static Timer                  voltage_check_timer;

//-----------------------------------------------------
//	PREDECLARATIONs
//-----------------------------------------------------

static float get_voltage();
static void check_voltage();

//--------------------------------------------------------------
void setup()
{
    delay(1000);

	Logger::Initialize();
    LOGGER << NL;

    LOGGER << S("==================================================") << NL;
    LOGGER << S("VOLTAGE CONTROLLER ver. ") << VERSION << NL;
    LOGGER << S("==================================================") << NL << NL;
    LOGGER << "Starting..."   << NL;

    buzzer.On();
    warning_led.On();       
    delay(500);
    buzzer.Off();
    warning_led.Off();

    LOGGER << "Initial voltage: " << get_voltage() << NL;
    LOGGER << "use_VL_11_5 is : " << ONOFF(use_VL_11_5.Get()).Get() << NL;
    
    builtin_led_toggler.Start(builtin_led, Toggler::OnTotal(200, 10000));
    voltage_check_timer.StartForever(10, SECS);

	LOGGER << S("Ready!") << NL;
}
//--------------------------------------------------------------
void loop()
{
    builtin_led_toggler.Toggle();
    buzzer_toggler.Toggle();
    light_toggler.Toggle();

    check_voltage();
}
//--------------------------------------------------------------
static float get_voltage()
{
    return voltage_sensor.GetScaled(0, 25);
}
//--------------------------------------------------------------
static void set_toggler_array( uint on, uint off, uint freq, uint16_t a_count, unsigned long a[13] )
{
    unsigned long pause = freq;
    int idx;

    for(idx = 0; idx < a_count; idx++)
    {
        a[idx*2]      = on;
        a[(idx*2)+1]  = off;
        pause        -= (on + off);
    }

    a[(idx*2)-1]     += pause;
}
//--------------------------------------------------------------
static void stop_alarm()
{
    if(light_toggler.IsStarted())
    {
        buzzer_toggler.Stop();
        buzzer.Off();
        light_toggler.Stop();
        warning_led.Off();
    }
}
//--------------------------------------------------------------
static void check_voltage()
{
    if(!voltage_check_timer.Test()) 
        return;

    static float    voltages[6] = { NV, NV, NV, NV, NV, NV };
    static float    current_voltage     = NV,
                    previous_voltage    = NV;
    static int      v_idx = 0;

    voltages[v_idx] = get_voltage();

    int prev_idx = v_idx;
    v_idx = (v_idx + 1) % countof(voltages);

    current_voltage  = 0;

    LOGGER << "Voltages: ";
    for(int i = 0; i < countof(voltages); i++)
    {
        current_voltage += voltages[i];

        LOGGER << voltages[i];
        if(prev_idx == i)
            LOGGER << "*  ";
        else
            LOGGER << "   ";
    }

    current_voltage /= countof(voltages);

    LOGGER << "= " << current_voltage << NL;

    if(v_idx)
        return;

    builtin_led.Toggle(); delay(100);
    builtin_led.Toggle(); delay(100);
    builtin_led.Toggle(); delay(100);
    builtin_led.Toggle(); delay(100);

    float pv = previous_voltage;

    previous_voltage = current_voltage;

 //   if(current_voltage - pv >= 0.5)
 //       stop_alarm();

    int level = current_voltage * 10;

    if(VL_11_5 <= level)
    {
        stop_alarm();
        return;
    }

    static uint16_t a_count;
    uint16_t        prev_a_count = a_count;

    if(level <= VL_9_0)
    {
        a_count = 6;
    } else if(level <= VL_9_5)
    {
        a_count = 5;
    } else if(level <= VL_10_0)
    {
        a_count = 4;
    } else if(level <= VL_10_5)
    {
        a_count = 3;
    } else if(level <= VL_11_0)
    {
        a_count = 2;
    } else
    {
        a_count = 1;
    }

    if(prev_a_count == a_count)
        return;

    stop_alarm();

    bool both = a_count != 1 || use_VL_11_5.Get();

    enum { NOIZE = 500, SILENCE = 1000, BUZZER_FREQ = 60000 };
    enum { LIGHT = 350, DARK    =  650, LIGHT_FREQ  = 10000 };

    static unsigned long buzzer_a[13] = {};
    static unsigned long light_a[13] = {};

    if(both)
    set_toggler_array(NOIZE, SILENCE, BUZZER_FREQ, a_count, buzzer_a );
    set_toggler_array(LIGHT, DARK,    LIGHT_FREQ,  a_count, light_a );

    if(both)
    buzzer_toggler.Start(buzzer,      buzzer_a, a_count * 2);
    light_toggler .Start(warning_led, light_a,  a_count * 2);
}
//--------------------------------------------------------------
