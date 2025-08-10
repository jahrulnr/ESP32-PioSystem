#include "routes.h"

void registerApiRoutes(Router* router) {
		// API routes with middleware
		router->group("/api/v1", [&](Router& api) {
				api.middleware({"cors", "json", "ratelimit"});
				
				// Auth routes for user info (register first to avoid conflicts)
				api.group("/auth", [&](Router& auth) {
						AuthController* authController = new AuthController();
						
						auth.get("/user", [authController](Request& request) -> Response {
								return authController->getUserInfo(request);
						}).name("api.auth.user");
						
						auth.post("/password", [authController](Request& request) -> Response {
								// Password update endpoint (not implemented yet)
								JsonDocument response;
								response["success"] = false;
								response["message"] = "Password update not implemented yet";
								
								return Response(request.getServerRequest())
										.status(200)
										.json(response);
						}).name("api.auth.password");
				});
				
				// Admin routes
				api.group("/admin", [&](Router& admin) {
						admin.middleware({"auth", "admin", "json"}); // Middleware to check if user is admin
						
						AuthController* authController = new AuthController();
						
						admin.get("/users", [authController](Request& request) -> Response {
								// Get all users (not implemented yet)
								JsonDocument response;
								response["success"] = true;
								response["users"] = JsonArray();
								
								// Return user data
								
								return Response(request.getServerRequest())
										.status(200)
										.json(response);
						}).name("api.admin.users");
				});
		});
}
