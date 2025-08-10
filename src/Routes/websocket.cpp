#include "routes.h"

void registerWebSocketRoutes(Router* router) {
		// WiFi clients WebSocket for real-time updates
		router->websocket("/ws/wifi")
				.onConnect([](WebSocketRequest& request) {
						Serial.printf("[WebSocket] WiFi client %u connected\n", request.clientId());
						
						// Send welcome message
						JsonDocument welcome;
						welcome["type"] = "welcome";
						welcome["message"] = "Connected to WiFi status stream";
						
						String welcomeMsg;
						serializeJson(welcome, welcomeMsg);
						request.send(welcomeMsg);
				})
				.onDisconnect([](WebSocketRequest& request) {
						Serial.printf("[WebSocket] WiFi client %u disconnected\n", request.clientId());
				})
				.onMessage([](WebSocketRequest& request, const String& message) {
						// Handle WiFi commands
						JsonDocument doc;
						DeserializationError error = deserializeJson(doc, message);
						
						if (error) {
								Serial.println("[WebSocket] Invalid JSON received");
								return;
						}
						
						String command = doc["command"].as<String>();
						
						if (command == "subscribe") {
								// Handle subscription request
								String topic = doc["topic"].as<String>();
								
								JsonDocument response;
								response["type"] = "subscription";
								response["status"] = "success";
								response["topic"] = topic;
								
								String responseMsg;
								serializeJson(response, responseMsg);
								request.send(responseMsg);
						}
				});
}