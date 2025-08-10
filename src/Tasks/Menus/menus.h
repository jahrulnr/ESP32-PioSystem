#ifndef MENUS_H
#define MENUS_H

#include "SerialDebug.h"
#include "../init.h"

// Menu variables
enum MenuState {
  MENU_MAIN,
  MENU_WIFI_STATUS,
  MENU_WIFI_SCAN,
  MENU_SETTINGS,
  MENU_CLIENTS,
  MENU_IOT_DEVICES,
  MAX_MENU_ITEMS
};

extern MenuState currentMenu;
extern int selectedMenuItem;

void drawMainMenu();
void executeMenuAction(int menuItem);

void displayWiFiStatus();
void displayNetworkList();
void displayConnectedClients();
void displaySettings();
void displayIoTDevices();

#endif