#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>

class Config {
private:
    std::map<String, String> values;

public:
    Config();
    
    void load();
    void set(const String& key, const String& value);
    String get(const String& key, const String& defaultValue = "");
    int getInt(const String& key, int defaultValue = 0);
    bool getBool(const String& key, bool defaultValue = false);
    
    // App configuration
    String getAppName() { return get("app.name", "ESP32 MVC App"); }
    String getAppEnv() { return get("app.env", "production"); }
    bool isDebug() { return getBool("app.debug", false); }
    
    // Server configuration
    String getServerHost() { return get("server.host", "0.0.0.0"); }
    int getServerPort() { return getInt("server.port", 80); }
    
    // Database configuration (for future use)
    String getDatabaseHost() { return get("database.host", "localhost"); }
    int getDatabasePort() { return getInt("database.port", 3306); }
    String getDatabaseName() { return get("database.name", "esp32_app"); }
    String getDatabaseUser() { return get("database.user", "root"); }
    String getDatabasePassword() { return get("database.password", ""); }
};

#endif
