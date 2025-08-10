#include "routes.h"

void registerWifiRoutes(Router* router, WiFiManager* wifiManager) {
		// Create WiFi controllers
		WifiController* wifiController = new WifiController(wifiManager);
		WifiConfigController* wifiConfigController = new WifiConfigController(wifiManager);
		
		// WiFi API routes
		router->group("/api/wifi", [&](Router& wifi) {
				wifi.middleware({"cors", "json"});  // Add authentication middleware in production
				
				// Status endpoint
				wifi.get("/status", [wifiController](Request& request) -> Response {
						return wifiController->status(request);
				}).name("api.wifi.status");
				
				// Scan endpoint
				wifi.get("/scan", [wifiController](Request& request) -> Response {
						return wifiController->scan(request);
				}).name("api.wifi.scan");
				
				// AP management endpoints
				wifi.post("/ap", [wifiController](Request& request) -> Response {
						return wifiController->startAP(request);
				}).name("api.wifi.ap.start");
				
				wifi.put("/ap", [wifiController](Request& request) -> Response {
						return wifiController->updateAP(request);
				}).name("api.wifi.ap.update");
				
				wifi.delete_("/ap", [wifiController](Request& request) -> Response {
						return wifiController->stopAP(request);
				}).name("api.wifi.ap.stop");
				
				// Client management endpoints
				wifi.get("/clients", [wifiController](Request& request) -> Response {
						return wifiController->getClients(request);
				}).name("api.wifi.clients");
				
				wifi.post("/clients/{id}/disconnect", [wifiController](Request& request) -> Response {
						return wifiController->disconnectClient(request);
				}).name("api.wifi.clients.disconnect");
				
				// WiFi configuration endpoints
				wifi.get("/config", [wifiConfigController](Request& request) -> Response {
						return wifiConfigController->getConfigurations(request);
				}).name("api.wifi.config.list");
				
				wifi.post("/config", [wifiConfigController](Request& request) -> Response {
						return wifiConfigController->saveConfiguration(request);
				}).name("api.wifi.config.save");
				
				wifi.delete_("/config/{id}", [wifiConfigController](Request& request) -> Response {
						return wifiConfigController->deleteConfiguration(request);
				}).name("api.wifi.config.delete");
				
				wifi.post("/config/{id}/connect", [wifiConfigController](Request& request) -> Response {
						return wifiConfigController->connectToSaved(request);
				}).name("api.wifi.config.connect");
				
				wifi.post("/scan", [wifiConfigController](Request& request) -> Response {
						return wifiConfigController->scanNetworks(request);
				}).name("api.wifi.scan.networks");
				
				wifi.post("/connect", [wifiConfigController](Request& request) -> Response {
						return wifiConfigController->connectToNetwork(request);
				}).name("api.wifi.connect");
				
				// Route to clean up invalid networks
				wifi.get("/cleanup-networks", [wifiConfigController](Request& request) -> Response {
						return wifiConfigController->cleanUpNetworks(request);
				}).name("api.wifi.cleanup-networks");
		});
}