#include "daisy_seed.h"

extern "C"
{
#include "dev/codec_ak4556.h"
}

using namespace daisy;

#define SEED_LED_PORT Pin::Port::DSY_GPIOC
#define SEED_LED_PIN 7

#define SEED_TEST_POINT_PORT Pin::Port::DSY_GPIOG
#define SEED_TEST_POINT_PIN 14

#ifndef SEED_REV2
const Pin::Port seedgpio_port[31] = {
    // GPIO 1-8
    //{DSY_GPIOA, 8}, // removed on Rev4
    Pin::Port::DSY_GPIOB,
    Pin::Port::DSY_GPIOC,
    Pin::Port::DSY_GPIOC,
    Pin::Port::DSY_GPIOC,
    Pin::Port::DSY_GPIOC,
    Pin::Port::DSY_GPIOD,
    Pin::Port::DSY_GPIOC,
    // GPIO 9-16
    Pin::Port::DSY_GPIOG,
    Pin::Port::DSY_GPIOG,
    Pin::Port::DSY_GPIOB,
    Pin::Port::DSY_GPIOB,
    Pin::Port::DSY_GPIOB,
    Pin::Port::DSY_GPIOB,
    Pin::Port::DSY_GPIOB,
    Pin::Port::DSY_GPIOB,
    // GPIO 17-24
    Pin::Port::DSY_GPIOC,
    Pin::Port::DSY_GPIOA,
    Pin::Port::DSY_GPIOB,
    Pin::Port::DSY_GPIOA,
    Pin::Port::DSY_GPIOA,
    Pin::Port::DSY_GPIOC,
    Pin::Port::DSY_GPIOC,
    Pin::Port::DSY_GPIOA,
    // GPIO 17-24
    Pin::Port::DSY_GPIOA,
    Pin::Port::DSY_GPIOA,
    Pin::Port::DSY_GPIOA,
    Pin::Port::DSY_GPIOD,
    Pin::Port::DSY_GPIOG,
    Pin::Port::DSY_GPIOA,
    Pin::Port::DSY_GPIOB,
    Pin::Port::DSY_GPIOB,
};
const uint8_t seedgpio_pin[31] = {
    // GPIO 1-8
    //{DSY_GPIOA, 8}, // removed on Rev4
    12,
    11,
    10,
    9,
    8,
    2,
    12,
    // GPIO 9-16
    10,
    11,
    4,
    5,
    8,
    9,
    6,
    7,
    // GPIO 17-24
    0,
    3,
    1,
    7,
    6,
    1,
    4,
    5,
    // GPIO 17-24
    4,
    1,
    0,
    11,
    9,
    2,
    14,
    15,
};
#else
const dsy_gpio_port seed_ports[32] = {
    DSY_GPIOA, DSY_GPIOB, DSY_GPIOC, DSY_GPIOC, DSY_GPIOC, DSY_GPIOC, DSY_GPIOD,
    DSY_GPIOC, DSY_GPIOG, DSY_GPIOG, DSY_GPIOB, DSY_GPIOB, DSY_GPIOB, DSY_GPIOB,
    DSY_GPIOB, DSY_GPIOB, DSY_GPIOC, DSY_GPIOA, DSY_GPIOA, DSY_GPIOB, DSY_GPIOA,
    DSY_GPIOA, DSY_GPIOC, DSY_GPIOC, DSY_GPIOA, DSY_GPIOA, DSY_GPIOA, DSY_GPIOD,
    DSY_GPIOG, DSY_GPIOA, DSY_GPIOB, DSY_GPIOB,
};

const uint8_t seed_pins[32] = {
    8, 12, 11, 10, 9, 8, 7, 12, 10, 11, 4, 5,  8, 9, 6,  7,
    0, 1,  3,  1,  7, 6, 1, 5,  5,  4,  0, 11, 9, 2, 14, 15,
};

const dsy_gpio_pin seedgpio[32] = {
    {seed_ports[0], seed_pins[0]},   {seed_ports[1], seed_pins[1]},
    {seed_ports[2], seed_pins[2]},   {seed_ports[3], seed_pins[3]},
    {seed_ports[4], seed_pins[4]},   {seed_ports[5], seed_pins[5]},
    {seed_ports[6], seed_pins[6]},   {seed_ports[7], seed_pins[7]},
    {seed_ports[8], seed_pins[8]},   {seed_ports[9], seed_pins[9]},
    {seed_ports[10], seed_pins[10]}, {seed_ports[11], seed_pins[11]},
    {seed_ports[12], seed_pins[12]}, {seed_ports[13], seed_pins[13]},
    {seed_ports[14], seed_pins[14]}, {seed_ports[15], seed_pins[15]},
    {seed_ports[16], seed_pins[16]}, {seed_ports[17], seed_pins[17]},
    {seed_ports[18], seed_pins[18]}, {seed_ports[19], seed_pins[19]},
    {seed_ports[20], seed_pins[20]}, {seed_ports[21], seed_pins[21]},
    {seed_ports[22], seed_pins[22]}, {seed_ports[23], seed_pins[23]},
    {seed_ports[24], seed_pins[24]}, {seed_ports[25], seed_pins[25]},
    {seed_ports[26], seed_pins[26]}, {seed_ports[27], seed_pins[27]},
    {seed_ports[28], seed_pins[28]}, {seed_ports[29], seed_pins[29]},
    {seed_ports[30], seed_pins[30]}, {seed_ports[31], seed_pins[31]},
};
#endif

