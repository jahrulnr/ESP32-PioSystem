#include "routes.h"
#include <SPIFFS.h>

void registerWebRoutes(Router* router) {
		AuthController* authController = new AuthController();

		// Single-page application route
		router->get("/", [](Request& request) -> Response {
				// Serve the dashboard.html as the main entry point
				if (SPIFFS.exists("/views/dashboard.html")) {
						return Response(request.getServerRequest())
								.file("/views/dashboard.html");
				}
				
				return Response(request.getServerRequest())
						.status(404);
		}).name("app");
		
		// Dashboard route
		router->get("/dashboard", [](Request& request) -> Response {
				if (SPIFFS.exists("/views/dashboard.html")) {
						return Response(request.getServerRequest())
								.file("/views/dashboard.html");
				}
				
				return Response(request.getServerRequest())
						.status(404);
		}).name("dashboard");
		
		// WiFi Configuration route
		router->get("/wifi-config", [](Request& request) -> Response {
				if (SPIFFS.exists("/views/wifi-config.html")) {
						return Response(request.getServerRequest())
								.file("/views/wifi-config.html");
				}
				
				return Response(request.getServerRequest())
						.status(404);
		}).name("wifi-config");
		
		// WiFi Test route (for debugging)
		router->get("/wifi-test", [](Request& request) -> Response {
				if (SPIFFS.exists("/views/wifi-test.html")) {
						return Response(request.getServerRequest())
								.file("/views/wifi-test.html");
				}
				
				return Response(request.getServerRequest())
						.status(404);
		}).name("wifi-test");
		
		// Authentication routes
		router->get("/login", [](Request& request) -> Response {
				if (SPIFFS.exists("/views/login.html")) {
						return Response(request.getServerRequest())
								.file("/views/login.html");
				}
				
				// Redirect to the main app with login hash
				return Response(request.getServerRequest())
						.redirect("/#login");
		}).name("login.show");
		
		router->post("/login", [authController](Request& request) -> Response {
				return authController->login(request);
		}).name("login");
		
		router->post("/logout", [authController](Request& request) -> Response {
				return authController->logout(request);
		}).name("logout");
		
		// Static file serving for CSS, JS, and other assets
		router->get("/assets/{file}", [](Request& request) -> Response {
				String file = request.route("file");
				String path = "/assets/" + file;
				
				return Response(request.getServerRequest())
						.file(path);
		}).name("assets");


		router->get("/favicon.ico", [](Request& request) -> Response {
				return Response(request.getServerRequest())
						.file("/favicon.ico");
		});
}



