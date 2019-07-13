#include <BrewUNO/DisplayService.h>

time_t lastUpdate = now();
String blankline = "                    ";
byte apmode[] = {B01000, B10100, B11100, B10100, B00111, B00101, B00111, B00100};
byte stmode[] = {B01110, B10001, B00100, B01010, B00000, B00100, B00000, B00000};
byte gpump[] = {B00100, B00100, B01110, B01110, B11111, B11101, B11011, B01110};
byte pheater[] = {B11100, B10100, B11100, B10101, B10101, B00111, B00101, B00101};
byte sheater[] = {B11100, B10000, B11100, B00101, B11101, B00111, B00101, B00101};
byte gcelsius[] = {B01000, B10100, B01000, B00110, B01001, B01000, B01001, B00110};
byte gwm[] = {B11111, B01000, B00100, B01000, B11111, B00000, B11111, B00110};
byte gpw[] = {B00110, B11111, B00000, B11100, B10100, B10100, B11111, B00000};

DisplayService::DisplayService(ActiveStatus *activeStatus, WiFiStatus *wifiStatus, LiquidCrystal_I2C *lcd) : _activeStatus(activeStatus),
                                                                                                             _wifiStatus(wifiStatus),
                                                                                                             _lcd(lcd)
{
}

DisplayService::~DisplayService() {}

void DisplayService::begin()
{
    _lcd->init();
    _lcd->backlight();
    _lcd->createChar(apmode_icon, apmode);
    _lcd->createChar(stmode_icon, stmode);
    _lcd->createChar(gpump_icon, gpump);
    _lcd->createChar(pheater_icon, pheater);
    _lcd->createChar(sheater_icon, sheater);
    _lcd->createChar(gcelsius_icon, gcelsius);
    _lcd->createChar(gwm_icon, gwm);
    _lcd->createChar(gpw_icon, gpw);
}

void DisplayService::loop()
{
    if (now() - lastUpdate > 1)
    {
        lastUpdate = now();
        _lcd->home();
        printHead();
        printBody(1, pheater_icon, gwm_icon, _activeStatus->Temperature, _activeStatus->TargetTemperature, _activeStatus->PWMPercentage,
                  _activeStatus->PumpOn, _activeStatus->BrewStarted, true, false, _activeStatus->EnableSparge);
        printBody(2, sheater_icon, gpw_icon, _activeStatus->SpargeTemperature, _activeStatus->SpargeTargetTemperature, _activeStatus->SpargePWMPercentage,
                  _activeStatus->PumpOn, _activeStatus->BrewStarted, false, true, _activeStatus->EnableSparge);
        printFooter();
    }
}

void DisplayService::printHead()
{
    _lcd->setCursor(0, 0);
    wl_status_t status = WiFi.status();
    WiFiMode_t currentWiFiMode = WiFi.getMode();
    if (status == WL_CONNECTED)
        _lcd->write(2);
    else if (currentWiFiMode == WIFI_AP || currentWiFiMode == WIFI_AP_STA)
        _lcd->write(1);
    _lcd->print(" BrewUNO   ");
    if (_activeStatus->BrewStarted)
        _lcd->print(GetCountDown());
}

void DisplayService::printBody(int line, byte heatIcon, byte pwmIcon, double temperature, double targetTemperature,
                               double pwm, bool pump, bool brewStarted, bool showPump, bool sparge, bool enableSparge)
{
    _lcd->setCursor(0, line);
    _lcd->write(heatIcon);
    _lcd->print(" " + getTargetTemp(temperature, enableSparge, sparge, false) +
                ">" + (brewStarted ? getTargetTemp(targetTemperature, enableSparge, sparge, true) : "00"));
    _lcd->setCursor(10, line);
    _lcd->write(gcelsius_icon);
    _lcd->setCursor(13, line);
    _lcd->write(pwmIcon);
    _lcd->setCursor(14, line);

    if (pwm <= 0)
        _lcd->print("  0%");
    else if (pwm <= 99)
        _lcd->print(" " + String(pwm).substring(0, 2) + "%");
    else
        _lcd->print("100%");

    _lcd->setCursor(19, 1);
    if (showPump && pump)
        _lcd->write(gpump_icon);
    else if (showPump)
        _lcd->print("P");
}

void DisplayService::printFooter()
{
    _lcd->setCursor(0, 3);
    if (!_activeStatus->BrewStarted)
    {
        String ip = "";
        wl_status_t status = WiFi.status();
        WiFiMode_t currentWiFiMode = WiFi.getMode();
        if (status == WL_CONNECTED)
            ip = "IP: " + WiFi.localIP().toString();
        else if (currentWiFiMode == WIFI_AP || currentWiFiMode == WIFI_AP_STA)
            ip = "IP: " + WiFi.softAPIP().toString();
        _lcd->print(ip);
        RemoveLastChars(ip);
    }
    else if (_activeStatus->ActiveStep == mash)
    {
        String step = _activeStatus->ActiveMashStepName.substring(0, 13) + " " + _activeStatus->ActiveMashStepSufixName.substring(0, 6);
        _lcd->print(step);
        RemoveLastChars(step);
    }
    else if (_activeStatus->ActiveStep == boil && _activeStatus->ActiveBoilStepName != "")
    {
        String boil = _activeStatus->ActiveBoilStepName.substring(0, 20);
        _lcd->print(boil);
        RemoveLastChars(boil);
    }
    else
        _lcd->print(blankline);
}

String DisplayService::getTargetTemp(double targetTemperature, bool enableSparge, bool sparge, bool target)
{
    String temp = "";
    if (targetTemperature <= 0 || (!enableSparge && sparge))
        temp = "00";
    else if (targetTemperature >= 100)
        temp = target ? "1H" : "100";
    else if (target)
        temp = String(targetTemperature).substring(0, 2);
    else
        temp = String(targetTemperature);
    return temp;
}

void DisplayService::RemoveLastChars(String text)
{
    for (int i = 0; i < 20 - text.length(); i++)
        _lcd->print(" ");
}

String DisplayService::GetCountDown()
{
    char buffer[16];
    int dateEntered = _activeStatus->EndTime;
    int difference = dateEntered - now();
    if (difference <= 0)
        return "00:00:00";
    int seconds = floor(difference);
    int minutes = floor(seconds / 60);
    int hours = floor(minutes / 60);
    hours %= 24;
    minutes %= 60;
    seconds %= 60;
    sprintf(buffer, "%02u:%02u:%02u", hours, minutes, seconds);
    return buffer;
}