// Public Initialization

void DaisySeed::Configure()
{
    // Configure internal peripherals
    ConfigureSdram();
    ConfigureQspi();
    //ConfigureDac();

    // Configure the built-in GPIOs.
    GPIO::Config gpio_config;
    gpio_config.mode = GPIO::Config::Mode::OUTPUT_PP;
    gpio_config.pin.Init(SEED_LED_PORT, SEED_LED_PIN);
    led.Init(gpio_config);

    gpio_config.pin.Init(SEED_TEST_POINT_PORT, SEED_TEST_POINT_PIN);
    testpoint.Init(gpio_config);
}

void DaisySeed::Init(bool boost)
{
    //dsy_system_init();
    System::Config syscfg;
    boost ? syscfg.Boost() : syscfg.Defaults();
    system.Init(syscfg);

    dsy_sdram_init(&sdram_handle);
    dsy_qspi_init(&qspi_handle);
    ConfigureAudio();

    callback_rate_ = AudioSampleRate() / AudioBlockSize();
    // Due to the added 16kB+ of flash usage,
    // and the fact that certain breakouts use
    // both; USB won't be initialized by the
    // SEED file.
    //usb_handle.Init(UsbHandle::FS_INTERNAL);
}

Pin DaisySeed::GetPin(uint8_t pin_idx)
{
    Pin p;
    pin_idx = pin_idx < 32 ? pin_idx : 0;
#ifndef SEED_REV2
    p.Init(seedgpio_port[pin_idx], seedgpio_pin[pin_idx]);
#else
    p = {seed_ports[pin_idx], seed_pins[pin_idx]};
#endif
    return p;
}

void DaisySeed::DelayMs(size_t del)
{
    system.Delay(del);
}

void DaisySeed::StartAudio(AudioHandle::InterleavingAudioCallback cb)
{
    audio_handle.Start(cb);
}

void DaisySeed::StartAudio(AudioHandle::AudioCallback cb)
{
    audio_handle.Start(cb);
}

void DaisySeed::ChangeAudioCallback(AudioHandle::InterleavingAudioCallback cb)
{
    audio_handle.ChangeCallback(cb);
}

void DaisySeed::ChangeAudioCallback(AudioHandle::AudioCallback cb)
{
    audio_handle.ChangeCallback(cb);
}

void DaisySeed::StopAudio()
{
    audio_handle.Stop();
}

void DaisySeed::SetAudioSampleRate(SaiHandle::Config::SampleRate samplerate)
{
    audio_handle.SetSampleRate(samplerate);
    callback_rate_ = AudioSampleRate() / AudioBlockSize();
}

float DaisySeed::AudioSampleRate()
{
    return audio_handle.GetSampleRate();
}

void DaisySeed::SetAudioBlockSize(size_t blocksize)
{
    audio_handle.SetBlockSize(blocksize);
    callback_rate_ = AudioSampleRate() / AudioBlockSize();
}

size_t DaisySeed::AudioBlockSize()
{
    return audio_handle.GetConfig().blocksize;
}

float DaisySeed::AudioCallbackRate() const
{
    return callback_rate_;
}

void DaisySeed::SetLed(bool state)
{
    led.Write(state);
}

void DaisySeed::SetTestPoint(bool state)
{
    testpoint.Write(state);
}

// Private Implementation

