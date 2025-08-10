#include "routes.h"
#include "../Controllers/IoTDeviceController.h"

void registerIoTRoutes(Router* router, IoTDeviceManager* iotManager) {
    IoTDeviceController* iotController = new IoTDeviceController(iotManager);
    
    router->group("/api/v1/iot", [&](Router& iot) {
        iot.middleware({"cors", "json"});
        
        // Device discovery and management
        iot.get("/devices", [iotController](Request& request) -> Response {
            return iotController->getAllDevices(request);
        }).name("api.iot.devices");
        
        iot.get("/devices/online", [iotController](Request& request) -> Response {
            return iotController->getOnlineDevices(request);
        }).name("api.iot.devices.online");
        
        iot.get("/devices/{id}", [iotController](Request& request) -> Response {
            return iotController->getDevice(request);
        }).name("api.iot.device");
        
        iot.get("/devices/{id}/info", [iotController](Request& request) -> Response {
            return iotController->getDeviceInfo(request);
        }).name("api.iot.device.info");
        
        iot.post("/devices/{id}/command", [iotController](Request& request) -> Response {
            return iotController->executeCommand(request);
        }).name("api.iot.device.command");
        
        iot.post("/devices/{id}/refresh", [iotController](Request& request) -> Response {
            return iotController->refreshDevice(request);
        }).name("api.iot.device.refresh");
        
        // Device filtering
        iot.get("/devices/types/{type}", [iotController](Request& request) -> Response {
            return iotController->getDevicesByType(request);
        }).name("api.iot.devices.by_type");
        
        iot.get("/devices/capabilities/{capability}", [iotController](Request& request) -> Response {
            return iotController->getDevicesByCapability(request);
        }).name("api.iot.devices.by_capability");
        
        // Discovery management
        iot.post("/scan", [iotController](Request& request) -> Response {
            return iotController->scanDevices(request);
        }).name("api.iot.scan");
        
        iot.get("/statistics", [iotController](Request& request) -> Response {
            return iotController->getStatistics(request);
        }).name("api.iot.statistics");
        
        iot.get("/discovery/status", [iotController](Request& request) -> Response {
            return iotController->getDiscoveryStatus(request);
        }).name("api.iot.discovery.status");
        
        iot.post("/discovery/start", [iotController](Request& request) -> Response {
            return iotController->startDiscovery(request);
        }).name("api.iot.discovery.start");
        
        iot.post("/discovery/stop", [iotController](Request& request) -> Response {
            return iotController->stopDiscovery(request);
        }).name("api.iot.discovery.stop");
    });
}