void DaisySeed::ConfigureSdram()
{
    Pin *pin_group;
    sdram_handle.state = DSY_SDRAM_STATE_ENABLE;
    pin_group          = sdram_handle.pin_config;
    pin_group[DSY_SDRAM_PIN_SDNWE].Init(Pin::Port::DSY_GPIOH, 5);
}
void DaisySeed::ConfigureQspi()
{
    Pin *pin_group;
    qspi_handle.device = DSY_QSPI_DEVICE_IS25LP064A;
    qspi_handle.mode   = DSY_QSPI_MODE_DSY_MEMORY_MAPPED;
    pin_group          = qspi_handle.pin_config;


    pin_group[DSY_QSPI_PIN_IO0].Init(Pin::Port::DSY_GPIOF, 8);
    pin_group[DSY_QSPI_PIN_IO1].Init(Pin::Port::DSY_GPIOF, 9);
    pin_group[DSY_QSPI_PIN_IO2].Init(Pin::Port::DSY_GPIOF, 7);
    pin_group[DSY_QSPI_PIN_IO3].Init(Pin::Port::DSY_GPIOF, 6);
    pin_group[DSY_QSPI_PIN_CLK].Init(Pin::Port::DSY_GPIOF, 10);
    pin_group[DSY_QSPI_PIN_NCS].Init(Pin::Port::DSY_GPIOG, 6);
}
void DaisySeed::ConfigureAudio()
{
    // SAI2 - config
    // Example Config
    //      SAI2 Pins (available on pinout)
    //    pin_group = sai_handle.sai2_pin_config;
    //    pin_group[DSY_SAI_PIN_MCLK] = dsy_pin(DSY_GPIOA, 1);
    //    pin_group[DSY_SAI_PIN_FS]   = dsy_pin(DSY_GPIOG, 9);
    //    pin_group[DSY_SAI_PIN_SCK]  = dsy_pin(DSY_GPIOA, 2);
    //    pin_group[DSY_SAI_PIN_SIN]  = dsy_pin(DSY_GPIOD, 11);
    //    pin_group[DSY_SAI_PIN_SOUT] = dsy_pin(DSY_GPIOA, 0);

    // SAI1 -- Peripheral
    // Configure
    SaiHandle::Config sai_config;
    sai_config.periph    = SaiHandle::Config::Peripheral::SAI_1;
    sai_config.sr        = SaiHandle::Config::SampleRate::SAI_48KHZ;
    sai_config.bit_depth = SaiHandle::Config::BitDepth::SAI_24BIT;
    sai_config.a_sync    = SaiHandle::Config::Sync::MASTER;
    sai_config.b_sync    = SaiHandle::Config::Sync::SLAVE;
    sai_config.a_dir     = SaiHandle::Config::Direction::TRANSMIT;
    sai_config.b_dir     = SaiHandle::Config::Direction::RECEIVE;
    sai_config.pin_config.fs.Init(Pin::Port::DSY_GPIOE, 4);
    sai_config.pin_config.mclk.Init(Pin::Port::DSY_GPIOE, 2);
    sai_config.pin_config.sck.Init(Pin::Port::DSY_GPIOE, 5);
    sai_config.pin_config.sa.Init(Pin::Port::DSY_GPIOE, 6);
    sai_config.pin_config.sb.Init(Pin::Port::DSY_GPIOE, 3);
    // Then Initialize
    SaiHandle sai_1_handle;
    sai_1_handle.Init(sai_config);

    // Device Init
    Pin codec_reset_pin;
    codec_reset_pin.Init(Pin::Port::DSY_GPIOB, 11);
    //codec_ak4556_init(codec_reset_pin);
    Ak4556::Init(codec_reset_pin);

    // Audio
    AudioHandle::Config audio_config;
    audio_config.blocksize  = 48;
    audio_config.samplerate = SaiHandle::Config::SampleRate::SAI_48KHZ;
    audio_config.postgain   = 1.f;
    audio_handle.Init(audio_config, sai_1_handle);
}
void DaisySeed::ConfigureDac()
{
    // This would be the equivalent initialization as previously existed.
    // However, not all platforms have the DAC, and many use those pins
    // for other things.
    //    DacHandle::Config cfg;
    //    cfg.bitdepth   = DacHandle::Config::BitDepth::BITS_12;
    //    cfg.buff_state = DacHandle::Config::BufferState::ENABLED;
    //    cfg.mode       = DacHandle::Config::Mode::POLLING;
    //    cfg.chn        = DacHandle::Config::Channel::BOTH;
    //    dac.Init(cfg);
}
/*void DaisySeed::ConfigureI2c()
{
    dsy_gpio_pin *pin_group;
    // TODO: Add Config for I2C3 and I2C4
    // I2C 1 - (On daisy patch this controls the LED Driver, and the WM8731).
    i2c1_handle.periph              = DSY_I2C_PERIPH_1;
    i2c1_handle.speed               = DSY_I2C_SPEED_400KHZ;
    pin_group                       = i2c1_handle.pin_config;
    pin_group[DSY_I2C_PIN_SCL].port = DSY_GPIOB;
    pin_group[DSY_I2C_PIN_SCL].pin  = 8;
    pin_group[DSY_I2C_PIN_SDA].port = DSY_GPIOB;
    pin_group[DSY_I2C_PIN_SDA].pin  = 9;
    // I2C 2 - (On daisy patch this controls the on-board WM8731)
    i2c2_handle.periph              = DSY_I2C_PERIPH_2;
    i2c2_handle.speed               = DSY_I2C_SPEED_400KHZ;
    pin_group                       = i2c2_handle.pin_config;
    pin_group[DSY_I2C_PIN_SCL].port = DSY_GPIOH;
    pin_group[DSY_I2C_PIN_SCL].pin  = 4;
    pin_group[DSY_I2C_PIN_SDA].port = DSY_GPIOB;
    pin_group[DSY_I2C_PIN_SDA].pin  = 11;
}*/
